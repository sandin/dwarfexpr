#include "dwarfexpr/dwarf_expression.h"

#include <gtest/gtest.h>

#include <numeric>
#include <vector>

namespace dwarfexpr {

using Result = DwarfExpression::Result;
using ErrorCode = DwarfExpression::ErrorCode;
using Context = DwarfExpression::Context;

#define OP3(opcode, op1, op2, op3, off) \
  { opcode, .op1, op2, op3, off }
#define OP2(opcode, op1, op2, off) \
  { opcode, op1, op2, 0, off }
#define OP1(opcode, op1, off) \
  { opcode, op1, 0, 0, off }
#define OP(opcode, off) \
  { opcode, 0, 0, 0, off }

#define ILLEGAL_OP_CODE 0x00

template <typename T>
static std::vector<T> stack2vector(std::stack<T> mystack) {
  std::vector<T> vec;
  while (!mystack.empty()) {
    vec.emplace_back(mystack.top());
    mystack.pop();
  }
  return vec;
}

template <typename TypeParam>
class DwarfExpressionTest : public ::testing::Test {
 protected:
  void SetUp() override { ctx_ = {}; }

  void TearDown() override {
    this->expr_.clear();
    this->ClearStack();
  }

  Dwarf_Small GetOpCodeByOffset(Dwarf_Unsigned off) {
    int64_t idx = this->expr_.findOpIndexByOffset(off);
    if (idx != -1) {
      return this->expr_.getOp(idx).opcode;
    }
    return ILLEGAL_OP_CODE;
  }

  Dwarf_Signed StackAt(size_t index) {
    if (index < this->stack_.size()) {
      return stack2vector(this->stack_)[index];
    }
    return std::numeric_limits<Dwarf_Signed>::max();
  }

  void ClearStack() {
    while (!this->stack_.empty()) {
      this->stack_.pop();
    }
  }

