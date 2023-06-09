#include "dwarfexpr/dwarf_tag.h"

#include "dwarfexpr/dwarf_utils.h"

namespace dwarfexpr {

bool DwarfTag::load() {
  Dwarf_Error err = nullptr;
  if (!getDieFromOffset(dbg_, offset_, die_)) {
    return false;
  }

  if (dwarf_tag(die_, &tag_, &err) != DW_DLV_OK) {
    return false;
  }

  return true;
}

void DwarfTag::dump() const {
  printf("0x%llx: %s\n", offset_, tagName().c_str());
}

};  // namespace dwarfexpr
