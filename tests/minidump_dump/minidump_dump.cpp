#include <algorithm>  // sort
#include <iomanip>    // std::setw
#include <iostream>

#include "minidump/minidump.h"

#ifdef CAPSTONE_ENABLED
#include "capstone/capstone.h"
#endif

using minidump::Minidump;
using minidump::MinidumpContext;

const char USAGE[] = "Usage: minidump_dump <minidump_file>";

void dump_system_info(Minidump* minidump, const MDRawSystemInfo& system_info) {
  std::cout << "Operating system: ";
  switch (system_info.platform_id) {
    case MD_OS_IOS:
      std::cout << "iOS";
      break;
    case MD_OS_ANDROID:
      std::cout << "Android";
      break;
    default:
      std::cout << system_info.platform_id;
      break;
  }
  std::cout << "\n";

  std::cout << "CPU: ";
  switch (system_info.processor_architecture) {
    case MD_CPU_ARCHITECTURE_X86:
      std::cout << "x86";
      break;
    case MD_CPU_ARCHITECTURE_AMD64:
      std::cout << "amd64";
      break;
    case MD_CPU_ARCHITECTURE_ARM:
      std::cout << "arm";
      break;
    case MD_CPU_ARCHITECTURE_ARM64:  // passthrough
    case MD_CPU_ARCHITECTURE_ARM64_OLD:
      std::cout << "arm64";
      break;
    default:
      std::cout << system_info.processor_architecture;
      break;
  }
  std::cout << "\n";
  std::cout << "     "
            << static_cast<uint32_t>(system_info.number_of_processors)
            << " CPUs\n\n";
  std::cout << "GPU: UNKNOWN\n\n";
}

void dump_exception(Minidump* minidump, const MDRawExceptionStream& exception) {
  std::cout << "Crash reason: ";
  std::cout << exception.exception_record.exception_code;
  std::cout << "\n";
  std::cout << "Crash address: ";
  std::cout << "0x" << std::hex << exception.exception_record.exception_address
            << std::dec << "\n\n";
}

void dump_context(Minidump* minidump, MinidumpContext* context) {
  switch (context->GetCpuType()) {
    case MD_CONTEXT_X86:
      break;
    case MD_CONTEXT_AMD64: {
      auto ctx = context->GetContextAMD64();
      // clang-format off
      std::cout << "     " << "rax" << " = " << "0x" << std::hex << std::setw(16) << std::setfill('0') << ctx->rax;
      std::cout << "     " << "rdx" << " = " << "0x" << std::hex << std::setw(16) << std::setfill('0') << ctx->rdx;
      std::cout << "\n";
      std::cout << "     " << "rcx" << " = " << "0x" << std::hex << std::setw(16) << std::setfill('0') << ctx->rcx;
      std::cout << "     " << "rbx" << " = " << "0x" << std::hex << std::setw(16) << std::setfill('0') << ctx->rbx;
      std::cout << "\n";
      std::cout << "     " << "rsi" << " = " << "0x" << std::hex << std::setw(16) << std::setfill('0') << ctx->rsi;
      std::cout << "     " << "rdi" << " = " << "0x" << std::hex << std::setw(16) << std::setfill('0') << ctx->rdi;
      std::cout << "\n";
      std::cout << "     " << "rbp" << " = " << "0x" << std::hex << std::setw(16) << std::setfill('0') << ctx->rbp;
      std::cout << "     " << "rsp" << " = " << "0x" << std::hex << std::setw(16) << std::setfill('0') << ctx->rsp;
      std::cout << "\n";
      std::cout << "     " << "r8" << " = " << "0x" << std::hex << std::setw(16) << std::setfill('0') << ctx->r8;
      std::cout << "     " << "r9" << " = " << "0x" << std::hex << std::setw(16) << std::setfill('0') << ctx->r9;
      std::cout << "\n";
      std::cout << "     " << "r10" << " = " << "0x" << std::hex << std::setw(16) << std::setfill('0') << ctx->r10;
      std::cout << "     " << "r11" << " = " << "0x" << std::hex << std::setw(16) << std::setfill('0') << ctx->r11;
      std::cout << "\n";
      std::cout << "     " << "r12" << " = " << "0x" << std::hex << std::setw(16) << std::setfill('0') << ctx->r12;
      std::cout << "     " << "r13" << " = " << "0x" << std::hex << std::setw(16) << std::setfill('0') << ctx->r13;
      std::cout << "\n";
      std::cout << "     " << "r14" << " = " << "0x" << std::hex << std::setw(16) << std::setfill('0') << ctx->r14;
      std::cout << "     " << "r15" << " = " << "0x" << std::hex << std::setw(16) << std::setfill('0') << ctx->r15;
      std::cout << "\n";
      std::cout << "     " << "rip" << " = " << "0x" << std::hex << std::setw(16) << std::setfill('0') << ctx->rip;
      std::cout << "\n";
      // clang-format on
      break;
    }
    case MD_CONTEXT_ARM:
      break;
    case MD_CONTEXT_ARM64:  // passthrough
    case MD_CONTEXT_ARM64_OLD: {
      auto ctx = context->GetContextARM64();
      static const char* register_names[] = {
          "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",  "x8",
          "x9",  "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17",
          "x18", "x19", "x20", "x21", "x22", "x23", "x24", "x25", "x26",
          "x27", "x28", "x29", "x30", "sp",  "pc",  NULL};

      for (int i = 0; register_names[i]; ++i) {
        std::cout << "     " << register_names[i] << " = "
                  << "0x" << std::hex << std::setw(16) << std::setfill('0')
                  << ctx->iregs[i] << std::dec;
        if (i != 0 && (i + 1) % 2 == 0) {
          std::cout << "\n";
        } else {
          std::cout << "     ";
        }
      }
      std::cout << "\n";
      break;
    }
    default:
      printf("Error: unsupported cpu type: %d\n", context->GetCpuType());
      break;
  }
}

