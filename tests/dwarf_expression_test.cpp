#include "dwarfexpr/dwarf_expression.h"

#include <gtest/gtest.h>

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

template <typename TypeParam>
class DwarfExpressionTest : public ::testing::Test {
 protected:
  void SetUp() override { ctx_ = {}; }

  void TearDown() override { expr_.clear(); }

  Dwarf_Small GetOpCodeByOffset(Dwarf_Unsigned off) {
    int64_t idx = this->expr_.findOpIndexByOffset(off);
    if (idx != -1) {
      return this->expr_.getOp(idx).opcode;
    }
    return ILLEGAL_OP_CODE;
  }

  DwarfExpression expr_;
  Context ctx_;
};
TYPED_TEST_SUITE_P(DwarfExpressionTest);

TYPED_TEST_P(DwarfExpressionTest, empty_ops) {
  // ops_ count == 0 error.
  Result ret = this->expr_.evaluate(this->ctx_, 0);
  ASSERT_FALSE(ret.valid());
  ASSERT_EQ(ErrorCode::kIllegalState, ret.error_code);
  ASSERT_EQ(0U, ret.error_addr);
}

TYPED_TEST_P(DwarfExpressionTest, not_implemented) {
  // DW_OP_piece not implemented error.
  this->expr_.setOps({OP(DW_OP_nop, 0), OP(DW_OP_nop, 1), OP(DW_OP_piece, 2)});
  Result ret = this->expr_.evaluate(this->ctx_, 0);
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

    Result ret = this->expr_.evaluate(this->ctx_, 0);
    ASSERT_FALSE(ret.valid());
    ASSERT_EQ(ErrorCode::kIllegalOp, ret.error_code);
    ASSERT_EQ(i, ret.error_addr);
    ASSERT_EQ(invalid_ops[i], this->GetOpCodeByOffset(ret.error_addr));
  }
}

TYPED_TEST_P(DwarfExpressionTest, op_addr) {
  this->expr_.setOps({OP1(DW_OP_addr, 0x45342312U, 0)});
  Result ret = this->expr_.evaluate(this->ctx_, 0);
  ASSERT_TRUE(ret.valid());
  ASSERT_EQ(Result::Type::kAddress, ret.type);
  ASSERT_EQ(0x45342312U, ret.value);

  this->expr_.setOps({OP1(DW_OP_addr, 0x8978675645342312UL, 0)});
  ret = this->expr_.evaluate(this->ctx_, 0);
  ASSERT_TRUE(ret.valid());
  ASSERT_EQ(Result::Type::kAddress, ret.type);
  ASSERT_EQ(0x8978675645342312UL, ret.value);
}

TYPED_TEST_P(DwarfExpressionTest, op_deref) {
  // clang-format off
  this->expr_.setOps({
      // Try a dereference with nothing on the stack.
      OP(DW_OP_deref, 0),
      // Add an address, then dereference.
      OP1(DW_OP_const2s, 0x2010, 1), OP(DW_OP_deref, 3),
      // Now do another dereference that should fail in memory.
      OP(DW_OP_deref, 4),
  });
  // clang-format on

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

  Result ret = this->expr_.evaluate(this->ctx_, 0);
  ASSERT_EQ(ErrorCode::kStackIndexInvalid, ret.error_code);
  ASSERT_EQ(0U, ret.error_addr);
  ASSERT_EQ(DW_OP_deref, this->GetOpCodeByOffset(ret.error_addr));

  // TODO: step
  ret = this->expr_.evaluate(this->ctx_, 0);
  ASSERT_TRUE(ret.valid());
  ASSERT_EQ(Result::Type::kAddress, ret.type);
  ASSERT_EQ(deref_val, ret.value);

  ret = this->expr_.evaluate(this->ctx_, 0);
  ASSERT_EQ(ErrorCode::kMemoryInvalid, ret.error_code);
  ASSERT_EQ(4U, ret.error_addr);
  ASSERT_EQ(DW_OP_deref, this->GetOpCodeByOffset(ret.error_addr));
}

REGISTER_TYPED_TEST_SUITE_P(DwarfExpressionTest, empty_ops, not_implemented,
                            illegal_op, op_addr, op_deref);
using DwarfExpressionTypes = ::testing::Types<uint64_t>;
INSTANTIATE_TYPED_TEST_SUITE_P(TypedDwarfExpressionTest, DwarfExpressionTest,
                               DwarfExpressionTypes);

}  // namespace dwarfexpr