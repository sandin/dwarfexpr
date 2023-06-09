#ifndef DWARFEXPR_DWARF_TYPES_H
#define DWARFEXPR_DWARF_TYPES_H

#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

#include <limits>
#include <string>

namespace dwarfexpr {

constexpr size_t MAX_SIZE = std::numeric_limits<size_t>::max();

class DwarfType {
 public:
  DwarfType(Dwarf_Debug dbg, Dwarf_Off offset)
      : dbg_(dbg),
        offset_(offset),
        tag_(0),
        die_(nullptr),
        base_type_(nullptr),
        valid_(false),
        name_("unknown"),
        size_(MAX_SIZE) {}
  virtual ~DwarfType() {
    if (die_) {
      dwarf_dealloc(dbg_, die_, DW_DLA_DIE);
    }
    if (base_type_) {
      delete base_type_;
    }
  }

  virtual bool load();
  virtual std::string name() const;
  virtual size_t size() const;

  Dwarf_Half tag() const { return tag_; }
  bool isValid() const { return valid_; }
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
  DwarfType* base_type_;
  bool valid_;
  std::string name_;
  Dwarf_Unsigned size_;  // in bytes

  // DW_TAG_class_type
  //  DW_AT_name
  //  DW_AT_declaration
  Dwarf_Bool declaration_ = false;
};  // class DwarfType

}  // namespace dwarfexpr

#endif  // DWARFEXPR_DWARF_TYPES_H
