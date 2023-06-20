#ifndef DWARFEXPR_DWARF_FRAMES_H
#define DWARFEXPR_DWARF_FRAMES_H

#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

#include "dwarfexpr/dwarf_expression.h"

namespace dwarfexpr {

// 7.23 Call Frame Information
class DwarfFrames {
 public:
  struct FdeList {
    Dwarf_Fde* fde_data = nullptr;
    Dwarf_Cie* cie_data = nullptr;
    Dwarf_Signed cie_element_count = 0;
    Dwarf_Signed fde_element_count = 0;
  };

  struct FdeInfo {
    Dwarf_Unsigned reg;
    Dwarf_Unsigned offset_relevant;
    Dwarf_Small value_type;
    Dwarf_Unsigned offset;
    Dwarf_Block block;
    Dwarf_Addr row_pc;
    Dwarf_Bool has_more_rows;
    Dwarf_Addr subsequent_pc;

    Dwarf_Addr cfa;
  };

  DwarfFrames(Dwarf_Debug dbg, Dwarf_Half addr_size, Dwarf_Half offset_size,
              Dwarf_Half version)
      : dbg_(dbg),
        addr_size_(addr_size),
        offset_size_(offset_size),
        version_(version) {}
  ~DwarfFrames() {}

  Dwarf_Addr GetCfa(const DwarfExpression::Context& context,
                    Dwarf_Addr pc) const;

 private:
  Dwarf_Addr GetReg(const DwarfExpression::Context& context, Dwarf_Half i,
                    Dwarf_Addr pc, const FdeInfo& info) const;
  Dwarf_Addr EvalExpr(const DwarfExpression::Context& context, Dwarf_Half i,
                      Dwarf_Addr pc, const FdeInfo& info) const;

  bool GetAllFde(FdeList* fde_list) const;

  Dwarf_Debug dbg_;
  Dwarf_Half addr_size_;
  Dwarf_Half offset_size_;
  Dwarf_Half version_;
};  // class DwarfFrames

}  // namespace dwarfexpr

#endif  // DWARFEXPR_DWARF_FRAMES_H
