#include "dwarfexpr/dwarf_types.h"

#include "dwarfexpr/dwarf_attrs.h"
#include "dwarfexpr/dwarf_utils.h"

namespace dwarfexpr {

bool DwarfType::load() {
  if (!this->DwarfTag::load()) {
    return false;
  }

  const char* tag_name = nullptr;
  dwarf_get_TAG_name(tag_, &tag_name);

  name_ = getAttrValue(dbg_, die_, DW_AT_name, name_);
  // printf("\t\t0x%llx %s, DW_AT_name: %s\n", offset_, tag_name,
  // name_.c_str());

  Dwarf_Unsigned bitSz =
      getAttrValue(dbg_, die_, DW_AT_bit_size, MAX_DWARF_UNSIGNED);
  if (bitSz != MAX_DWARF_UNSIGNED) {
    size_ = bitSz /= 8;  // TODO
  } else {
    size_ = getAttrValue(dbg_, die_, DW_AT_byte_size, MAX_DWARF_UNSIGNED);
  }

  switch (tag_) {
    case DW_TAG_base_type: {
      // TODO:
      // DW_AT_name	("int")
      // DW_AT_encoding	(DW_ATE_signed)
      // DW_AT_byte_size	(0x04)
      return true;
    }
    case DW_TAG_enumeration_type: {
      // TODO:
      // DW_AT_type	(0x0665c006 "uint8")
      // DW_AT_enum_class	(true)
      return true;
    }
    // TODO: case DW_TAG_array_type:  // passthrough
    case DW_TAG_typedef:  // passthrough
    case DW_TAG_pointer_type: {
      size_ = 8;  // TODO: 32-bit/64-bit
      Dwarf_Off type_ref =
          getAttrValueRef(dbg_, die_, DW_AT_type, MAX_DWARF_OFF);
      if (type_ref != MAX_DWARF_OFF) {
        DwarfType* base_type = new DwarfType(dbg_, type_ref);
        if (base_type->load()) {
          base_type_ = base_type;
          valid_ = true;
          return true;
        }
        delete base_type;
      }
      return false;
    }
    case DW_TAG_structure_type: {
      // TODO:
      // fields
      return true;
    }
    case DW_TAG_class_type:
      declaration_ = getAttrValue(dbg_, die_, DW_AT_declaration,
                                  static_cast<Dwarf_Bool>(false));
      // TODO:
      // members
      return true;
    default:
      printf("Error: unsupport type tag: 0x%llx %s(%d)\n", offset_, tag_name,
             tag_);
      break;
  }
  return false;
}

std::string DwarfType::name() const {
  // const char* tag_name = nullptr;
  // dwarf_get_TAG_name(tag_, &tag_name);

  switch (tag_) {
    case DW_TAG_pointer_type:
      if (base_type_ != nullptr) {
        std::string base_type_name = base_type_->name();
        return base_type_name + "*";
      } else {
        return "void*";
      }
    case DW_TAG_typedef:           // passthrough
    case DW_TAG_class_type:        // passthrough
    case DW_TAG_structure_type:    // passthrough
    case DW_TAG_enumeration_type:  // passthrough
    case DW_TAG_base_type:         // passthrough
    default:
      // printf("%s: toString(): %s\n", tag_name, name_.c_str());
      return name_;
  }
}

size_t DwarfType::size() const {
  switch (tag_) {
    case DW_TAG_typedef: {
      DwarfType* cur = base_type_;
      while (cur != nullptr) {
        if (cur->size() != MAX_SIZE) {
          return cur->size();
        }
        cur = cur->base_type_;
      }
      return MAX_SIZE;
    }
    case DW_TAG_pointer_type:  // passthrough
    default:
      return size_;
  }
}

};  // namespace dwarfexpr
