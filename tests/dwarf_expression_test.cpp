#include "dwarfexpr/dwarf_expression.h"

#include <gtest/gtest.h>

namespace dwarfexpr {

using Result = DwarfExpression::Result;
using ErrorCode = DwarfExpression::ErrorCode;
using Context = DwarfExpression::Context;

template <typename TypeParam>
class DwarfExpressionTest : public ::testing::Test {
 protected:
  void SetUp() override { ctx_ = {}; }

  void TearDown() override { expr_.clear(); }

  DwarfExpression expr_;
  Context ctx_;
};
TYPED_TEST_SUITE_P(DwarfExpressionTest);

TYPED_TEST_P(DwarfExpressionTest, Evaluate) {
  // ops_ count == 0 error.
  Result ret = this->expr_.evaluate(this->ctx_, 0);
  ASSERT_FALSE(ret.valid());
  ASSERT_EQ(ErrorCode::kIllegalState, ret.error_code);
  ASSERT_EQ(0U, ret.error_addr);

  // DW_OP_piece not implemented error.
  this->expr_.clear();
  this->expr_.addOp({.opcode = DW_OP_piece});
  ret = this->expr_.evaluate(this->ctx_, 0);
  ASSERT_FALSE(ret.valid());
  ASSERT_EQ(ErrorCode::kNotImplemented, ret.error_code);
  ASSERT_EQ(0U, ret.error_addr);

  // Illegal Opcode error.
  this->expr_.clear();
  this->expr_.addOp({.opcode = 0xef});
  ret = this->expr_.evaluate(this->ctx_, 0);
  ASSERT_FALSE(ret.valid());
  ASSERT_EQ(ErrorCode::kIllegalOp, ret.error_code);
  ASSERT_EQ(0U, ret.error_addr);
}

REGISTER_TYPED_TEST_SUITE_P(DwarfExpressionTest, Evaluate);
using DwarfExpressionTypes = ::testing::Types<uint64_t>;
INSTANTIATE_TYPED_TEST_SUITE_P(TypedDwarfExpressionTest, DwarfExpressionTest,
                               DwarfExpressionTypes);

}  // namespace dwarfexpr