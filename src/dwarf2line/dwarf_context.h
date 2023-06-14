#ifndef DWARF2LINE_DWARF_CONTEXT_H
#define DWARF2LINE_DWARF_CONTEXT_H

#include <cstdint>
#include <vector>
#include <string>

namespace dwarf2line {

constexpr char kDwarfContextMagic[] = "DWFC";

struct DwarfContextFrame {
  uint32_t frame_num;
  std::string frame_func;

  std::vector<uint64_t> regs;

  uint64_t stack_memory_base_addr;
  std::vector<uint8_t> stack_memory;
};

struct DwarfContextThreadHeader {
  uint32_t tid;
  uint32_t crashed;
  uint32_t frames_size;
};

struct DwarfContextThread {
  DwarfContextThreadHeader header;
  std::vector<DwarfContextFrame> frames;
};

struct DwarfContextHeader {
  char magic[4];
  uint16_t version;
  uint16_t arch;  // 0:32-bit, 1:64-bit
  uint32_t threads_size;
};

struct DwarfContext {
  DwarfContextHeader header;
  std::vector<DwarfContextThread> threads;
};

bool load_dwarf_context_file(const char* filename, DwarfContext* ctx);

void dump_dwarf_context(DwarfContext* ctx);

}  // namespace dwarf2line

#endif  // DWARF2LINE_DWARF_CONTEXT_H