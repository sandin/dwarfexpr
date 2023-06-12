#include "dwarfexpr/dwarf_vars.h"

#include "dwarfexpr/dwarf_utils.h"

namespace dwarfexpr {

bool DwarfVar::load() {
  if (!this->DwarfTag::load()) {
    return false;
  }

  Dwarf_Error err = nullptr;
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

  printf("load location for tag 0x%llx\n", offset_);
  location_ = loadLocation();
  if (!location_) {
    printf("Warning: can not find DW_AT_location attr for tag [0x%llx %s] %s\n",
           offset_, tag_name, var_name);
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

DwarfLocation* DwarfVar::loadLocation() {
  Dwarf_Error err = nullptr;
  // Get address size.
  Dwarf_Half addr_size = 0;
  if (dwarf_get_die_address_size(die_, &addr_size, &err) != DW_DLV_OK) {
    return nullptr;
  }

  // Get offset size
  Dwarf_Half offset_size = 0;
  /*
  if (dwarf_get_offset_size(dbg_, &offset_size, &err) != DW_DLV_OK) {
    return nullptr;
  }
  */
  Dwarf_Half version = 2;
  if (dwarf_get_version_of_die(die_, &version, &offset_size) != DW_DLV_OK) {
    return nullptr;
  }

  Dwarf_Attribute loc_attr;
  if (dwarf_attr(die_, DW_AT_location, &loc_attr, &err) == DW_DLV_OK) {
    DwarfLocation* loc = new DwarfLocation(dbg_, /* move */ loc_attr, addr_size, offset_size, version);
    if (loc->load()) {
      return loc;
    }
    delete loc;  // dealloc loc_attr
  }
  return nullptr;
}

};  // namespace dwarfexpr