  DwarfExpression expr_;
  Context ctx_;
  std::stack<Dwarf_Signed> stack_;
};
TYPED_TEST_SUITE_P(DwarfExpressionTest);

TYPED_TEST_P(DwarfExpressionTest, empty_ops) {
  // ops_ count == 0 error.
  Result ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_FALSE(ret.valid());
  ASSERT_EQ(ErrorCode::kIllegalState, ret.error_code);
  ASSERT_EQ(0U, ret.error_addr);
}

TYPED_TEST_P(DwarfExpressionTest, not_implemented) {
  // DW_OP_piece not implemented error.
  this->expr_.setOps({OP(DW_OP_nop, 0), OP(DW_OP_nop, 1), OP(DW_OP_piece, 2)});
  Result ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_FALSE(ret.valid());
  ASSERT_EQ(ErrorCode::kNotImplemented, ret.error_code);
  ASSERT_EQ(2U, ret.error_addr);
  ASSERT_EQ(DW_OP_piece, this->GetOpCodeByOffset(ret.error_addr));
}

TYPED_TEST_P(DwarfExpressionTest, illegal_op) {
  // Illegal Opcode error.
  Dwarf_Small invalid_ops[] = {0x00, 0x01, 0x02, 0x04, 0x05, 0x07};

  for (size_t i = 0; i < sizeof(invalid_ops); ++i) {
    this->expr_.clear();

    for (size_t l = 0; l < sizeof(invalid_ops); ++l) {
      if (i == l) {
        this->expr_.addOp(OP(invalid_ops[i], l));
      } else {
        this->expr_.addOp(OP(DW_OP_nop, l));
      }
    }

    Result ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
    ASSERT_FALSE(ret.valid());
    ASSERT_EQ(ErrorCode::kIllegalOp, ret.error_code);
    ASSERT_EQ(i, ret.error_addr);
    ASSERT_EQ(invalid_ops[i], this->GetOpCodeByOffset(ret.error_addr));
  }
}

TYPED_TEST_P(DwarfExpressionTest, op_addr) {
  this->expr_.setOps({OP1(DW_OP_addr, 0x45342312U, 0)});
  Result ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_TRUE(ret.valid());
  ASSERT_EQ(Result::Type::kAddress, ret.type);
  ASSERT_EQ(0x45342312U, ret.value);

  this->expr_.setOps({OP1(DW_OP_addr, 0x8978675645342312UL, 0)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_TRUE(ret.valid());
  ASSERT_EQ(Result::Type::kAddress, ret.type);
  ASSERT_EQ(0x8978675645342312UL, ret.value);
}

TYPED_TEST_P(DwarfExpressionTest, op_deref) {
  TypeParam deref_val = 0x12345678;
  this->ctx_.memory = [&](uint64_t addr, size_t size, char** buf,
                          size_t* buf_size) -> bool {
    if (addr == 0x2010) {
      *buf = reinterpret_cast<char*>(&deref_val);
      *buf_size = sizeof(TypeParam);
      return true;
    }
    return false;
  };

  // Try a dereference with nothing on the stack.
  this->expr_.setOps({OP(DW_OP_nop, 0), OP(DW_OP_deref, 1)});
  Result ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_EQ(ErrorCode::kStackIndexInvalid, ret.error_code);
  ASSERT_EQ(1U, ret.error_addr);
  ASSERT_EQ(DW_OP_deref, this->GetOpCodeByOffset(ret.error_addr));

  // Add an valid address, then dereference.
  this->expr_.setOps({OP1(DW_OP_const2s, 0x2010, 0), OP(DW_OP_deref, 2)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_TRUE(ret.valid());
  ASSERT_EQ(Result::Type::kAddress, ret.type);
  ASSERT_EQ(deref_val, ret.value);

  // Add an invalid address, then dereference that should fail in memory.
  this->expr_.setOps({OP1(DW_OP_const2s, 0x2011, 0), OP(DW_OP_deref, 2)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_FALSE(ret.valid());
  ASSERT_EQ(ErrorCode::kMemoryInvalid, ret.error_code);
  ASSERT_EQ(2U, ret.error_addr);
  ASSERT_EQ(DW_OP_deref, this->GetOpCodeByOffset(ret.error_addr));
}

TYPED_TEST_P(DwarfExpressionTest, op_deref_size) {
  TypeParam deref_val = 0x12345678;
  this->ctx_.memory = [&](uint64_t addr, size_t size, char** buf,
                          size_t* buf_size) -> bool {
    if (addr == 0x2010) {
      *buf = reinterpret_cast<char*>(&deref_val);
      *buf_size = size;
      return true;
    }
    return false;
  };

  for (size_t i = 1; i < sizeof(TypeParam); ++i) {
    this->expr_.setOps(
        {OP1(DW_OP_const2s, 0x2010, 0),
         OP1(DW_OP_deref_size, static_cast<Dwarf_Unsigned>(i), 2)});
    TypeParam expected_value = 0;
    memcpy(&expected_value, &deref_val, i);
    Result ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
    ASSERT_TRUE(ret.valid());
    ASSERT_EQ(Result::Type::kAddress, ret.type);
    ASSERT_EQ(expected_value, ret.value);
  }

  // Zero byte read.
  this->expr_.setOps({OP1(DW_OP_const2s, 0x2010, 0),
                      OP1(DW_OP_deref_size, 0 /* bytes_to_read */, 2)});
  Result ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_FALSE(ret.valid());
  ASSERT_EQ(ErrorCode::kIllegalOpd, ret.error_code);

  // Read too many bytes.
  this->expr_.setOps(
      {OP1(DW_OP_const2s, 0x2010, 0),
       OP1(DW_OP_deref_size, sizeof(TypeParam) + 1 /* bytes_to_read */, 2)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_FALSE(ret.valid());
  ASSERT_EQ(ErrorCode::kIllegalOpd, ret.error_code);

  // Force bad memory read.
  this->expr_.setOps({OP1(DW_OP_const2s, 0x4010, 0),
                      OP1(DW_OP_deref_size, 1 /* bytes_to_read */, 2)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_FALSE(ret.valid());
  ASSERT_EQ(ErrorCode::kMemoryInvalid, ret.error_code);
}

TYPED_TEST_P(DwarfExpressionTest, op_const_unsigned) {
  // const1u
  this->expr_.setOps({OP1(DW_OP_const1u, 0x12U, 0)});
  Result ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_TRUE(ret.valid());
  ASSERT_EQ(Result::Type::kAddress, ret.type);
  ASSERT_EQ(0x12U, ret.value);

  // const2u
  this->expr_.setOps({OP1(DW_OP_const2u, 0x1245U, 0)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_TRUE(ret.valid());
  ASSERT_EQ(Result::Type::kAddress, ret.type);
  ASSERT_EQ(0x1245U, ret.value);

  // const4u
  this->expr_.setOps({OP1(DW_OP_const4u, 0x45342312U, 0)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_TRUE(ret.valid());
  ASSERT_EQ(Result::Type::kAddress, ret.type);
  ASSERT_EQ(0x45342312U, ret.value);

  // const8u
  if (sizeof(TypeParam) == 8) {
    this->expr_.setOps({OP1(DW_OP_const8u, 0x0102030405060708ULL, 0)});
    ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
    ASSERT_TRUE(ret.valid());
    ASSERT_EQ(Result::Type::kAddress, ret.type);
    ASSERT_EQ(0x0102030405060708ULL, ret.value);
  } else {
    this->expr_.setOps({OP1(DW_OP_const8u, 0x05060708U, 0)});
    ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
    ASSERT_TRUE(ret.valid());
    ASSERT_EQ(Result::Type::kAddress, ret.type);
    ASSERT_EQ(0x05060708U, ret.value);
  }

  // constu (libdwarf handle ULEB128 for us)
  this->expr_.setOps({OP1(DW_OP_constu, 0x45342312U, 0)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_TRUE(ret.valid());
  ASSERT_EQ(Result::Type::kAddress, ret.type);
  ASSERT_EQ(0x45342312U, ret.value);
}

TYPED_TEST_P(DwarfExpressionTest, op_const_signed) {
#define EXPAND_I8(v) static_cast<uint64_t>(static_cast<int8_t>(v))
#define EXPAND_I16(v) static_cast<uint64_t>(static_cast<int16_t>(v))
#define EXPAND_I32(v) static_cast<uint64_t>(static_cast<int32_t>(v))
#define EXPAND_I64(v) static_cast<uint64_t>(static_cast<int64_t>(v))

  // const1s
  this->expr_.setOps({OP1(DW_OP_const1s, EXPAND_I8(0xff), 0)});
  Result ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_TRUE(ret.valid());
  ASSERT_EQ(Result::Type::kAddress, ret.type);
  ASSERT_EQ(static_cast<TypeParam>(-1), ret.value);

  // const2s
  this->expr_.setOps({OP1(DW_OP_const2s, EXPAND_I16(0xff08), 0)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_TRUE(ret.valid());
  ASSERT_EQ(Result::Type::kAddress, ret.type);
  ASSERT_EQ(static_cast<TypeParam>(-248), ret.value);

  // const4s
  this->expr_.setOps({OP1(DW_OP_const4s, EXPAND_I32(0xff030201), 0)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_TRUE(ret.valid());
  ASSERT_EQ(Result::Type::kAddress, ret.type);
  ASSERT_EQ(static_cast<TypeParam>(-16580095), ret.value);

  // const8s
  if (sizeof(TypeParam) == 8) {
    this->expr_.setOps({OP1(DW_OP_const8s, EXPAND_I64(0xffefefef01020304), 0)});
    ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
    ASSERT_TRUE(ret.valid());
    ASSERT_EQ(Result::Type::kAddress, ret.type);
    ASSERT_EQ(static_cast<TypeParam>(-4521264810949884LL), ret.value);
  } else {
    this->expr_.setOps({OP1(DW_OP_const8s, EXPAND_I32(0x1020304), 0)});
    ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
    ASSERT_TRUE(ret.valid());
    ASSERT_EQ(Result::Type::kAddress, ret.type);
    ASSERT_EQ(static_cast<TypeParam>(0x1020304), ret.value);
  }

  // consts (libdwarf handle SLEB128 for us)
  this->expr_.setOps({OP1(DW_OP_consts, EXPAND_I32(0xff030201), 0)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_TRUE(ret.valid());
  ASSERT_EQ(Result::Type::kAddress, ret.type);
  ASSERT_EQ(static_cast<TypeParam>(-16580095), ret.value);
}

TYPED_TEST_P(DwarfExpressionTest, op_dup) {
  // Should fail since nothing is on the stack.
  this->expr_.setOps({OP(DW_OP_dup, 0)});
  Result ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_FALSE(ret.valid());
  ASSERT_EQ(ErrorCode::kStackIndexInvalid, ret.error_code);
  ASSERT_EQ(0U, ret.error_addr);

  // Push on a value and dup.
  this->expr_.setOps({OP1(DW_OP_const1u, 0x15, 0), OP(DW_OP_dup, 1)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_TRUE(ret.valid());
  ASSERT_EQ(Result::Type::kAddress, ret.type);
  ASSERT_EQ(2, this->stack_.size());
  ASSERT_EQ(0x15U, this->StackAt(0));
  ASSERT_EQ(0x15U, this->StackAt(1));

  // Do it again.
  this->expr_.setOps({OP1(DW_OP_const1u, 0x23, 2), OP(DW_OP_dup, 3)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_TRUE(ret.valid());
  ASSERT_EQ(Result::Type::kAddress, ret.type);
  ASSERT_EQ(4, this->stack_.size());
  ASSERT_EQ(0x23U, this->StackAt(0));
  ASSERT_EQ(0x23U, this->StackAt(1));
  ASSERT_EQ(0x15U, this->StackAt(2));
  ASSERT_EQ(0x15U, this->StackAt(3));
}

TYPED_TEST_P(DwarfExpressionTest, op_drop) {
  // Push a couple of values.
  this->expr_.setOps(
      {OP1(DW_OP_const1u, 0x10, 0), OP1(DW_OP_const1u, 0x20, 1)});
  Result ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_EQ(2, this->stack_.size());

  // Drop the values.
  this->expr_.setOps({OP(DW_OP_drop, 2), OP(DW_OP_drop, 3)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_EQ(0, this->stack_.size());

  // Attempt to drop empty stack.
  this->expr_.setOps({OP(DW_OP_drop, 4)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_FALSE(ret.valid());
  ASSERT_EQ(4U, ret.error_addr);
  ASSERT_EQ(ErrorCode::kStackIndexInvalid, ret.error_code);
}

TYPED_TEST_P(DwarfExpressionTest, op_over) {
  // Push a couple of values.
  this->expr_.setOps(
      {OP1(DW_OP_const1u, 0x1a, 0), OP1(DW_OP_const1u, 0xed, 1)});
  Result ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_EQ(2, this->stack_.size());

  // Copy a value(the 2nd element, 0x1a).
  this->expr_.setOps({OP(DW_OP_over, 2)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_EQ(3, this->stack_.size());
  ASSERT_EQ(0x1aU, this->StackAt(0));
  ASSERT_EQ(0xedU, this->StackAt(1));
  ASSERT_EQ(0x1aU, this->StackAt(2));

  // Provoke a failure with this opcode.
  this->ClearStack();
  this->expr_.setOps({OP1(DW_OP_const1u, 0x1a, 0), OP(DW_OP_over, 1)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_FALSE(ret.valid());
  ASSERT_EQ(1U, ret.error_addr);
  ASSERT_EQ(ErrorCode::kStackIndexInvalid, ret.error_code);
}

TYPED_TEST_P(DwarfExpressionTest, op_pick) {
  // Choose a zero index with an empty stack.
  this->expr_.setOps({OP1(DW_OP_pick, 0x0, 0)});
  Result ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_FALSE(ret.valid());
  ASSERT_EQ(0U, ret.error_addr);
  ASSERT_EQ(ErrorCode::kStackIndexInvalid, ret.error_code);

  // Push a few values.
  this->ClearStack();
  this->expr_.setOps({OP1(DW_OP_const1u, 0x1a, 0), OP1(DW_OP_const1u, 0xed, 1),
                      OP1(DW_OP_const1u, 0x34, 2)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_EQ(3, this->stack_.size());

  // Copy the value at offset 2.
  this->expr_.setOps({OP1(DW_OP_pick, 0x1, 3)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_EQ(4, this->stack_.size());
  ASSERT_EQ(0xedU, this->StackAt(0));
  ASSERT_EQ(0x34U, this->StackAt(1));
  ASSERT_EQ(0xedU, this->StackAt(2));
  ASSERT_EQ(0x1aU, this->StackAt(3));

  // Copy the last value in the stack.
  this->expr_.setOps({OP1(DW_OP_pick, 0x3, 4)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_EQ(5, this->stack_.size());
  ASSERT_EQ(0x1aU, this->StackAt(0));
  ASSERT_EQ(0xedU, this->StackAt(1));
  ASSERT_EQ(0x34U, this->StackAt(2));
  ASSERT_EQ(0xedU, this->StackAt(3));
  ASSERT_EQ(0x1aU, this->StackAt(4));

  // Choose an invalid index.
  this->expr_.setOps({OP1(DW_OP_pick, 0x10, 5)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_FALSE(ret.valid());
  ASSERT_EQ(5U, ret.error_addr);
  ASSERT_EQ(ErrorCode::kStackIndexInvalid, ret.error_code);
}

TYPED_TEST_P(DwarfExpressionTest, op_swap) {
  // Push a couple of values.
  this->expr_.setOps(
      {OP1(DW_OP_const1u, 0x26, 0), OP1(DW_OP_const1u, 0xab, 1)});
  Result ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_EQ(2, this->stack_.size());
  ASSERT_EQ(0xabU, this->StackAt(0));
  ASSERT_EQ(0x26U, this->StackAt(1));

  // Swap values.
  this->expr_.setOps({OP(DW_OP_swap, 2)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_EQ(2, this->stack_.size());
  ASSERT_EQ(0x26U, this->StackAt(0));
  ASSERT_EQ(0xabU, this->StackAt(1));

  // Pop a value to cause a failure.
  this->ClearStack();
  this->expr_.setOps({OP1(DW_OP_const1u, 0x26, 0), OP(DW_OP_swap, 1)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_FALSE(ret.valid());
  ASSERT_EQ(1U, ret.error_addr);
  ASSERT_EQ(ErrorCode::kStackIndexInvalid, ret.error_code);
}

TYPED_TEST_P(DwarfExpressionTest, op_rot) {
  // Rotate that should cause a failure.
  this->expr_.setOps({OP(DW_OP_rot, 0)});
  Result ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_FALSE(ret.valid());
  ASSERT_EQ(0U, ret.error_addr);
  ASSERT_EQ(ErrorCode::kStackIndexInvalid, ret.error_code);

  // Only 1 value on stack, should fail.
  this->ClearStack();
  this->expr_.setOps({OP1(DW_OP_const1u, 0x10, 0), OP(DW_OP_rot, 1)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_EQ(1, this->stack_.size());
  ASSERT_FALSE(ret.valid());
  ASSERT_EQ(1U, ret.error_addr);
  ASSERT_EQ(ErrorCode::kStackIndexInvalid, ret.error_code);

  // Only 2 value on stack, should fail.
  this->expr_.setOps({OP1(DW_OP_const1u, 0x20, 1), OP(DW_OP_rot, 2)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_EQ(2, this->stack_.size());
  ASSERT_FALSE(ret.valid());
  ASSERT_EQ(2U, ret.error_addr);
  ASSERT_EQ(ErrorCode::kStackIndexInvalid, ret.error_code);

  // Should rotate properly.
  this->expr_.setOps({OP1(DW_OP_const1u, 0x30, 1), OP(DW_OP_rot, 2)});
  ret = this->expr_.evaluate(this->ctx_, 0, &this->stack_);
  ASSERT_EQ(3, this->stack_.size());
  ASSERT_TRUE(ret.valid());
  ASSERT_EQ(0x20U, this->StackAt(0));
  ASSERT_EQ(0x10U, this->StackAt(1));
  ASSERT_EQ(0x30U, this->StackAt(2));
}

REGISTER_TYPED_TEST_SUITE_P(DwarfExpressionTest, empty_ops, not_implemented,
                            illegal_op, op_addr, op_deref, op_deref_size,
                            op_const_unsigned, op_const_signed, op_dup, op_drop,
                            op_over, op_pick, op_swap, op_rot);
using DwarfExpressionTypes = ::testing::Types<uint64_t>;
INSTANTIATE_TYPED_TEST_SUITE_P(TypedDwarfExpressionTest, DwarfExpressionTest,
                               DwarfExpressionTypes);

}  // namespace dwarfexpr