#ifndef DWARFEXPR_DWARF_LOCATION_H
#define DWARFEXPR_DWARF_LOCATION_H

#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

#include <string>
#include <vector>

#include "dwarfexpr/context.h"
#include "dwarfexpr/dwarf_expression.h"

namespace dwarfexpr {

// DW_AT_location
// DW_AT_data_member_location
// DW_AT_frame_base
class DwarfLocation {
 public:
  struct LocationExpression {
    Dwarf_Addr lowAddr;   // Lowest address of active range.
    Dwarf_Addr highAddr;  // Highest address of active range.
    DwarfExpression expr;
  };

  DwarfLocation(Dwarf_Debug dbg, Dwarf_Attribute attr, Dwarf_Half addr_size,
                Dwarf_Half offset_size, Dwarf_Half version)
      : dbg_(dbg),
        attr_(attr),
        addr_size_(addr_size),
        offset_size_(offset_size),
        version_(version) {}
  virtual ~DwarfLocation() {
    if (dbg_ && attr_) {
      dwarf_dealloc(dbg_, attr_, DW_DLA_ATTR);
    }
  }

  static DwarfLocation* loadFromDieAttr(Dwarf_Debug dbg, Dwarf_Die die,
                                        Dwarf_Half attrnum);

  virtual bool load();
  virtual void dump() const;

  DwarfExpression::Result evalValue(const DwarfExpression::Context& context,
                                    Dwarf_Addr pc) const;

 private:
  bool loadLocDescEntry(Dwarf_Loc_Head_c loclist_head, Dwarf_Unsigned idx);

 protected:
  Dwarf_Debug dbg_;
  Dwarf_Attribute attr_;
  std::vector<LocationExpression> exprs_;
  Dwarf_Half addr_size_;
  Dwarf_Half offset_size_;
  Dwarf_Half version_;
};  // class DwarfLocation

}  // namespace dwarfexpr

#endif  // DWARFEXPR_DWARF_LOCATION_H