static void hexdump(std::string indent, char* buffer, size_t buffer_size,
                    uint64_t off) {
  uint64_t stack_start = reinterpret_cast<uint64_t>(buffer);
  uint64_t stack_end = stack_start + buffer_size;
  for (uint64_t address = stack_start; address < stack_end;) {
    // Print the start address of this row.
    printf("\n%s %016" PRIx64, indent.c_str(), off);

    // Print data in hex.
    const int kBytesPerRow = 16;
    std::string data_as_string;
    for (int i = 0; i < kBytesPerRow; ++i, ++address, ++off) {
      if (address < stack_end) {
        uint8_t value = *reinterpret_cast<uint8_t*>(address);
        printf(" %02x", value);
        data_as_string.push_back(isprint(value) ? value : '.');
      } else {
        printf("   ");
        data_as_string.push_back(' ');
      }
    }
    // Print data as string.
    printf("  %s", data_as_string.c_str());
  }
}

#ifdef CAPSTONE_ENABLED
void dump_disasm(Minidump* minidump, uint64_t pc, uint32_t cpu_type) {
  uint64_t start_addr = pc - 128;
  uint64_t size = 256;

  char* buffer = nullptr;
  size_t buffer_size = 0;
  if (minidump->GetMemory(start_addr, size, &buffer, &buffer_size)) {
    printf("\nDisasm, pc: 0x%lx\n", pc);

    csh handle;
    cs_insn* insn;
    size_t count;
    cs_err ret;
    switch (cpu_type) {
      case MD_CONTEXT_X86:
        ret = cs_open(CS_ARCH_X86, CS_MODE_32, &handle);
        break;
      case MD_CONTEXT_AMD64:
        ret = cs_open(CS_ARCH_X86, CS_MODE_64, &handle);
        break;
      case MD_CONTEXT_ARM:
        ret = cs_open(CS_ARCH_ARM, CS_MODE_ARM, &handle);
        break;
      case MD_CONTEXT_ARM64:  // passthrough
      case MD_CONTEXT_ARM64_OLD:
        ret = cs_open(CS_ARCH_ARM64, CS_MODE_ARM, &handle);
        break;
      default:
        printf("Error: unsupported cpu type: %d\n", cpu_type);
        minidump->FreeMemory(buffer);
        return;
    }
    if (ret != CS_ERR_OK) {
      minidump->FreeMemory(buffer);
      return;
    }
    count = cs_disasm(handle, reinterpret_cast<const uint8_t*>(buffer),
                      buffer_size, start_addr, 0, &insn);
    if (count > 0) {
      size_t j;
      for (j = 0; j < count; j++) {
        printf("%s0x%lx:\t%s\t\t%s\n", insn[j].address == pc ? " -> " : "    ",
               insn[j].address, insn[j].mnemonic, insn[j].op_str);
      }
      cs_free(insn, count);
    }
    cs_close(&handle);
    std::cout << "\n";

    minidump->FreeMemory(buffer);
  } else {
    printf("Error: can not read memory at addr: 0x%lx\n", start_addr);
  }
}
#else
void dump_disasm(Minidump* minidump, uint64_t pc, uint32_t cpu_type) {}
#endif  // #ifdef CAPSTONE_ENABLED

