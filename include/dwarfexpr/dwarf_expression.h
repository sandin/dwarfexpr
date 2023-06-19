#ifndef DWARFEXPR_DWARF_EXPRESSION_H
#define DWARFEXPR_DWARF_EXPRESSION_H

#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

#include <string>
#include <vector>
#include <functional>

namespace dwarfexpr {

class DwarfLocation;  // forward declaration

struct DwarfOp {
  Dwarf_Small opcode;  // Operation code.
  Dwarf_Unsigned op1;  // Operand #1.
  Dwarf_Unsigned op2;  // Operand #2.
  Dwarf_Unsigned op3;  // Operand #3.
  Dwarf_Unsigned off;  // Offset in locexpr used in OP_BRA.
};

/**
 * @brief DWARF expression.
 * @see https://dwarfstd.org/doc/040408.1.html
 */
class DwarfExpression {
 public:
  using RegisterProvider = std::function<uint64_t(int)>;
  using MemoryProvider = std::function<bool(uint64_t, size_t, char**, size_t*)>;
  using CfaProvider = std::function<Dwarf_Addr(Dwarf_Addr)>;

  struct Result {
    enum class Type { kInvalid = 0, kAddress, kValue };

    Type type;
    Dwarf_Addr value;
    std::string error_msg;

    static Result Error(std::string err) {
      return Result{.type = Type::kInvalid, .value = 0, .error_msg = err};
    }

    static Result Value(Dwarf_Addr value) {
      return Result{.type = Type::kValue, .value = value, .error_msg = ""};
    }

    static Result Address(Dwarf_Addr address) {
      return Result{.type = Type::kAddress, .value = address, .error_msg = ""};
    }

    bool valid() const { return type != Type::kInvalid; }
  };

  struct Context {
    Dwarf_Addr cuLowAddr;
    Dwarf_Addr cuHighAddr;
    DwarfLocation* frameBaseLoc;
    RegisterProvider registers;
    MemoryProvider memory;
    CfaProvider cfa;
  };

  DwarfExpression() {}
  ~DwarfExpression() {}

  void addOp(DwarfOp&& op) { ops_.emplace_back(op); }

  Result evaluate(const Context& context, Dwarf_Addr pc) const;
  void dump() const;
  std::size_t count() const { return ops_.size(); }

 private:
  std::vector<DwarfOp> ops_;
};

}  // namespace dwarfexpr

#endif  // DWARFEXPR_DWARF_EXPRESSION_H
