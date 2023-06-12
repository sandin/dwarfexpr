#ifndef DWARFEXPR_DWARF_VARS_H
#define DWARFEXPR_DWARF_VARS_H

#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

#include <string>

#include "dwarfexpr/dwarf_tag.h"
#include "dwarfexpr/dwarf_types.h"
#include "dwarfexpr/dwarf_location.h"

namespace dwarfexpr {

// DW_TAG_variable
// DW_TAG_constant
// DW_TAG_formal_parameter
class DwarfVar : public DwarfTag {
 public:
  DwarfVar(Dwarf_Debug dbg, Dwarf_Off offset)
      : DwarfTag(dbg, offset), name_(""), type_(nullptr) {}
  virtual ~DwarfVar() {
    if (type_) {
      delete type_;
    }
  }

  virtual bool load() override;

  virtual std::string name() const { return name_; }
  virtual DwarfType* type() const { return type_; }

 private:
  DwarfType* loadType();
  DwarfLocation* loadLocation();

 private:
  std::string name_;
  DwarfType* type_;
  DwarfLocation* location_;

};  // class DwarfVar

}  // namespace dwarfexpr

#endif  // DWARFEXPR_DWARF_VARS_H
