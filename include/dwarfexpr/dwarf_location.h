#ifndef DWARFEXPR_DWARF_LOCATION_H
#define DWARFEXPR_DWARF_LOCATION_H

#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

#include <string>
#include <vector>

#include "dwarfexpr/context.h"

namespace dwarfexpr {

// DW_AT_location
// DW_AT_data_member_location
// DW_AT_frame_base
class DwarfLocation {
 public:
  struct LocValue {
    enum class Type {
      kInvalid = 0,
      kAddress,
      kValue
    };
    union Value {
      Dwarf_Addr addr;
      Dwarf_Signed value;
    };

    Type type;
    Value value;
  };

  /**
   * @brief Basic unit of location description.
   */
  struct Atom {
    Dwarf_Small opcode;  ///< Operation code.
    Dwarf_Unsigned op1;  ///< Operand #1.
    Dwarf_Unsigned op2;  ///< Operand #2.
    Dwarf_Unsigned op3;  ///< Operand #3.
    Dwarf_Unsigned off;  ///< Offset in locexpr used in OP_BRA.
  };

  /**
   * @brief DWARF expression.
   */
  class Expression {
   public:
    Dwarf_Addr lowAddr;       ///< Lowest address of active range.
    Dwarf_Addr highAddr;      ///< Highest address of active range.
    std::vector<Atom> atoms;  ///< Vector of expression's atoms.
    std::size_t count() const { return atoms.size(); }
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

  LocValue evalValue(Dwarf_Addr pc, Dwarf_Addr cuLowAddr,
                      Dwarf_Addr cuHighAddr, DwarfLocation* frameBaseLoc,
                      RegisterProvider registers, MemoryProvider memory) const;

 private:
  bool loadLocDescEntry(Dwarf_Loc_Head_c loclist_head, Dwarf_Unsigned idx);

  LocValue evaluateExpression(const Expression& expr, Dwarf_Addr pc,
                               Dwarf_Addr cuLowAddr, Dwarf_Addr cuHighAddr,
                               DwarfLocation* frameBaseLoc,
                               RegisterProvider registers,
                               MemoryProvider memory) const;

 protected:
  Dwarf_Debug dbg_;
  Dwarf_Attribute attr_;
  std::vector<Expression> exprs_;
  Dwarf_Half addr_size_;
  Dwarf_Half offset_size_;
  Dwarf_Half version_;
};  // class DwarfLocation

}  // namespace dwarfexpr

#endif  // DWARFEXPR_DWARF_LOCATION_H
