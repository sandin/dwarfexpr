#include <iomanip>  // std::setw
#include <iostream>

#include "minidump/minidump.h"

using minidump::Minidump;

const char USAGE[] = "Usage: minidump_dump <minidump_file>";

void dump_thread(Minidump* minidump, MDRawThread* thread, bool crashed) {
  std::cout << "Thread " << thread->thread_id;
  if (crashed) {
    std::cout << "(crashed)";
  }
  std::cout << "\n";

  auto context = minidump->GetContext(thread->thread_id);
  if (context) {
    printf("context->GetCpuType(): %d\n", context->GetCpuType());
    switch (context->GetCpuType()) {
      case MD_CONTEXT_X86:
        break;
      case MD_CONTEXT_AMD64:
        break;
      case MD_CONTEXT_ARM:
        break;
      case MD_CONTEXT_ARM64:  // passthrough
      case MD_CONTEXT_ARM64_OLD: {
        auto ctx = context->GetContextARM64();
        for (int i = 0; i < MD_CONTEXT_ARM64_GPR_COUNT; ++i) {
          std::cout << "     " << "x" << i << " = "
                    << "0x" << std::hex << std::setw(16) << std::setfill('0')
                    << ctx->iregs[i];
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
        break;
    }
  }

  std::cout << "\n";
}

void dump_module(Minidump* minidump, MDRawModule* raw_module) {
  std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0')
            << raw_module->base_of_image << " - ";
  std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0')
            << (raw_module->base_of_image + raw_module->size_of_image);
  std::cout << "   " << minidump->ReadString(raw_module->module_name_rva)
            << "\n";
}

int main(int argc, char* argv[]) {
  printf("minidump dump\n");
  if (argc < 2) {
    printf(USAGE);
    return -1;
  }

  char* filename = argv[1];
  Minidump minidump(filename);
  bool success = minidump.Read();
  if (success) {
    auto system_info = minidump.GetSystemInfo();
    // Minidump::DumpSystemInfo(system_info);
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

    auto exception = minidump.GetException();
    std::cout << "Crash reason: ";
    std::cout << exception.exception_record.exception_code;
    std::cout << "\n";
    std::cout << "Crash address: ";
    std::cout << "0x" << std::hex
              << exception.exception_record.exception_address << std::dec
              << "\n\n";

    auto thread = minidump.GetCrashThread();
    if (thread) {
      dump_thread(&minidump, thread, true);
    }

    std::cout << "Loaded modules:\n";
    auto modules = minidump.GetModules();
    for (auto raw_module : modules) {
      dump_module(&minidump, &raw_module);
    }

  } else {
    printf("Error: can not parse the minidump.\n");
  }

  return 0;
}