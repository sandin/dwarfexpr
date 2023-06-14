#ifndef DWARFEXPR_DWARF_TYPES_H
#define DWARFEXPR_DWARF_TYPES_H

#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

#include <limits>
#include <string>

#include "dwarfexpr/dwarf_tag.h"

namespace dwarfexpr {

constexpr size_t MAX_SIZE = std::numeric_limits<size_t>::max();

class DwarfType : public DwarfTag {
 public:
  DwarfType(Dwarf_Debug dbg, Dwarf_Off offset)
      : DwarfTag(dbg, offset),
        base_type_(nullptr),
        valid_(false),
        name_("unknown"),
        size_(MAX_SIZE) {}
  virtual ~DwarfType() {
    if (base_type_) {
      delete base_type_;
    }
  }

  virtual bool load() override;
  virtual void dump() const override;

  virtual std::string name() const;
  virtual size_t size() const;

 protected:
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
