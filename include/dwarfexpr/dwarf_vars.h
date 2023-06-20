#ifndef DWARFEXPR_DWARF_VARS_H
#define DWARFEXPR_DWARF_VARS_H

#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

#include <string>

#include "dwarfexpr/dwarf_expression.h"
#include "dwarfexpr/dwarf_location.h"
#include "dwarfexpr/dwarf_tag.h"
#include "dwarfexpr/dwarf_types.h"

namespace dwarfexpr {

// DW_TAG_variable
// DW_TAG_constant
// DW_TAG_formal_parameter
class DwarfVar : public DwarfTag {
 public:
  using DwarfValue = std::string;

  DwarfVar(Dwarf_Debug dbg, Dwarf_Off offset)
      : DwarfTag(dbg, offset), name_(""), type_(nullptr) {}
  virtual ~DwarfVar() {
    if (type_) {
      delete type_;
      type_ = nullptr;
    }
    if (location_) {
      delete location_;
      location_ = nullptr;
    }
  }

  virtual bool load() override;
  virtual void dump() const override;

  virtual std::string name() const { return name_; }
  virtual DwarfType* type() const { return type_; }

  DwarfValue evalValue(const DwarfExpression::Context& context,
                       Dwarf_Addr pc) const;

 private:
  DwarfValue evalValueAtLoc(DwarfType* type, Dwarf_Addr addr,
                            DwarfExpression::MemoryProvider memory) const;
  DwarfValue formatValue(DwarfType* type, char* buf, size_t buf_size) const;

  DwarfType* loadType();
  DwarfLocation* loadLocation();

 private:
  std::string name_;
  DwarfType* type_;
  DwarfLocation* location_;

};  // class DwarfVar

}  // namespace dwarfexpr

#endif  // DWARFEXPR_DWARF_VARS_H
