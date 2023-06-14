#include "dwarfexpr/dwarf_location.h"

#include <stack>

#include "dwarfexpr/dwarf_utils.h"

namespace dwarfexpr {

/*  DW_FORM_data16 should not apply here. */
static bool is_simple_location_expr(int form) {
  if (form == DW_FORM_block1 || form == DW_FORM_block2 ||
      form == DW_FORM_block4 || form == DW_FORM_block ||
      form == DW_FORM_exprloc) {
    return true;
  }
  return false;
}
static bool is_location_form(int form) {
  if (form == DW_FORM_data4 || form == DW_FORM_data8 ||
      form == DW_FORM_sec_offset || form == DW_FORM_loclistx ||
      form == DW_FORM_rnglistx) {
    return true;
  }
  return false;
}

// static
DwarfLocation* DwarfLocation::loadFromDieAttr(Dwarf_Debug dbg, Dwarf_Die die,
                                              Dwarf_Half attrnum) {
  Dwarf_Error err = nullptr;
  // Get address size.
  Dwarf_Half addr_size = 0;
  if (dwarf_get_die_address_size(die, &addr_size, &err) != DW_DLV_OK) {
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
  if (dwarf_get_version_of_die(die, &version, &offset_size) != DW_DLV_OK) {
    return nullptr;
  }

  Dwarf_Attribute loc_attr;
  if (dwarf_attr(die, attrnum, &loc_attr, &err) == DW_DLV_OK) {
    DwarfLocation* loc = new DwarfLocation(dbg, /* move */ loc_attr, addr_size,
                                           offset_size, version);
    if (loc->load()) {
      return loc;
    }
    delete loc;  // dealloc loc_attr
  }
  return nullptr;
}

bool DwarfLocation::load() {
  Dwarf_Half form = 0;
  Dwarf_Error error = nullptr;
  if (dwarf_whatform(attr_, &form, &error) != DW_DLV_OK) {
    return false;
  }

  /*
  const char* form_name;
  if (dwarf_get_FORM_name(form, &form_name) != DW_DLV_OK) {
    return false;
  }
  printf("DW_AT_location: form_name=%s\n", form_name);
  */

  if (is_simple_location_expr(form)) {
    // Get expression location pointer.
    Dwarf_Unsigned retExprLen;     // Length of location expression.
    Dwarf_Ptr blockPtr = nullptr;  // Pointer to location expression.
    if (dwarf_formexprloc(attr_, &retExprLen, &blockPtr, &error) != DW_DLV_OK) {
      return false;
    }

    Dwarf_Unsigned cnt;  // number of list records
    Dwarf_Loc_Head_c loclist_head = nullptr;
    if (dwarf_loclist_from_expr_c(dbg_, blockPtr, retExprLen, addr_size_,
                                  offset_size_, version_, &loclist_head,
                                  &cnt,  // should be set to 1.
                                  &error) != DW_DLV_OK) {
      return false;
    }
    auto guard =
        make_scope_exit([&]() { dwarf_dealloc_loc_head_c(loclist_head); });

    if (cnt > 0) {
      loadLocDescEntry(loclist_head, 0);
    }
    return true;

  } else if (is_location_form(form)) {
    Dwarf_Unsigned cnt;  // number of list records
    Dwarf_Loc_Head_c loclist_head = nullptr;

    if (dwarf_get_loclist_c(attr_, &loclist_head, &cnt, &error) != DW_DLV_OK) {
      return false;
    }
    auto guard =
        make_scope_exit([&]() { dwarf_dealloc_loc_head_c(loclist_head); });

    for (Dwarf_Unsigned i = 0; i < cnt; ++i) {
      loadLocDescEntry(loclist_head, i);
    }
    return true;
  } else {
    return false;  // unsupport form
  }

  return true;
}

bool DwarfLocation::loadLocDescEntry(Dwarf_Loc_Head_c loclist_head,
                                     Dwarf_Unsigned idx) {
  Dwarf_Error error = nullptr;
  Dwarf_Small loclist_lkind = 0;
  Dwarf_Small lle_value = 0;
  Dwarf_Unsigned rawval1 = 0;
  Dwarf_Unsigned rawval2 = 0;
  Dwarf_Bool debug_addr_unavailable = false;
  Dwarf_Addr lopc = 0;
  Dwarf_Addr hipc = 0;
  Dwarf_Unsigned loclist_expr_op_count = 0;
  Dwarf_Locdesc_c locdesc_entry = 0;  // list of location descriptions
  Dwarf_Unsigned expression_offset = 0;
  Dwarf_Unsigned locdesc_offset = 0;

  if (dwarf_get_locdesc_entry_d(loclist_head, idx, &lle_value, &rawval1,
                                &rawval2, &debug_addr_unavailable, &lopc, &hipc,
                                &loclist_expr_op_count, &locdesc_entry,
                                &loclist_lkind, &expression_offset,
                                &locdesc_offset, &error) != DW_DLV_OK) {
    return false;
  }

  Expression expr;
  expr.lowAddr = lopc;
  expr.highAddr = hipc;
  // List of atoms in one expression.
  Dwarf_Small op = 0;
  for (int j = 0; j < static_cast<int>(loclist_expr_op_count); j++) {
    Dwarf_Unsigned opd1 = 0;
    Dwarf_Unsigned opd2 = 0;
    Dwarf_Unsigned opd3 = 0;
    Dwarf_Unsigned offsetforbranch = 0;

    if (dwarf_get_location_op_value_c(locdesc_entry, j, &op, &opd1, &opd2,
                                      &opd3, &offsetforbranch,
                                      &error) == DW_DLV_OK) {
      Atom a;
      a.opcode = op;
      a.op1 = opd1;
      a.op2 = opd2;
      a.op3 = opd3;
      a.off = offsetforbranch;

      expr.atoms.push_back(a);
    }
  }

  exprs_.emplace_back(expr);
  return true;
}

DwarfLocation::LocValue DwarfLocation::evalValue(Dwarf_Addr pc,
                                                 Dwarf_Addr cuLowAddr,
                                                 Dwarf_Addr cuHighAddr,
                                                 DwarfLocation* frameBaseLoc,
                                                 RegisterProvider registers,
                                                 MemoryProvider memory) const {
  for (const Expression& e : exprs_) {
    // Expression range is unlimited -> evaluate.
    if (e.lowAddr == 0 && e.highAddr == MAX_DWARF_UNSIGNED) {
      return evaluateExpression(e, pc, cuLowAddr, cuHighAddr, frameBaseLoc,
                                registers, memory);
    } else {
      // We have got program counter, check if it is in range of the expression.
      // CUs lowpc is base for expression's range.
      Dwarf_Addr low = e.lowAddr + cuLowAddr;
      Dwarf_Addr high = e.highAddr + cuHighAddr;

      if ((pc >= low) && (pc < high)) {
        printf("evaluateExpression [0x%llx - 0x%llx] pc=0x%llx\n", low, high,
               pc);
        return evaluateExpression(e, pc, cuLowAddr, cuHighAddr, frameBaseLoc,
                                  registers, memory);
      }
    }
  }
  printf(
      "Error: unable to find the target pc in the address range, pc=0x%llx\n",
      pc);
  return LocValue();  // invalid location
}

// ref: https://dwarfstd.org/doc/040408.1.html
DwarfLocation::LocValue DwarfLocation::evaluateExpression(
    const Expression& expr, Dwarf_Addr pc, Dwarf_Addr cuLowAddr,
    Dwarf_Addr cuHighAddr, DwarfLocation* frameBaseLoc,
    RegisterProvider registers, MemoryProvider memory) const {
  if (expr.count() < 1) {
    printf("Error: no atoms in expr\n");
    return LocValue();
    ;
  }

  for (const Atom& a : expr.atoms) {
    if (a.opcode == DW_OP_piece) {
      printf("Error: not impl DW_OP_piece\n");
      return LocValue();
      ;
    }
  }

  Dwarf_Small opcode = expr.atoms[0].opcode;
  const char* opcode_name;
  if (dwarf_get_OP_name(opcode, &opcode_name) != DW_DLV_OK) {
    printf("Error: can not get opcode name\n");
    return LocValue();
    ;
  }

  if (DW_OP_reg0 <= opcode && opcode <= DW_OP_reg31) {
    Dwarf_Half reg_num = opcode - DW_OP_reg0;
    uint64_t reg_val = registers(reg_num);
    printf("op=%s reg%d = 0x%lx\n", opcode_name, reg_num, reg_val);
    return LocValue{LocValue::Type::kAddress, {reg_val}};
  } else if (opcode == DW_OP_regx) {
    Dwarf_Half reg_num = expr.atoms[0].op1;
    uint64_t reg_val = registers(reg_num);
    printf("op=%s reg%d = 0x%lx\n", opcode_name, reg_num, reg_val);
    return LocValue{LocValue::Type::kAddress, {reg_val}};
  }

  std::stack<Dwarf_Signed> mystack;
  for (const Atom& a : expr.atoms) {
    if (dwarf_get_OP_name(opcode, &opcode_name) != DW_DLV_OK) {
      printf("Error: can not get opcode name\n");
      return LocValue();
    }
    printf("stack: op=%s, op1=0x%llx, op2=0x%llx, op3=0x%llx, off=0x%llx\n",
           opcode_name, a.op1, a.op2, a.op3, a.off);

    switch (a.opcode) {
        //
        // Literal Encodings.
        // Push a value onto the DWARF stack.
        //

        // Literals.
      case DW_OP_lit0:
      case DW_OP_lit1:
      case DW_OP_lit2:
      case DW_OP_lit3:
      case DW_OP_lit4:
      case DW_OP_lit5:
      case DW_OP_lit6:
      case DW_OP_lit7:
      case DW_OP_lit8:
      case DW_OP_lit9:
      case DW_OP_lit10:
      case DW_OP_lit11:
      case DW_OP_lit12:
      case DW_OP_lit13:
      case DW_OP_lit14:
      case DW_OP_lit15:
      case DW_OP_lit16:
      case DW_OP_lit17:
      case DW_OP_lit18:
      case DW_OP_lit19:
      case DW_OP_lit20:
      case DW_OP_lit21:
      case DW_OP_lit22:
      case DW_OP_lit23:
      case DW_OP_lit24:
      case DW_OP_lit25:
      case DW_OP_lit26:
      case DW_OP_lit27:
      case DW_OP_lit28:
      case DW_OP_lit29:
      case DW_OP_lit30:
      case DW_OP_lit31:
        mystack.push(a.opcode - DW_OP_lit0);
        break;

        // First operand pushed to stack.
        // Signed and unsigned together.
      case DW_OP_addr:
      case DW_OP_const1u:
      case DW_OP_const1s:
      case DW_OP_const2u:
      case DW_OP_const2s:
      case DW_OP_const4u:
      case DW_OP_const4s:
      case DW_OP_const8u:
      case DW_OP_const8s:
      case DW_OP_constu:
      case DW_OP_consts:
        mystack.push(a.op1);
        break;

        //
        // Register Based Addressing.
        // Pushed value is result of adding the contents of a register
        // with a given signed offset.
        //

        // Frame base plus signed first operand.
      case DW_OP_fbreg: {
        if (frameBaseLoc == nullptr) {
          printf("Error: frameBaseLoc can not be null.\n");
          return LocValue();
        }

        LocValue frameBase = frameBaseLoc->evalValue(
            pc, cuLowAddr, cuHighAddr, frameBaseLoc, registers, memory);
        if (frameBase.type == LocValue::Type::kInvalid) {
          printf("Error: can not eval value of frameBaseLoc.\n");
          return LocValue();
        }
        mystack.push(frameBase.value.addr + a.op1);
        break;
      }

        // Content of register (address) plus signed first operand.
      case DW_OP_breg0:
      case DW_OP_breg1:
      case DW_OP_breg2:
      case DW_OP_breg3:
      case DW_OP_breg4:
      case DW_OP_breg5:
      case DW_OP_breg6:
      case DW_OP_breg7:
      case DW_OP_breg8:
      case DW_OP_breg9:
      case DW_OP_breg10:
      case DW_OP_breg11:
      case DW_OP_breg12:
      case DW_OP_breg13:
      case DW_OP_breg14:
      case DW_OP_breg15:
      case DW_OP_breg16:
      case DW_OP_breg17:
      case DW_OP_breg18:
      case DW_OP_breg19:
      case DW_OP_breg20:
      case DW_OP_breg21:
      case DW_OP_breg22:
      case DW_OP_breg23:
      case DW_OP_breg24:
      case DW_OP_breg25:
      case DW_OP_breg26:
      case DW_OP_breg27:
      case DW_OP_breg28:
      case DW_OP_breg29:
      case DW_OP_breg30:
      case DW_OP_breg31: {
        Dwarf_Half reg_num = opcode - DW_OP_breg0;
        uint64_t reg_val = registers(reg_num);
        printf("reg%d(0x%lx) + 0x%llx = 0x%llx\n", reg_num, reg_val, a.op1,
               reg_val + a.op1);
        mystack.push(reg_val + a.op1);
        break;
      }
      case DW_OP_bregx: {
        Dwarf_Half reg_num = expr.atoms[0].op1;
        uint64_t reg_val = registers(reg_num);
        printf("reg%d(0x%lx) + 0x%llx = 0x%llx\n", reg_num, reg_val, a.op1,
               reg_val + a.op1);
        mystack.push(reg_val + a.op1);
        break;
      }

        //
        // Stack Operations.
        // Operations manipulate the DWARF stack.
        //

        // TODO

        //
        // Object does not exist in memory but its value is known and it is at
        // the top of the DWARF expression stack. DWARF expression represents
        // actual value of the object, rather then its location.
        // DW_OP_stack_alue operation terminates the expression.
        //
      case DW_OP_stack_value: {
        if (mystack.empty()) {
          return LocValue();
        }
        Dwarf_Addr value = mystack.top();
        return LocValue{LocValue::Type::kValue, {value}};
      }

      default: {
        const char* op_name;
        if (dwarf_get_OP_name(a.opcode, &op_name) == DW_DLV_OK) {
          printf("Error: not impl opcode: %s\n", op_name);
        }
        return LocValue();
      }
    }  // switch
  }    // for

  if (!mystack.empty()) {
    Dwarf_Addr value = mystack.top();
    return LocValue{LocValue::Type::kAddress, {value}};
  }

  return LocValue();
}

void DwarfLocation::dump() const {
  for (const Expression& expr : exprs_) {
    printf("\t[0x%llx - 0x%llx): ", expr.lowAddr, expr.highAddr);

    for (const Atom& a : expr.atoms) {
      const char* name;
      dwarf_get_OP_name(a.opcode, &name);

      printf("op=%s, op1=0x%llx, op2=0x%llx, op3=0x%llx, off=0x%llx ", name,
             a.op1, a.op2, a.op3, a.off);
    }
    printf("\n");
  }
}

};  // namespace dwarfexpr
