#include "dwarfexpr/dwarf_location.h"

#include <stack>
#include <cstdlib> // abs

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
    return LocValue{LocValue::Type::kValue, {reg_val}};
  } else if (opcode == DW_OP_regx) {
    Dwarf_Half reg_num = expr.atoms[0].op1;
    uint64_t reg_val = registers(reg_num);
    printf("op=%s reg%d = 0x%lx\n", opcode_name, reg_num, reg_val);
    return LocValue{LocValue::Type::kValue, {reg_val}};
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

        // Duplicates the value at the top of the stack.
      case DW_OP_dup:
        if (mystack.empty()) {
          return LocValue();
        }
        mystack.push(mystack.top());
        break;

      // Pops the value at the top of the stack
      case DW_OP_drop:
        if (mystack.empty()) {
          return LocValue();
        }
        mystack.pop();
        break;

        // Entry with specified index is copied at the top.
      case DW_OP_pick: {
        Dwarf_Unsigned idx = a.op1;
        std::stack<Dwarf_Signed> t;
        if (mystack.size() < (idx + 1)) {
          // out of index
          return LocValue();
        }

        for (unsigned i = 0; i < idx; i++) {
          t.push(mystack.top());
          mystack.pop();
        }

        Dwarf_Signed pick = mystack.top();

        for (unsigned i = 0; i < idx; i++) {
          mystack.push(t.top());
          t.pop();
        }

        mystack.push(pick);
        break;
      }

        // Duplicates the second entry to the top of the stack.
      case DW_OP_over: {
        if (mystack.size() < 2) {
          return LocValue();
        }

        Dwarf_Signed t = mystack.top();
        mystack.pop();
        Dwarf_Signed d = mystack.top();

        mystack.push(t);
        mystack.push(d);
        break;
      }

        // Swaps the top two stack entries.
      case DW_OP_swap: {
        if (mystack.size() < 2) {
          return LocValue();
        }

        Dwarf_Signed e1 = mystack.top();
        mystack.pop();
        Dwarf_Signed e2 = mystack.top();
        mystack.pop();

        mystack.push(e1);
        mystack.push(e2);
        break;
      }

        // Rotates the first three stack entries
      case DW_OP_rot: {
        if (mystack.size() < 3) {
          return LocValue();
        }

        Dwarf_Signed e1 = mystack.top();
        mystack.pop();
        Dwarf_Signed e2 = mystack.top();
        mystack.pop();
        Dwarf_Signed e3 = mystack.top();
        mystack.pop();

        mystack.push(e1);
        mystack.push(e3);
        mystack.push(e2);
        break;
      }

        // Pops the top stack entry and treats it as an address.
        // The value retrieved from that address is pushed.
      case DW_OP_deref: {
        if (mystack.empty()) {
          return LocValue();
        }

        Dwarf_Addr adr = mystack.top();
        mystack.pop();

        char* buf = nullptr;
        size_t out_size = 0;
        if (!memory(adr, sizeof(Dwarf_Signed), &buf, &out_size)) {
          printf("Error: can not read memory at: 0x%llx\n", adr);
          return LocValue();
        }

        Dwarf_Signed deref_val = *(reinterpret_cast<Dwarf_Signed*>(buf));
        mystack.push(deref_val);
        break;
      }

        // The DW_OP_deref_size operation behaves like the DW_OP_deref
        // operation: it pops the top stack entry and treats it as an address.
        // The value retrieved from that address is pushed. In the
        // DW_OP_deref_size operation, however, the size in bytes of the data
        // retrieved from the dereferenced address is specified by the single
        // operand. This operand is a 1-byte unsigned integral constant whose
        // value may not be larger than the size of an address on the target
        // machine. The data retrieved is zero extended to the size of an
        // address on the target machine before being pushed on the expression
        // stack.
      case DW_OP_deref_size: {
        if (mystack.empty()) {
          return LocValue();
        }

        Dwarf_Unsigned size = a.op1;
        if (size > sizeof(Dwarf_Signed)) {
          return LocValue();
        }

        Dwarf_Addr adr = mystack.top();
        mystack.pop();

        char* buf = nullptr;
        size_t buf_size = 0;
        if (!memory(adr, size, &buf, &buf_size)) {
          printf("Error: can not read memory at: 0x%llx\n", adr);
          return LocValue();
        }

        Dwarf_Signed deref_val = 0;
        char* defref_val_ptr = reinterpret_cast<char*>(&deref_val);
        for (size_t i = 0; i < sizeof(Dwarf_Signed); i++) {
          *(defref_val_ptr + i) = i < buf_size ? buf[i] : 0;
        }
        mystack.push(deref_val);
        break;
      }

        // TODO: case DW_OP_xderef:
        // TODO: case DW_OP_xderef_size:
        // TODO: case DW_OP_push_object_address:
        // TODO: case DW_OP_form_tls_address:

        // The DW_OP_call_frame_cfa operation pushes the value of the CFA,
        // obtained from the Call Frame Information (see Section 6.4).
      case DW_OP_call_frame_cfa: {
        // TODO: CFA
        break;
      }

        //
        // Arithmetic and Logical Operations.
        // The arithmetic operations perform addressing arithmetic, that is,
        // unsigned arithmetic that wraps on an address-sized boundary.
        //

        // Operates on top entry.
      case DW_OP_abs:
      case DW_OP_neg:
      case DW_OP_not:
      case DW_OP_plus_uconst: {
        if (mystack.empty()) {
          return LocValue();
        }
        Dwarf_Signed top = mystack.top();
        mystack.pop();

        switch (a.opcode) {
          // Replace top with it's absolute value.
          case DW_OP_abs:
            mystack.push(std::abs(top));
            break;

          // Negate top.
          case DW_OP_neg:
            mystack.push(-top);
            break;

          // Bitwise complement of the top.
          case DW_OP_not:
            mystack.push(~top);
            break;

          // Top value plus unsigned first operand.
          case DW_OP_plus_uconst:
            mystack.push(top + a.op1);
            break;

          // Should not happen.
          default:
            return LocValue();
        }

        break;
      }

        // Operates on top two entries.
      case DW_OP_and:
      case DW_OP_div:
      case DW_OP_minus:
      case DW_OP_mod:
      case DW_OP_mul:
      case DW_OP_or:
      case DW_OP_plus:
      case DW_OP_shl:
      case DW_OP_shr:
      case DW_OP_shra:
      case DW_OP_xor: {
        if (mystack.size() < 2) {
          return LocValue();
        }
        Dwarf_Signed e1 = mystack.top();
        mystack.pop();
        Dwarf_Signed e2 = mystack.top();
        mystack.pop();

        switch (a.opcode) {
          // Bitwise and on top 2 values.
          case DW_OP_and:
            mystack.push(e1 & e2);
            break;

          // Second div first from top (signed division).
          case DW_OP_div:
            mystack.push(e2 / e1);
            break;

          // Second minus first from top.
          case DW_OP_minus:
            mystack.push(e2 - e1);
            break;

          // Second modulo first from top.
          case DW_OP_mod:
            mystack.push(e2 % e1);
            break;

          // Second times first from top.
          case DW_OP_mul:
            mystack.push(e2 * e1);
            break;

          // Bitwise or of top 2 entries.
          case DW_OP_or:
            mystack.push(e2 | e1);
            break;

          // Adds together top two entries.
          case DW_OP_plus:
            mystack.push(e2 + e1);
            break;

          // Shift second entry to left by first entry.
          case DW_OP_shl:
            mystack.push(e2 << e1);
            break;

          // Shift second entry to right by first entry.
          case DW_OP_shr:
            mystack.push(e2 >> e1);
            break;

          // Shift second entry arithmetically to right by first entry.
          case DW_OP_shra:
            mystack.push(e2 >> e1);
            break;

          // Bitwise XOR on top two entries.
          case DW_OP_xor:
            mystack.push(e2 ^ e1);
            break;

          // Should not happen.
          default:
            return LocValue();
        }

        break;
      }

        //
        // Control Flow Operations.
        //

        // TODO: case DW_OP_le:
        // TODO: case DW_OP_ge:
        // TODO: case DW_OP_eq:
        // TODO: case DW_OP_lt:
        // TODO: case DW_OP_gt:
        // TODO: case DW_OP_ne:
        // TODO: case DW_OP_skip:
        // TODO: case DW_OP_bra:
        // TODO: case DW_OP_call2:
        // TODO: case DW_OP_call4:
        // TODO: case DW_OP_call_ref:

        //
        // Implicit Location Descriptions.
        //

        // TODO: case DW_OP_implicit_value:
        // TODO: case DW_OP_piece:
        // TODO: case DW_OP_bit_piece:

        //
        // Special Operations.
        //

      // This has no effect.
      case DW_OP_nop:
        break;

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

        //
        // Invalid or unrecognized operations.
        //
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
}  // namespace dwarfexpr

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
