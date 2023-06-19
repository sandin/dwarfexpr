#include "dwarfexpr/dwarf_vars.h"

#include <algorithm>  // std::min
#include <sstream>

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
  return DwarfLocation::loadFromDieAttr(dbg_, die_, DW_AT_location);
}

DwarfVar::DwarfValue DwarfVar::evalValue(
    const DwarfExpression::Context& context, Dwarf_Addr pc) const {
  if (location_) {
    DwarfExpression::Result loc = location_->evalValue(context, pc);
    if (loc.type == DwarfExpression::Result::Type::kValue) {
      return formatValue(type_, reinterpret_cast<char*>(&loc.value),
                         sizeof(Dwarf_Signed));
    } else if (loc.type == DwarfExpression::Result::Type::kAddress) {
      return evalValueAtLoc(type_, loc.value, context.memory);
    }  // else loc.type == DwarfExpression::Result::Type::kInvalid
  }
  return "unknown";
}

DwarfVar::DwarfValue DwarfVar::evalValueAtLoc(DwarfType* type, Dwarf_Addr addr,
                                              DwarfExpression::MemoryProvider memory) const {
  char* buf = nullptr;
  size_t buf_size = 0;
  if (addr != 0) {
    size_t size = type->size();
    bool found = memory(addr, size, &buf, &buf_size);
    if (!found) {
      printf("Error: can not read memory at addr: 0x%llx, size=0x%zx\n", addr,
             size);
      std::stringstream ss;
      ss << "unknown(addr=" << std::hex << addr << ")";
      return ss.str();
    }
  }
  return formatValue(type, buf, buf_size);
}

DwarfVar::DwarfValue DwarfVar::formatValue(DwarfType* type, char* buf,
                                           size_t buf_size) const {
  if (type->tag() == DW_TAG_pointer_type) {
    if (buf == nullptr) {
      return "nullptr";
    }
    uint64_t ptr_val =
        *reinterpret_cast<uint64_t*>(buf);  // TODO: little endian
    if (ptr_val == 0) {
      return "nullptr";
    }
    std::stringstream ss;
    ss << "0x" << std::hex << ptr_val;
    return ss.str();
  }

  if (buf == nullptr) {
    return "0";
  }
  return hexstring(buf, std::min(buf_size, type->size()));
}

void DwarfVar::dump() const {
  this->DwarfTag::dump();

  printf("name: %s\n", name_.c_str());
  if (type_) {
    printf("type:\n");
    type_->dump();
  }
  if (location_) {
    printf("location:\n");
    location_->dump();
  }
}

};  // namespace dwarfexpr