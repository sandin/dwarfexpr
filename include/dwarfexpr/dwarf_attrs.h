#ifndef DWARFEXPR_DWARF_ATTRS_H
#define DWARFEXPR_DWARF_ATTRS_H

#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

#include <string>
#include <type_traits>

#include "dwarfexpr/dwarf_utils.h"

namespace dwarfexpr {

template <typename T>
std::enable_if_t<std::is_same<T, std::string>::value, std::string> getAttr(
    Dwarf_Attribute attr, T defVal) {
  return getAttrStr(attr, defVal);
}

template <typename T>
std::enable_if_t<std::is_same<T, Dwarf_Bool>::value, Dwarf_Bool> getAttr(
    Dwarf_Attribute attr, T defVal) {
  return getAttrFlag(attr, defVal);
}

template <typename T>
std::enable_if_t<std::is_same<T, Dwarf_Unsigned>::value, Dwarf_Unsigned>
getAttr(Dwarf_Attribute attr, T defVal) {
  return getAttrNumb(attr, defVal);
}

template <typename T>
T getAttrValue(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half attrNum, T defVal) {
  Dwarf_Error err;
  Dwarf_Attribute attr;
  if (dwarf_attr(die, attrNum, &attr, &err) == DW_DLV_OK) {
    auto type_attr_guard =
        make_scope_exit([&]() { dwarf_dealloc(dbg, attr, DW_DLA_ATTR); });
    return getAttr<T>(attr, defVal);
  }
  return defVal;
}

Dwarf_Off getAttrValueRef(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half attrNum,
                          Dwarf_Off defVal);
Dwarf_Addr getAttrValueAddr(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half attrNum,
                            Dwarf_Addr defVal);

}  // namespace dwarfexpr

#endif  // DWARFEXPR_DWARF_ATTRS_H
