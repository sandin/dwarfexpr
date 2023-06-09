#include <string.h>

#include <cstdint>
#include <iostream>
#include <limits>  // numeric_limits
#include <string>
#include <utility>  // std::make_pair
#include <vector>

#include "dwarfexpr/dwarf_searcher.h"
#include "dwarfexpr/dwarf_types.h"
#include "dwarfexpr/dwarf_utils.h"
#include "dwarfexpr/dwarf_vars.h"

using namespace dwarfexpr;

const char USAGE[] =
    "USAGE: dwarf2line [options] [addresses]\n"
    " Options:\n"
    "  -e --exe <executable>   Set the input filename\n"
    "  -f --functions          Show function names\n"
    "  -C --demangle           Demangle function names\n"
    "\n"
    "  -l --locals             Show local variables\n"
    "  -p --params             Show function params\n"
    "  -v --verbose            Show debug log\n";

int main(int argc, char** argv) {
  // parse args
  std::string input;
  bool print_func_name = false;
  bool demangle = false;
  bool show_locals = false;
  bool show_params = false;
  bool debug = false;
  std::vector<uint64_t> addresses;
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "-e") || !strcmp(argv[i], "--exe")) {
      ++i;
      if (i >= argc) {
        printf("Error: missing `-e` arg.\n");
      }
      input = argv[i];
    } else if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--functions")) {
      print_func_name = true;
    } else if (!strcmp(argv[i], "-C") || !strcmp(argv[i], "--demangle")) {
      demangle = true;
    } else if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "--locals")) {
      show_locals = true;
    } else if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--params")) {
      show_params = true;
    } else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) {
      debug = true;
    } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
      printf("%s", USAGE);
      return 0;
    } else {
      std::string arg = argv[i];
      // printf("addr: %s\n", arg.c_str());
      uint64_t addr = std::stoul(arg, 0, 16);  // TODO: unsafe
      addresses.emplace_back(addr);
    }
  }
  if (input.empty()) {
    printf("Error: missing the input `-e` arg.\n");
    printf("%s", USAGE);
    return -1;
  }
  if (addresses.empty()) {
    printf("Error: missing address arg.\n");
    printf("%s", USAGE);
    return -1;
  }

  Dwarf_Debug dbg = nullptr;
#define PATH_LEN 2000
  char real_path[PATH_LEN];
  Dwarf_Error error;
  Dwarf_Handler errhand = 0;
  Dwarf_Ptr errarg = 0;

  int res = res =
      dwarf_init_path(input.c_str(), real_path, PATH_LEN, DW_GROUPNUMBER_ANY,
                      errhand, errarg, &dbg, &error);
  if (res == DW_DLV_ERROR) {
    printf(
        "Giving up, cannot do DWARF processing of %s "
        "dwarf err %llx %s\n",
        input.c_str(), dwarf_errno(error), dwarf_errmsg(error));
    dwarf_dealloc_error(dbg, error);
    dwarf_finish(dbg);
    exit(EXIT_FAILURE);
  }
  if (res == DW_DLV_NO_ENTRY) {
    printf("Giving up, file %s not found\n", input.c_str());
    exit(EXIT_FAILURE);
  }

  DwarfSearcher searcher(dbg);
  for (uint64_t address : addresses) {
    Dwarf_Die cu_die;
    Dwarf_Die func_die;
    Dwarf_Error* errp = nullptr;
    bool found = searcher.searchFunction(address, &cu_die, &func_die, errp);
    if (found) {
      // dumpDIE(dbg, cu_die);
      dumpDIE(dbg, func_die);

      if (print_func_name) {
        std::string function_name =
            getFunctionName(dbg, func_die, demangle, "?");
        printf("%s\n", function_name.c_str());
      }

      std::pair<std::string, Dwarf_Unsigned> file_line =
          getFileNameAndLineNumber(dbg, cu_die, address, "?",
                                   MAX_DWARF_UNSIGNED);
      std::string file_name = file_line.first;
      Dwarf_Unsigned line_number = file_line.second;
      if (line_number != MAX_DWARF_UNSIGNED) {
        printf("%s:%llu\n", file_name.c_str(), line_number);
      } else {
        printf("%s:?\n", file_name.c_str());
      }

      if (show_locals || show_params) {
        void* ctx = nullptr;
        std::vector<DwarfVar*> locals;
        std::vector<DwarfVar*> params;

        walkDIE(dbg, cu_die, func_die, 0, 1, ctx,
                [&](Dwarf_Debug dbg, Dwarf_Die parent_die, Dwarf_Die child_die,
                    int cur_lv, int max_lv, void* ctxt) {
                  Dwarf_Error err = nullptr;
                  Dwarf_Half tag = 0;
                  if (dwarf_tag(child_die, &tag, &err) == DW_DLV_OK) {
                    if (tag == DW_TAG_variable || tag == DW_TAG_constant ||
                        tag == DW_TAG_formal_parameter) {
                      Dwarf_Off tag_offset;
                      dwarf_dieoffset(child_die, &tag_offset, &error);

                      const char* tag_name;
                      dwarf_get_TAG_name(tag, &tag_name);

                      DwarfVar* var = new DwarfVar(dbg, tag_offset);
                      if (!var->load()) {
                        printf("Error: can not load var 0x%llx %s\n",
                               tag_offset, tag_name);
                        delete var;
                        return;
                      }

                      if (tag == DW_TAG_formal_parameter) {
                        params.emplace_back(var);
                      } else {
                        locals.emplace_back(var);
                      }
                    }
                  }
                });  // end walkDIE
        printf("params:\n");
        for (const DwarfVar* var : params) {
          DwarfType* type = var->type();
          if (debug) {
            printf("  0x%llx: %s ", var->offset(), var->tagName().c_str());
            printf("  0x%llx: %s ", type->offset(), type->tagName().c_str());
          }
          printf("  %s %s (%zu bytes)\n", type->name().c_str(),
                 var->name().c_str(), type->size());
          delete var;
        }

        printf("locals:\n");
        for (const DwarfVar* var : locals) {
          DwarfType* type = var->type();
          if (debug) {
            printf("  0x%llx: %s ", var->offset(), var->tagName().c_str());
            printf("  0x%llx: %s ", type->offset(), type->tagName().c_str());
          }
          printf("  %s %s (%zu bytes)\n", type->name().c_str(),
                 var->name().c_str(), type->size());
          delete var;
        }
      }

      dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
      dwarf_dealloc(dbg, func_die, DW_DLA_DIE);
    } else {
      printf("Not found.\n");
    }
  }

  res = dwarf_finish(dbg);
  if (res != DW_DLV_OK) {
    printf("dwarf_finish failed!\n");
  }
  return 0;
}