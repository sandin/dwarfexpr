#ifndef DWARFEXPR_DWARF_EXPRESSION_H
#define DWARFEXPR_DWARF_EXPRESSION_H

#include <assert.h>
#include <inttypes.h>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

#include <functional>
#include <initializer_list>
#include <string>
#include <vector>

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
  using RegisterProvider = std::function<bool(int, uint64_t*)>;
  using MemoryProvider = std::function<bool(uint64_t, size_t, char**, size_t*)>;
  using CfaProvider = std::function<Dwarf_Addr(Dwarf_Addr)>;

  enum class ErrorCode {
    kNone = 0,
    kMemoryInvalid,
    kRegisterInvalid,
    kFrameBaseInvalid,
    kIllegalState,
    kIllegalOp,
    kStackIndexInvalid,
    kCfaInvalid,
    kNotImplemented,
    kAddressInvalid,
    kUnknown = 255
  };

  struct Result {
    enum class Type { kInvalid = 0, kAddress, kValue };

    Type type;
    Dwarf_Addr value;

    ErrorCode error_code;
    uint64_t error_addr;

    static Result Error(ErrorCode err_code, uint64_t err_addr) {
      return Result{.type = Type::kInvalid,
                    .value = 0,
                    .error_code = err_code,
                    .error_addr = err_addr};
    }

    static Result Value(Dwarf_Addr value) {
      return Result{.type = Type::kValue,
                    .value = value,
                    .error_code = ErrorCode::kNone,
                    .error_addr = 0};
    }

    static Result Address(Dwarf_Addr address) {
      return Result{.type = Type::kAddress,
                    .value = address,
                    .error_code = ErrorCode::kNone,
                    .error_addr = 0};
    }

    bool valid() const {
      return type != Type::kInvalid && error_code == ErrorCode::kNone;
    }
  };

  struct Context {
    Dwarf_Addr cuLowAddr;
    Dwarf_Addr cuHighAddr;
    DwarfLocation* frameBaseLoc;  // for DW_OP_fbreg
    RegisterProvider registers;
    MemoryProvider memory;
    CfaProvider cfa;  // for DW_OP_call_frame_cfa
  };

  DwarfExpression() {}
  ~DwarfExpression() {}

  void addOp(DwarfOp&& op) { ops_.emplace_back(op); }
  void setOps(std::initializer_list<DwarfOp> ops) {
    clear();
    ops_.insert(ops_.end(), ops.begin(), ops.end());
  }
  const DwarfOp& getOp(int64_t index) const {
    assert(static_cast<size_t>(index) < ops_.size());
    return ops_.at(index);
  }
  void clear() { ops_.clear(); }

  Result evaluate(const Context& context, Dwarf_Addr pc) const;
  void dump() const;
  std::size_t count() const { return ops_.size(); }

  static bool loadExprFromLoclist(Dwarf_Loc_Head_c loclist_head,
                                  Dwarf_Unsigned idx, DwarfExpression* expr,
                                  Dwarf_Addr* lowAddr, Dwarf_Addr* highAddr);

  template <typename T>
  static T readMemory(MemoryProvider memory, uint64_t addr, T def_val) {
    char* buf = nullptr;
    size_t out_size = 0;
    if (!memory(addr, sizeof(T), &buf, &out_size)) {
      printf("Error: can not read memory at addr 0x%" PRIx64 "\n", addr);
      return def_val;
    }
    if (sizeof(T) != out_size) {
      printf("Error: expect size: %zu, actual size: %zu\n", sizeof(T),
             out_size);
      return def_val;
    }

    return *(reinterpret_cast<T*>(buf));
  }

  static uint64_t readRegister(RegisterProvider registers, int reg_num,
                               uint64_t def_val);

  int64_t findOpIndexByOffset(Dwarf_Unsigned off) const;

 private:
  std::vector<DwarfOp> ops_;
};

}  // namespace dwarfexpr

#endif  // DWARFEXPR_DWARF_EXPRESSION_H