void dump_thread(Minidump* minidump, MDRawThread* thread, bool crashed) {
  std::cout << "Thread " << std::dec << thread->thread_id;
  if (crashed) {
    std::cout << " (crashed)";
  }
  std::cout << "\n";

  auto context = minidump->GetContext(thread->thread_id);
  uint64_t fp = 0;
  uint64_t sp = 0;
  uint64_t pc = 0;
  uint32_t cpu_type = 0;
  if (context) {
    dump_context(minidump, context);
    context->GetStackPointer(&sp);
    context->GetFramePointer(&fp);
    context->GetInstructionPointer(&pc);
    cpu_type = context->GetCpuType();
  } else {
    std::cerr << "Can not get context of thread\n";
  }
  std::cout << "\n";

#ifdef CAPSTONE_ENABLED
  if (crashed) {
    if (pc != 0 && cpu_type != 0) {
      dump_disasm(minidump, pc, cpu_type);
    }
  }
#endif

  if (fp != 0 && sp != 0) {
    uint64_t stack_len = fp - sp;
    if (0 < stack_len && stack_len < 0x10000) {
      std::cout << "     Stack contents: (" << stack_len << ")";
      char* buffer = nullptr;
      size_t buffer_size = 0;
      if (minidump->GetMemory(sp, stack_len, &buffer, &buffer_size)) {
        hexdump("     ", buffer, buffer_size, sp);
        minidump->FreeMemory(buffer);
      }
    }
    std::cout << "\n";
  }

  std::cout << "\n";
}

void dump_module(Minidump* minidump, MDRawModule* raw_module) {
  std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0')
            << raw_module->base_of_image << " - ";
  std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0')
            << (raw_module->base_of_image + raw_module->size_of_image - 1);
  std::cout << "   " << minidump->ReadString(raw_module->module_name_rva);
  std::cout << " (size="
            << "0x" << std::hex << raw_module->size_of_image;
  std::cout << ", checksum="
            << "0x" << std::hex << raw_module->checksum;
  std::cout << ", timedateStamp="
            << "0x" << std::hex << raw_module->time_date_stamp;
  std::cout << ")"
            << "\n";
}

void dump_memory(Minidump* minidump, MDMemoryDescriptor* memory) {
  std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0')
            << memory->start_of_memory_range << " - ";
  std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0')
            << (memory->start_of_memory_range + memory->memory.data_size - 1)
            << "\n";
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf(USAGE);
    return -1;
  }

  char* filename = argv[1];
  Minidump minidump(filename);
  bool success = minidump.Read();
  if (success) {
    auto system_info = minidump.GetSystemInfo();
    dump_system_info(&minidump, system_info);

    auto exception = minidump.GetException();
    dump_exception(&minidump, exception);

    auto thread = minidump.GetCrashThread();
    uint32_t crash_thread_id = 0;
    if (thread) {
      crash_thread_id = thread->thread_id;
      dump_thread(&minidump, thread, true);
    }

    auto threads = minidump.GetThreads();
    for (auto thread : threads) {
      if (thread.thread_id == crash_thread_id) {
        continue;
      }
      dump_thread(&minidump, &thread, false);
    }

    std::cout << "Loaded modules:\n";
    auto modules = minidump.GetModules();
    sort(modules.begin(), modules.end(), [](const auto& m1, const auto& m2) {
      return m2.base_of_image > m1.base_of_image;
    });
    for (auto raw_module : modules) {
      dump_module(&minidump, &raw_module);
    }
    std::cout << "\n";

    std::cout << "Saved memories:\n";
    auto memories = minidump.GetMemories();
    sort(memories.begin(), memories.end(), [](const auto& m1, const auto& m2) {
      return m2.start_of_memory_range > m1.start_of_memory_range;
    });
    for (auto memory : memories) {
      dump_memory(&minidump, &memory);
    }

  } else {
    printf("Error: can not parse the minidump.\n");
  }

  return 0;
}