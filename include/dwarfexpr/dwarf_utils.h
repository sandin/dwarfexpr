#ifndef DWARFEXPR_DWARF_UTILS_H
#define DWARFEXPR_DWARF_UTILS_H

#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

#include <functional>
#include <limits>  // std::numeric_limits
#include <string>
#include <type_traits>
#include <utility>  // std::pair

namespace dwarfexpr {

// from retdec/dwarfparser/dwarf_utils.h

Dwarf_Addr getAttrAddr(Dwarf_Attribute attr, Dwarf_Addr def_val);
Dwarf_Unsigned getAttrNumb(Dwarf_Attribute attr, Dwarf_Unsigned def_val);
std::string getAttrStr(Dwarf_Attribute attr, std::string def_val);
Dwarf_Off getAttrRef(Dwarf_Attribute attr, Dwarf_Off def_val);
Dwarf_Off getAttrGlobalRef(Dwarf_Attribute attr, Dwarf_Off def_val);
Dwarf_Bool getAttrFlag(Dwarf_Attribute attr, Dwarf_Bool def_val);
Dwarf_Block* getAttrBlock(Dwarf_Attribute attr);
Dwarf_Sig8 getAttrSig(Dwarf_Attribute attr);
void getAttrExprLoc(Dwarf_Attribute attr, Dwarf_Unsigned* exprlen,
                    Dwarf_Ptr* ptr);

std::string getDwarfError(Dwarf_Error& error);

bool getDieFromOffset(Dwarf_Debug dbg, Dwarf_Off off, Dwarf_Die& die);

// end

// scope
template <typename Callable>
class scope_exit {
  Callable ExitFunction;
  bool Engaged = true;  // False once moved-from or release()d.

 public:
  template <typename Fp>
  explicit scope_exit(Fp&& F) : ExitFunction(std::forward<Fp>(F)) {}

  scope_exit(scope_exit&& Rhs)
      : ExitFunction(std::move(Rhs.ExitFunction)), Engaged(Rhs.Engaged) {
    Rhs.release();
  }
  scope_exit(const scope_exit&) = delete;
  scope_exit& operator=(scope_exit&&) = delete;
  scope_exit& operator=(const scope_exit&) = delete;

  void release() { Engaged = false; }

  ~scope_exit() {
    if (Engaged) ExitFunction();
  }
};

// Keeps the callable object that is passed in, and execute it at the
// destruction of the returned object (usually at the scope exit where the
// returned object is kept).
template <typename Callable>
scope_exit<typename std::decay<Callable>::type> make_scope_exit(Callable&& F) {
  return scope_exit<typename std::decay<Callable>::type>(
      std::forward<Callable>(F));
}

// end scope

constexpr Dwarf_Unsigned MAX_DWARF_UNSIGNED =
    std::numeric_limits<Dwarf_Unsigned>::max();
constexpr Dwarf_Off MAX_DWARF_OFF = std::numeric_limits<Dwarf_Off>::max();
constexpr Dwarf_Addr MAX_DWARF_ADDR = std::numeric_limits<Dwarf_Addr>::max();

int getLowAndHighPc(Dwarf_Debug dbg, Dwarf_Die die, bool* have_pc_range,
                    Dwarf_Addr* lowpc_out, Dwarf_Addr* highpc_out,
                    Dwarf_Error* error);
int getRnglistsBase(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Off* base_out,
                    Dwarf_Error* error);

std::string getFunctionName(Dwarf_Debug dbg, Dwarf_Die func_die, bool demangle,
                            std::string def_val);
std::string getDeclFile(Dwarf_Debug dbg, Dwarf_Die cu_die, Dwarf_Die func_die,
                        std::string def_val);
Dwarf_Unsigned getDeclLine(Dwarf_Debug dbg, Dwarf_Die func_die,
                           Dwarf_Unsigned def_val);
std::pair<std::string, Dwarf_Unsigned> getFileNameAndLineNumber(
    Dwarf_Debug dbg, Dwarf_Die cu_die, Dwarf_Addr pc, std::string def_val1,
    Dwarf_Unsigned def_val2);

std::string demangleName(const std::string& mangled);

using DwarfDIEWalker =
    std::function<void(Dwarf_Debug dbg, Dwarf_Die parent, Dwarf_Die child,
                       int cu_lv, int max_lv, void* ctx)>;
void walkDIE(Dwarf_Debug dbg, Dwarf_Die parent_die, Dwarf_Die die, int cur_lv,
             int max_lv, void* ctx, DwarfDIEWalker walker);

void dumpDIE(Dwarf_Debug dbg, Dwarf_Die die);

std::string hexstring(char* buf, size_t buf_size);

}  // namespace dwarfexpr

#endif  // DWARFEXPR_DWARF_UTILS_H
