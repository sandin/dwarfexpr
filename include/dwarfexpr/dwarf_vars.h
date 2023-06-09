#ifndef DWARFEXPR_DWARF_VARS_H
#define DWARFEXPR_DWARF_VARS_H

#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

#include <string>

#include "dwarfexpr/dwarf_types.h"

namespace dwarfexpr {

// DW_TAG_variable
// DW_TAG_constant
// DW_TAG_formal_parameter
class DwarfVar {
 public:
  DwarfVar(Dwarf_Debug dbg, Dwarf_Off offset)
      : dbg_(dbg), offset_(offset), die_(nullptr), name_(""), type_(nullptr) {}
  virtual ~DwarfVar() {
    if (type_) {
      delete type_;
    }
    if (die_) {
      dwarf_dealloc(dbg_, die_, DW_DLA_DIE);
    }
  }

  virtual bool load();  // TODO:

  std::string name() const { return name_; }
  DwarfType* type() const { return type_; }
  Dwarf_Off offset() const { return offset_; }
  Dwarf_Half tag() const { return tag_; }
  std::string tagName() const {
    const char* tag_name = nullptr;
    dwarf_get_TAG_name(tag_, &tag_name);
    return tag_name;
  }

 private:
  DwarfType* loadType();

 private:
  Dwarf_Debug dbg_;
  Dwarf_Off offset_;

  Dwarf_Die die_;
  Dwarf_Half tag_;
  std::string name_;
  DwarfType* type_;
  // DwarfLocation* location_;

};  // class DwarfVar

}  // namespace dwarfexpr

#endif  // DWARFEXPR_DWARF_VARS_H
