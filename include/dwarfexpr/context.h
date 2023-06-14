#ifndef DWARFEXPR_CONTEXT_H
#define DWARFEXPR_CONTEXT_H

#include <functional>

namespace dwarfexpr {

using RegisterProvider = std::function<uint64_t(int)>;
using MemoryProvider = std::function<bool(uint64_t, size_t, char**, size_t*)>;

}  // namespace dwarfexpr

#endif  // DWARFEXPR_CONTEXT_H
