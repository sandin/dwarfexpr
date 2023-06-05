#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include <cstdint>

#include "retdec/dwarfparser/dwarf_file.h"
#include "retdec/dwarfparser/dwarf_searcher.h"


using namespace retdec::dwarfparser;

const char USAGE[] = "USAGE: dwarf2line [options] [addresses]\n"
                     " Options:\n"
                     "  -e --exe <executable>   Set the input filename\n"
                     "  -f --functions          Show function names\n";
 
int main(int argc, char** argv) 
{
    // parse args
    std::string input;
    bool print_func_name = false;
    std::vector<uint64_t> addresses;
    for (int i = 1; i< argc; ++i) {
        if (!strcmp(argv[i], "-e") || !strcmp(argv[i], "--exe")) {
            ++i;
            if (i >= argc) {
                printf("Error: missing `-e` arg.\n");
            }
            input = argv[i];
        } else if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--functions")) {
            print_func_name = true;
        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            printf("%s", USAGE);
            return 0;
        } else {
            std::string arg = argv[i];
            uint64_t addr = std::stoul(arg, 0, 16);
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

    int res = res = dwarf_init_path(input.c_str(),
        real_path,
        PATH_LEN,
        DW_GROUPNUMBER_ANY,errhand,errarg,&dbg,&error);
    if (res == DW_DLV_ERROR) {
        printf("Giving up, cannot do DWARF processing of %s "
            "dwarf err %llx %s\n",
            input,
            dwarf_errno(error),
            dwarf_errmsg(error));
        dwarf_dealloc_error(dbg,error);
        dwarf_finish(dbg);
        exit(EXIT_FAILURE);
    }
    if (res == DW_DLV_NO_ENTRY) {
        printf("Giving up, file %s not found\n", input);
        exit(EXIT_FAILURE);
    }

    DwarfSearcher searcher(dbg);
    for (uint64_t address : addresses) {
        Dwarf_Die cu_die;
        Dwarf_Die func_die;
        Dwarf_Error* errp;
        bool found = searcher.searchFunction(address, &cu_die, &func_die, errp);
        if (found) {
            // TODO: 
            dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
            dwarf_dealloc(dbg, func_die, DW_DLA_DIE);
        }
        /*
        for (DwarfFunction* func : *dwarf_file.getFunctions()) {
            if (func->lowAddr <= address && address < func->highAddr) {
                if (print_func_name) {
                    printf("%s\n", func->getName().c_str());
                }
                printf("%s:%llu\n", func->file.c_str(), func->line);
            }
        }
        */
    }

    res = dwarf_finish(dbg);
    if (res != DW_DLV_OK) {
        printf("dwarf_finish failed!\n");
    }
    return 0;




 

    return 0;
}