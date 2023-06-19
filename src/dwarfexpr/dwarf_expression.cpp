#include "dwarfexpr/dwarf_expression.h"

#include <cinttypes>
#include <cstdlib>  // abs
#include <stack>

#include "dwarfexpr/dwarf_location.h"
#include "dwarfexpr/dwarf_utils.h"

namespace dwarfexpr {

DwarfExpression::Result DwarfExpression::evaluate(const Context& context,
                                                  Dwarf_Addr pc) const {
  if (count() < 1) {
    return Result::Error("no operation in expression");
  }

  for (const DwarfOp& a : ops_) {
    if (a.opcode == DW_OP_piece) {
      return Result::Error("not impl DW_OP_piece");
      ;
    }
  }

  Dwarf_Small opcode = ops_[0].opcode;
  const char* opcode_name;
  if (dwarf_get_OP_name(opcode, &opcode_name) != DW_DLV_OK) {
    return Result::Error("can not get opcode name");
  }

  // 2.6.1.1.2 Register Location Descriptions
  if (DW_OP_reg0 <= opcode && opcode <= DW_OP_reg31) {
    if (context.registers == nullptr) {
      return Result::Error("can not access registers in context");
    }

    Dwarf_Half reg_num = opcode - DW_OP_reg0;
    uint64_t reg_val = context.registers(reg_num);
    printf("op=%s reg%d = 0x%" PRIx64 "\n", opcode_name, reg_num, reg_val);
    return Result::Value(reg_val);
  } else if (opcode == DW_OP_regx) {
    if (context.registers == nullptr) {
      return Result::Error("can not access registers in context");
    }

    Dwarf_Half reg_num = ops_[0].op1;
    uint64_t reg_val = context.registers(reg_num);
    printf("op=%s reg%d = 0x%" PRIx64 "\n", opcode_name, reg_num, reg_val);
    return Result::Value(reg_val);
  }

  std::stack<Dwarf_Signed> mystack;
  for (const DwarfOp& a : ops_) {
    if (dwarf_get_OP_name(opcode, &opcode_name) != DW_DLV_OK) {
      return Result::Error("can not get opcode name");
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
        if (context.frameBaseLoc == nullptr) {
          return Result::Error("frameBaseLoc can not be null");
        }

        Result frameBase = context.frameBaseLoc->evalValue(context, pc);
        if (!frameBase.valid()) {
          return Result::Error(frameBase.error_msg);
        }
        mystack.push(frameBase.value + a.op1);
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
        if (context.registers == nullptr) {
          return Result::Error("can not access registers in context");
        }
        Dwarf_Half reg_num = opcode - DW_OP_breg0;
        uint64_t reg_val = context.registers(reg_num);
        printf("reg%d(0x%" PRIx64 ") + 0x%llx = 0x%llx\n", reg_num, reg_val,
               a.op1, reg_val + a.op1);
        mystack.push(reg_val + a.op1);
        break;
      }
      case DW_OP_bregx: {
        if (context.registers == nullptr) {
          return Result::Error("can not access registers in context");
        }
        Dwarf_Half reg_num = a.op1;
        uint64_t reg_val = context.registers(reg_num);
        printf("reg%d(0x%" PRIx64 ") + 0x%llx = 0x%llx\n", reg_num, reg_val,
               a.op1, reg_val + a.op1);
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
          return Result::Error("stack is empty");
        }
        mystack.push(mystack.top());
        break;

      // Pops the value at the top of the stack
      case DW_OP_drop:
        if (mystack.empty()) {
          return Result::Error("stack is empty");
        }
        mystack.pop();
        break;

        // Entry with specified index is copied at the top.
      case DW_OP_pick: {
        Dwarf_Unsigned idx = a.op1;
        std::stack<Dwarf_Signed> t;
        if (mystack.size() < (idx + 1)) {
          return Result::Error("out of index");
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
          return Result::Error("stack size < 2");
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
          return Result::Error("stack size < 2");
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
          return Result();
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
        if (context.memory == nullptr) {
          return Result::Error("can not access memory in context");
        }

        if (mystack.empty()) {
          return Result::Error("stack is empty");
        }

        Dwarf_Addr adr = mystack.top();
        mystack.pop();

        char* buf = nullptr;
        size_t out_size = 0;
        if (!context.memory(adr, sizeof(Dwarf_Signed), &buf, &out_size)) {
          return Result::Error("can not read memory at address");
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
        if (context.memory == nullptr) {
          return Result::Error("can not access memory in context");
        }

        if (mystack.empty()) {
          return Result::Error("stack is empty");
        }

        Dwarf_Unsigned size = a.op1;
        if (size > sizeof(Dwarf_Signed)) {
          return Result::Error("unexpect size of op1");
        }

        Dwarf_Addr adr = mystack.top();
        mystack.pop();

        char* buf = nullptr;
        size_t buf_size = 0;
        if (!context.memory(adr, size, &buf, &buf_size)) {
          return Result::Error("can not read memory at address");
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
        if (context.cfa == nullptr) {
          return Result::Error("can not access cfa in context");
        }
        Dwarf_Addr addr = context.cfa(pc);
        mystack.push(addr);
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
          return Result::Error("stack is empty");
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
            break;
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
          return Result::Error("stack size < 2");
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
            break;
        }

        break;
      }

        //
        // Control Flow Operations.
        //

      case DW_OP_le: {
        Dwarf_Signed e1 = mystack.top();
        mystack.pop();
        Dwarf_Signed e2 = mystack.top();
        mystack.pop();

        mystack.push(e2 <= e1);
        break;
      }

      case DW_OP_ge: {
        Dwarf_Signed e1 = mystack.top();
        mystack.pop();
        Dwarf_Signed e2 = mystack.top();
        mystack.pop();

        mystack.push(e2 >= e1);
        break;
      }

      case DW_OP_eq: {
        Dwarf_Signed e1 = mystack.top();
        mystack.pop();
        Dwarf_Signed e2 = mystack.top();
        mystack.pop();

        mystack.push(e2 == e1);
        break;
      }

      case DW_OP_lt: {
        Dwarf_Signed e1 = mystack.top();
        mystack.pop();
        Dwarf_Signed e2 = mystack.top();
        mystack.pop();

        mystack.push(e2 < e1);
        break;
      }

      case DW_OP_gt: {
        Dwarf_Signed e1 = mystack.top();
        mystack.pop();
        Dwarf_Signed e2 = mystack.top();
        mystack.pop();

        mystack.push(e2 > e1);
        break;
      }

      case DW_OP_ne: {
        Dwarf_Signed e1 = mystack.top();
        mystack.pop();
        Dwarf_Signed e2 = mystack.top();
        mystack.pop();

        mystack.push(e2 != e1);
        break;
      }

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
          return Result::Error("stack is empty");
        }
        Dwarf_Addr value = mystack.top();
        return Result::Value(value);
      }

        //
        // Invalid or unrecognized operations.
        //
      default: {
        std::string err_msg = "not impl opcode: ";
        const char* op_name;
        if (dwarf_get_OP_name(a.opcode, &op_name) == DW_DLV_OK) {
          err_msg += op_name;
        }
        return Result::Error(err_msg);
      }
    }  // switch
  }    // for

  if (!mystack.empty()) {
    Dwarf_Addr value = mystack.top();
    return Result::Address(value);
  }

  return Result::Error("stack is empty");
}  // end of DwarfExpression::evaluate

void DwarfExpression::dump() const {
  for (const DwarfOp& a : ops_) {
    const char* name;
    dwarf_get_OP_name(a.opcode, &name);
    printf("op=%s, op1=0x%llx, op2=0x%llx, op3=0x%llx, off=0x%llx ", name,
           a.op1, a.op2, a.op3, a.off);
  }
}

};  // namespace dwarfexpr