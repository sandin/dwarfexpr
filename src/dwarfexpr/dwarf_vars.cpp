#include "dwarfexpr/dwarf_vars.h"

#include "dwarfexpr/dwarf_utils.h"

namespace dwarfexpr {

bool DwarfVar::load() {
  Dwarf_Error err = nullptr;
  if (!getDieFromOffset(dbg_, offset_, die_)) {
    return false;
  }

  if (dwarf_tag(die_, &tag_, &err) != DW_DLV_OK) {
    return false;
  }

  const char* tag_name = nullptr;
  dwarf_get_TAG_name(tag_, &tag_name);

  char* var_name;
  if (dwarf_diename(die_, &var_name, &err) != DW_DLV_OK) {
    return false;
  }
  name_ = var_name;

  type_ = loadType();
  if (!type_) {
    return false;
  }

  return true;
}

DwarfType* DwarfVar::loadType() {
  Dwarf_Error err = nullptr;
  Dwarf_Attribute type_attr;
  std::string type_name;
  if (dwarf_attr(die_, DW_AT_type, &type_attr, &err) == DW_DLV_OK) {
    auto type_attr_guard =
        make_scope_exit([&]() { dwarf_dealloc(dbg_, type_attr, DW_DLA_ATTR); });
    Dwarf_Off type_ref = getAttrGlobalRef(type_attr, MAX_DWARF_OFF);
    if (type_ref != MAX_DWARF_OFF) {
      DwarfType* type = new DwarfType(dbg_, type_ref);
      if (type->load()) {
        return type;
      }
      delete type;
    }
  }
  return nullptr;
}

};  // namespace dwarfexpr
