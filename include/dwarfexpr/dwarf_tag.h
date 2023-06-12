#ifndef DWARFEXPR_DWARF_TAG_H
#define DWARFEXPR_DWARF_TAG_H

#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

#include <limits>
#include <string>

namespace dwarfexpr {

class DwarfTag {
 public:
  DwarfTag(Dwarf_Debug dbg, Dwarf_Off offset)
      : dbg_(dbg),
        offset_(offset),
        tag_(0),
        die_(nullptr) {}
  virtual ~DwarfTag() {
    if (die_) {
      dwarf_dealloc(dbg_, die_, DW_DLA_DIE);
    }
  }

  virtual bool load();

  Dwarf_Half tag() const { return tag_; }
  Dwarf_Off offset() const { return offset_; }
  std::string tagName() const {
    const char* tag_name = nullptr;
    dwarf_get_TAG_name(tag_, &tag_name);
    return tag_name;
  }

 protected:
  Dwarf_Debug dbg_;
  Dwarf_Off offset_;
  Dwarf_Half tag_;
  Dwarf_Die die_;
};  // class DwarfTag

}  // namespace dwarfexpr

#endif  // DWARFEXPR_DWARF_TAG_H
