#include "dwarfexpr/dwarf_attrs.h"

namespace dwarfexpr {

Dwarf_Off getAttrValueRef(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half attrNum,
                          Dwarf_Off defVal) {
  Dwarf_Error err;
  Dwarf_Attribute attr;
  if (dwarf_attr(die, attrNum, &attr, &err) == DW_DLV_OK) {
    auto type_attr_guard =
        make_scope_exit([&]() { dwarf_dealloc(dbg, attr, DW_DLA_ATTR); });
    return getAttrGlobalRef(attr, defVal);
  }
  return defVal;
}

Dwarf_Addr getAttrValueAddr(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half attrNum,
                          Dwarf_Addr defVal) {
  Dwarf_Error err;
  Dwarf_Attribute attr;
  if (dwarf_attr(die, attrNum, &attr, &err) == DW_DLV_OK) {
    auto type_attr_guard =
        make_scope_exit([&]() { dwarf_dealloc(dbg, attr, DW_DLA_ATTR); });
    return getAttrAddr(attr, defVal);
  }
  return defVal;
}

};  // namespace dwarfexpr
