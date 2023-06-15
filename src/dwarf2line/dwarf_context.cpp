#include "dwarf_context.h"

#include <string.h>  // strcmp

#include <fstream>

#include "dwarfexpr/dwarf_utils.h"

namespace dwarf2line {

bool load_dwarf_context_file(const char* filename, DwarfContext* ctx) {
  std::ifstream file(filename, std::ios::binary);

  std::streampos file_size;
  file.seekg(0, std::ios::end);
  file_size = file.tellg();
  file.seekg(0, std::ios::beg);

  if (static_cast<size_t>(file_size) < sizeof(DwarfContextHeader)) {
    printf("Error: bad format file(file_size = %zu).\n",
           static_cast<size_t>(file_size));
    return false;
  }

  file.read(reinterpret_cast<char*>(&ctx->header), sizeof(ctx->header));
  if (memcmp(&kDwarfContextMagic[0], ctx->header.magic,
             sizeof(ctx->header.magic)) != 0) {
    printf("Error: bad format file(magic `%s` != `%s`).\n", ctx->header.magic,
           kDwarfContextMagic);
    return false;
  }

  ctx->threads.resize(ctx->header.threads_size);
  for (uint32_t i = 0; i < ctx->header.threads_size; ++i) {
    DwarfContextThreadHeader& thread_header = ctx->threads[i].header;
    file.read(reinterpret_cast<char*>(&thread_header), sizeof(thread_header));
    ctx->threads[i].frames.resize(thread_header.frames_size);
    for (uint32_t l = 0; l < thread_header.frames_size; ++l) {
      DwarfContextFrame& frame = ctx->threads[i].frames[l];
      file.read(reinterpret_cast<char*>(&frame.frame_num), sizeof(uint32_t));

      uint32_t frame_func_len;
      file.read(reinterpret_cast<char*>(&frame_func_len), sizeof(uint32_t));
      char* frame_func = static_cast<char*>(malloc(frame_func_len));
      file.read(frame_func, frame_func_len);
      frame.frame_func.assign(frame_func);
      free(frame_func);

      uint32_t regs_size;
      file.read(reinterpret_cast<char*>(&regs_size), sizeof(uint32_t));
      frame.regs.reserve(regs_size);
      for (uint32_t x = 0; x < regs_size; ++x) {
        uint64_t reg;
        file.read(reinterpret_cast<char*>(&reg), sizeof(uint64_t));
        frame.regs.emplace_back(reg);
      }

      file.read(reinterpret_cast<char*>(&frame.stack_memory_base_addr),
                sizeof(uint64_t));
      uint32_t stack_memory_size;
      file.read(reinterpret_cast<char*>(&stack_memory_size), sizeof(uint32_t));
      frame.stack_memory.resize(stack_memory_size);
      file.read(reinterpret_cast<char*>(frame.stack_memory.data()),
                stack_memory_size);
    }
  }

  file.close();
  return true;
}

void dump_dwarf_context(DwarfContext* ctx) {
#define T "  "
  printf("DwarfContext:\n");
  printf("magic: %s\n", ctx->header.magic);
  printf("version: %d\n", ctx->header.version);
  printf("arch: %d(%s)\n", ctx->header.arch,
         ctx->header.arch == 0 ? "32-bit" : "64-bit");
  printf("threads_size: %d\n", ctx->header.threads_size);
  for (const DwarfContextThread& thread : ctx->threads) {
    printf(T "Thread tid: %d\n", thread.header.tid);
    printf(T "crashed: %d\n", thread.header.crashed);
    printf(T "frames_size: %d\n", thread.header.frames_size);
    for (const DwarfContextFrame& frame : thread.frames) {
      printf(T T "frame_num: %d\n", frame.frame_num);
      printf(T T "frame_func: %s\n", frame.frame_func.c_str());
      printf(T T "registers: (%zu)\n", frame.regs.size());
      printf(T T T);
      for (size_t i = 0; i < frame.regs.size(); ++i) {
        printf("x%02zu = 0x%016lx ", i, frame.regs[i]);
        if (i != 0 && (i - 1) % 2 == 0) {
          if (i != frame.regs.size() - 1) {
            printf("\n" T T T);
          }
        }
      }
      printf("\n");
      printf(T T "stack_memory_base_addr: 0x%lx\n",
             frame.stack_memory_base_addr);
      printf(T T T);
      for (size_t i = 0; i < frame.stack_memory.size(); ++i) {
        printf("%02x ", frame.stack_memory[i]);
        if (i != 0 && (i + 1) % 16 == 0) {
          if (i != frame.stack_memory.size() - 1) {
            printf("\n" T T T);
          }
        }
      }
      printf("\n");
      printf("\n");
    }
    printf("\n");
  }
}

}  // namespace dwarf2line