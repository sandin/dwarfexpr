#include "dwarfexpr/dwarf_utils.h"

#include <cxxabi.h>  // abi::__cxa_demangle
#include <string.h>  // strdup

#include <cstdlib>
#include <iomanip>  // std::setfill std::setw
#include <sstream>

using namespace std;

namespace dwarfexpr {

#define DWARF_ERROR(s) printf("DwarfError: %s\n", s.c_str())

/**
 * @brief Get address from attribute.
 * @param attr Address class attribute.
 * @return Address represented by the attribute.
 */
Dwarf_Addr getAttrAddr(Dwarf_Attribute attr, Dwarf_Addr def_val) {
  Dwarf_Error error = nullptr;
  Dwarf_Addr retVal = 0;

  if (dwarf_formaddr(attr, &retVal, &error) == DW_DLV_OK) {
    return retVal;
  } else {
    DWARF_ERROR(getDwarfError(error));
    return def_val;
  }
}

/**
 * @brief Get value of the attribute.
 * @param attr Constant class attribute.
 * @return Value of the attrinute.
 */
Dwarf_Unsigned getAttrNumb(Dwarf_Attribute attr, Dwarf_Unsigned def_val) {
  Dwarf_Error error = nullptr;

  Dwarf_Unsigned uRetVal = 0;
  if (dwarf_formudata(attr, &uRetVal, &error) == DW_DLV_OK) {
    return uRetVal;
  }

  Dwarf_Signed sRetVal = 0;
  if (dwarf_formsdata(attr, &sRetVal, &error) == DW_DLV_OK) {
    return sRetVal;
  }

  DWARF_ERROR(getDwarfError(error));
  return def_val;
}

/**
 * @brief Get string from the attribute.
 * @param attr String class attribute.
 * @return String containing the value of the attribute.
 */
string getAttrStr(Dwarf_Attribute attr, string def_val) {
  Dwarf_Error error = nullptr;

  char* name = nullptr;
  if (dwarf_formstring(attr, &name, &error) == DW_DLV_OK) {
    return string(name);
  } else {
    DWARF_ERROR(getDwarfError(error));
    return def_val;
  }
}

/**
 * @brief Get reference from the attribute.
 * @param attr Reference class attribute.
 * @return Offset represented by the attribute.
 */
Dwarf_Off getAttrRef(Dwarf_Attribute attr, Dwarf_Off def_val) {
  Dwarf_Error error = nullptr;
  Dwarf_Bool is_info = true;

  Dwarf_Off retVal;
  if (dwarf_formref(attr, &retVal, &is_info, &error) == DW_DLV_OK) {
    return retVal;
  } else {
    DWARF_ERROR(getDwarfError(error));
    return def_val;
  }
}

/**
 * @brief Get global reference from the attribute.
 * @param attr Reference or other section-references class
 *        attribute.
 * @return Global offset represented by the attribute.
 */
Dwarf_Off getAttrGlobalRef(Dwarf_Attribute attr, Dwarf_Off def_val) {
  Dwarf_Error error = nullptr;

  Dwarf_Off retVal;
  if (dwarf_global_formref(attr, &retVal, &error) == DW_DLV_OK) {
    return retVal;
  } else {
    DWARF_ERROR(getDwarfError(error));
    return def_val;
  }
}

/**
 * @brief Get flag from the attribute.
 * @param attr Attribute.
 * @return True if attribute has a non-zero value, else false.
 */
Dwarf_Bool getAttrFlag(Dwarf_Attribute attr, Dwarf_Bool def_val) {
  Dwarf_Error error = nullptr;

  Dwarf_Bool retVal;
  if (dwarf_formflag(attr, &retVal, &error) == DW_DLV_OK) {
    return retVal;
  } else {
    DWARF_ERROR(getDwarfError(error));
    return def_val;
  }
}

/**
 * @brief Get block from the attribute.
 * @param attr Block class attribute.
 * @return Pointer to block structure represented by the attribute,
 */
Dwarf_Block* getAttrBlock(Dwarf_Attribute attr) {
  Dwarf_Error error = nullptr;

  Dwarf_Block* retVal = nullptr;
  if (dwarf_formblock(attr, &retVal, &error) == DW_DLV_OK) {
    return retVal;
  } else {
    DWARF_ERROR(getDwarfError(error));
    return nullptr;
  }
}

/**
 * @brief Get 8 byte signature from the attribute.
 * @param attr Attribute of DW_FORM_ref_sig8 form.
 * @return 8 byte signature represented by the attribute.
 */
Dwarf_Sig8 getAttrSig(Dwarf_Attribute attr) {
  Dwarf_Error error = nullptr;

  Dwarf_Sig8 retVal;
  if (dwarf_formsig8(attr, &retVal, &error) == DW_DLV_OK) {
    return retVal;
  } else {
    DWARF_ERROR(getDwarfError(error));
    return Dwarf_Sig8();
  }
}

/**
 * @brief Gets length of the location expression and pointer to
 *        the bytes of the location expression from the attribute.
 * @param attr    Attribute of DW_FORM_exprloc form.
 * @param exprlen Pointer to location expression length to set.
 * @param ptr     Pointer to location expression pointer to set.
 */
void getAttrExprLoc(Dwarf_Attribute attr, Dwarf_Unsigned* exprlen,
                    Dwarf_Ptr* ptr) {
  Dwarf_Error error = nullptr;

  if (dwarf_formexprloc(attr, exprlen, ptr, &error) != DW_DLV_OK) {
    DWARF_ERROR(getDwarfError(error));
    // non-recoverable --  exit(1) ???
  }
}

/**
 * @brief Gets libdwarf error message to provided error code.
 * @param error Error code.
 * @return Error message as strings.
 */
string getDwarfError(Dwarf_Error& error) { return string(dwarf_errmsg(error)); }

/**
 * @brief Gets DIE from provided offset.
 * @param dbg Debug file.
 * @param off Offset.
 * @param die Found die.
 * @return True if success, false otherwise.
 */
bool getDieFromOffset(Dwarf_Debug dbg, Dwarf_Off off, Dwarf_Die& die) {
  bool is_info = true;
  Dwarf_Error error;
  int res = dwarf_offdie_b(dbg, off, is_info, &die, &error);

  if (res == DW_DLV_ERROR) {
    DWARF_ERROR(getDwarfError(error));
    return false;
  }

  if (res == DW_DLV_NO_ENTRY) {
    return false;
  }

  return true;
}

int getLowAndHighPc(Dwarf_Debug dbg, Dwarf_Die die, bool* have_pc_range,
                    Dwarf_Addr* lowpc_out, Dwarf_Addr* highpc_out,
                    Dwarf_Error* error) {
  Dwarf_Addr hipc = 0;
  int res = 0;
  Dwarf_Half form = 0;
  enum Dwarf_Form_Class formclass = DW_FORM_CLASS_UNKNOWN;

  *have_pc_range = false;
  res = dwarf_lowpc(die, lowpc_out, error);
  if (res == DW_DLV_OK) {
    res = dwarf_highpc_b(die, &hipc, &form, &formclass, error);
    if (res == DW_DLV_OK) {
      if (formclass == DW_FORM_CLASS_CONSTANT) {
        hipc += *lowpc_out;
      }
      *highpc_out = hipc;
      *have_pc_range = true;
      return DW_DLV_OK;
    }
  }
  /*  Cannot check ranges yet, we don't know the ranges base
      offset yet. */
  return DW_DLV_NO_ENTRY;
}

int getRnglistsBase(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Off* base_out,
                    Dwarf_Error* error) {
  int res;
  *base_out = 0;

  Dwarf_Attribute attr;
  res = dwarf_attr(die, DW_AT_rnglists_base, &attr, error);
  if (res != DW_DLV_OK) {
    res = dwarf_attr(die, DW_AT_GNU_ranges_base, &attr, error);
    if (res != DW_DLV_OK) {
      dwarf_dealloc(dbg, attr, DW_DLA_LIST);
      return res;
    }
  }
  // else: res == DW_DLV_OK
  res = dwarf_global_formref(attr, base_out, error);
  dwarf_dealloc(dbg, attr, DW_DLA_LIST);
  return res;
}

std::string getFunctionName(Dwarf_Debug dbg, Dwarf_Die func_die, bool demangle,
                            std::string def_val) {
  Dwarf_Error error = nullptr;

  // Linkage name(mangled name)
  Dwarf_Attribute attr1 = nullptr;
  if (dwarf_attr(func_die, DW_AT_linkage_name, &attr1, &error) == DW_DLV_OK) {
    auto guard =
        make_scope_exit([&]() { dwarf_dealloc(dbg, attr1, DW_DLA_ATTR); });
    return demangle ? demangleName(getAttrStr(attr1, def_val))
                    : getAttrStr(attr1, def_val);
  }
  Dwarf_Attribute attr2 = nullptr;
  if (dwarf_attr(func_die, DW_AT_MIPS_linkage_name, &attr2, &error) ==
      DW_DLV_OK) {
    auto guard =
        make_scope_exit([&]() { dwarf_dealloc(dbg, attr2, DW_DLA_ATTR); });
    return demangle ? demangleName(getAttrStr(attr2, def_val))
                    : getAttrStr(attr2, def_val);
  }
  Dwarf_Attribute attr3 = nullptr;
  if (dwarf_attr(func_die, DW_AT_HP_linkage_name, &attr3, &error) ==
      DW_DLV_OK) {
    auto guard =
        make_scope_exit([&]() { dwarf_dealloc(dbg, attr3, DW_DLA_ATTR); });
    return demangle ? demangleName(getAttrStr(attr3, def_val))
                    : getAttrStr(attr3, def_val);
  }

  // DW_AT_name
  char* name = nullptr;
  if (dwarf_diename(func_die, &name, &error) == DW_DLV_OK) {
    return string(name);  // copy
  }

  // Some functions have a "specification" attribute
  // which means they were defined elsewhere. The name
  // attribute is not repeated, and must be taken from
  // the specification DIE.
  Dwarf_Attribute attr;
  if (dwarf_attr(func_die, DW_AT_specification, &attr, &error) == DW_DLV_OK) {
    auto guard =
        make_scope_exit([&]() { dwarf_dealloc(dbg, attr, DW_DLA_ATTR); });

    Dwarf_Off ref_die_ref = getAttrGlobalRef(attr, MAX_DWARF_OFF);
    if (ref_die_ref != MAX_DWARF_OFF) {
      Dwarf_Die ref_die;
      if (getDieFromOffset(dbg, ref_die_ref, ref_die)) {
        auto die_guard =
            make_scope_exit([&]() { dwarf_dealloc(dbg, ref_die, DW_DLA_DIE); });
        return getFunctionName(dbg, ref_die, demangle, def_val);
      }
    }
  }

  DWARF_ERROR(getDwarfError(error));
  return def_val;
}

std::string demangleName(const std::string& mangled) {
  std::string result;

  int status;
  char* demangled_c = abi::__cxa_demangle(mangled.c_str(), NULL, NULL, &status);
  if (status == 0) {
    // success
    result.assign(demangled_c);
  } else {
    // failed
    result.assign(mangled);
  }

  if (demangled_c) {
    free(reinterpret_cast<void*>(demangled_c));
  }

  return result;
}

std::string getDeclFile(Dwarf_Debug dbg, Dwarf_Die cu_die, Dwarf_Die func_die,
                        std::string def_val) {
  Dwarf_Error error = nullptr;
  Dwarf_Attribute file_attr = nullptr;

  if (dwarf_attr(func_die, DW_AT_decl_file, &file_attr, &error) == DW_DLV_OK) {
    Dwarf_Unsigned fileNum = getAttrNumb(file_attr, MAX_DWARF_UNSIGNED);
    dwarf_dealloc(dbg, file_attr, DW_DLA_ATTR);

    if (fileNum != MAX_DWARF_UNSIGNED) {
      Dwarf_Signed fileIdx = fileNum - 1;

      // Get all source files of cu.
      char** srcFiles;
      Dwarf_Signed fileCnt;
      if (dwarf_srcfiles(cu_die, &srcFiles, &fileCnt, &error) == DW_DLV_OK) {
        if (fileIdx < fileCnt) {
          std::string retVal = srcFiles[fileIdx];
          dwarf_dealloc(dbg, srcFiles, DW_DLA_LIST);
          return retVal;
        }
        dwarf_dealloc(dbg, srcFiles, DW_DLA_LIST);
      }
    }
  }

  DWARF_ERROR(getDwarfError(error));
  return def_val;
}

Dwarf_Unsigned getDeclLine(Dwarf_Debug dbg, Dwarf_Die func_die,
                           Dwarf_Unsigned def_val) {
  Dwarf_Error error = nullptr;
  Dwarf_Attribute line_attr = nullptr;
  Dwarf_Unsigned retVal = 0;

  if (dwarf_attr(func_die, DW_AT_decl_line, &line_attr, &error) == DW_DLV_OK) {
    retVal = getAttrNumb(line_attr, def_val);
    dwarf_dealloc(dbg, line_attr, DW_DLA_ATTR);
    return retVal;
  } else {
    DWARF_ERROR(getDwarfError(error));
    return def_val;
  }
}

std::pair<std::string, Dwarf_Unsigned> getFileNameAndLineNumber(
    Dwarf_Debug dbg, Dwarf_Die cu_die, Dwarf_Addr pc, std::string def_val1,
    Dwarf_Unsigned def_val2) {
  auto result = std::make_pair(def_val1, def_val2);

  Dwarf_Error error = nullptr;
  Dwarf_Unsigned line_version = 0;
  Dwarf_Small table_type = 0;
  Dwarf_Line_Context line_context = 0;

  if (dwarf_srclines_b(cu_die, &line_version, &table_type, &line_context,
                       &error) != DW_DLV_OK) {
    DWARF_ERROR(getDwarfError(error));
    return result;
  }
  auto line_context_guard =
      make_scope_exit([&]() { dwarf_srclines_dealloc_b(line_context); });

  if (table_type != 1) {
    // if table_type == 0: no lines, just table header and names
    // if table_type == 2: experimental two-level line table, not standard DWARF
    printf("Error: unsupport table type %d\n", table_type);
    return result;
  }

  Dwarf_Line* line_buf = 0;
  Dwarf_Signed line_count = 0;
  if (dwarf_srclines_from_linecontext(line_context, &line_buf, &line_count,
                                      &error) != DW_DLV_OK) {
    DWARF_ERROR(getDwarfError(error));
    return result;
  }
  // no need to free `line_buf`, `line_context` is its owner.

  Dwarf_Addr prev_lineaddr = 0;
  Dwarf_Unsigned prev_lineno = 0;
  char* prev_linesrcfile = nullptr;
  bool found = false;
  for (int i = 0; i < line_count; ++i) {
    Dwarf_Line line = line_buf[i];

    Dwarf_Unsigned lineno = 0;
    if (dwarf_lineno(line, &lineno, &error) != DW_DLV_OK) {
      DWARF_ERROR(getDwarfError(error));
      break;
    }

    /*
    Dwarf_Unsigned filenum = 0;
    if (dwarf_line_srcfileno(line, &filenum, errp) != DW_DLV_OK) {
            continue;
    }
    if (filenum >= 1) {
            filenum -= 1;
    }
    */

    char* linesrcfile = nullptr;
    if (dwarf_linesrc(line, &linesrcfile, &error) != DW_DLV_OK) {
      DWARF_ERROR(getDwarfError(error));
      break;
    }

    Dwarf_Addr lineaddr = 0;
    if (dwarf_lineaddr(line, &lineaddr, &error) != DW_DLV_OK) {
      DWARF_ERROR(getDwarfError(error));
      break;
    }

    if (lineaddr > pc) {
      result.first.assign(prev_linesrcfile);
      result.second = prev_lineno;
      found = true;
      break;
    }

    prev_lineaddr = lineaddr;
    prev_lineno = lineno;
    if (prev_linesrcfile) {
      dwarf_dealloc(dbg, prev_linesrcfile, DW_DLA_STRING);
    }
    prev_linesrcfile = linesrcfile;
  }

  if (!found) {
    result.first.assign(prev_linesrcfile);
    result.second = prev_lineno;
    found = true;
  }
  if (prev_linesrcfile) {
    dwarf_dealloc(dbg, prev_linesrcfile, DW_DLA_STRING);
  }

  return result;
}

void walkDIE(Dwarf_Debug dbg, Dwarf_Die parent_die, Dwarf_Die die, int cur_lv,
             int max_lv, void* ctx, DwarfDIEWalker walker) {
  int res = DW_DLV_ERROR;
  Dwarf_Error error = nullptr;
  Dwarf_Die cur_die = die;

  walker(dbg, parent_die, cur_die, cur_lv, max_lv, ctx);

  // Siblings and childs
  while (true) {
    Dwarf_Die child = nullptr;
    res = dwarf_child(cur_die, &child, &error);
    if (res == DW_DLV_ERROR) {
      DWARF_ERROR(getDwarfError(error));
      return;
    }

    // Has child -> recursion
    if (res == DW_DLV_OK) {
      if (max_lv == -1 || cur_lv < max_lv) {
        walkDIE(dbg, cur_die, child, ++cur_lv, max_lv, ctx, walker);
      }
    }

    // DW_DLV_NO_ENTRY
    // Not entry -> no child
    Dwarf_Die sib_die = nullptr;
    res = dwarf_siblingof_b(dbg, cur_die, true, &sib_die, &error);
    if (res == DW_DLV_ERROR) {
      DWARF_ERROR(getDwarfError(error));
      return;
    }

    // Done at this level
    if (res == DW_DLV_NO_ENTRY) {
      --cur_lv;
      break;
    }

    if (cur_die != die) {
      dwarf_dealloc(dbg, cur_die, DW_DLA_DIE);
    }

    cur_die = sib_die;
    walker(dbg, parent_die, cur_die, cur_lv, max_lv, ctx);
  }
}

void dumpDIE(Dwarf_Debug dbg, Dwarf_Die die) {
  int res = DW_DLV_ERROR;
  Dwarf_Error error = nullptr;

  Dwarf_Half tag = 0;
  res = dwarf_tag(die, &tag, &error);
  if (res == DW_DLV_OK) {
    const char* tag_name = nullptr;
    dwarf_get_TAG_name(tag, &tag_name);

    Dwarf_Off offset;
    dwarf_dieoffset(die, &offset, &error);
    printf("0x%llx: %s\n", offset, tag_name);
  }

  /*
  if (tag == DW_TAG_compile_unit) {
          // Get all source files of cu.
          char **srcFiles;
          Dwarf_Signed fileCnt;
          if (dwarf_srcfiles(die, &srcFiles, &fileCnt, &error) == DW_DLV_OK) {
                  printf("\t\tSource files:\n");
                  for (int i = 0; i < fileCnt; ++i) {
                          printf("\t\t\t%s\n", srcFiles[i]);
                          dwarf_dealloc(dbg, srcFiles[i], DW_DLA_STRING);
                  }
                  dwarf_dealloc(dbg, srcFiles, DW_DLA_LIST);
          }
  }
  */

  char* diename = nullptr;
  dwarf_diename(die, &diename, &error);
  if (res == DW_DLV_OK) {
    printf("\t\tDW_AT_name: %s\n", diename);
  }

  Dwarf_Attribute* attrlist = nullptr;
  Dwarf_Signed attrcount = 0;
  res = dwarf_attrlist(die, &attrlist, &attrcount, &error);
  if (res == DW_DLV_OK) {
    for (int i = 0; i < attrcount; ++i) {
      Dwarf_Attribute attr = attrlist[i];

      Dwarf_Half atr = 0;
      res = dwarf_whatattr(attr, &atr, &error);
      if (res == DW_DLV_OK) {
        const char* attr_name = nullptr;
        dwarf_get_AT_name(atr, &attr_name);
        printf("\t\t%s: ", attr_name);
        // TODO: attr value
        switch (atr) {
          case DW_AT_specification: {
            Dwarf_Off funcDieRef = getAttrGlobalRef(attr, MAX_DWARF_OFF);
            printf("ref=0x%llx", funcDieRef);

            /*
            Dwarf_Die refFuncDie;
            res = dwarf_offdie_b(dbg, funcDieRef, 1, &refFuncDie, &error);
            if (res == DW_DLV_OK) {
                    dumpDIE(dbg, refFuncDie);
            }
            */
            break;
          }
          default:
            printf("...");
            break;
        }
        printf("\n");
      }

      dwarf_dealloc_attribute(attr);
    }

    dwarf_dealloc(dbg, attrlist, DW_DLA_LIST);
  }
  printf("\n");
}

std::string hexstring(char* buf, size_t buf_size) {
  std::stringstream ss;
  for (size_t i = 0; i < buf_size; ++i) {
    ss << std::setfill('0') << std::setw(2) << std::hex
       << (0xff & buf[i]) << " ";
  }
  return ss.str();
}

}  // namespace dwarfexpr
