#include "minidump/minidump.h"

#include <cinttypes>
#include <cstring>  // memcpy, memset
#include <limits>

namespace minidump {

using std::ifstream;

// copy from: google_breakpad/src/src/processor/minidump.cc
static std::string UTF16ToUTF8(const std::vector<uint16_t>& in) {
  std::string out;

  // Set the string's initial capacity to the number of UTF-16 characters,
  // because the UTF-8 representation will always be at least this long.
  // If the UTF-8 representation is longer, the string will grow dynamically.
  out.reserve(in.size());

  for (std::vector<uint16_t>::const_iterator iterator = in.begin();
       iterator != in.end(); ++iterator) {
    // Get a 16-bit value from the input
    uint16_t in_word = *iterator;

    // Convert the input value (in_word) into a Unicode code point (unichar).
    uint32_t unichar;
    if (in_word >= 0xdc00 && in_word <= 0xdcff) {
      return "";
    } else if (in_word >= 0xd800 && in_word <= 0xdbff) {
      // High surrogate.
      unichar = (in_word - 0xd7c0) << 10;
      if (++iterator == in.end()) {
        return "";
      }
      uint32_t high_word = in_word;
      in_word = *iterator;
      if (in_word < 0xdc00 || in_word > 0xdcff) {
        return "";
      }
      unichar |= in_word & 0x03ff;
    } else {
      // The ordinary case, a single non-surrogate Unicode character encoded
      // as a single 16-bit value.
      unichar = in_word;
    }

    // Convert the Unicode code point (unichar) into its UTF-8 representation,
    // appending it to the out string.
    if (unichar < 0x80) {
      (out) += static_cast<char>(unichar);
    } else if (unichar < 0x800) {
      (out) += 0xc0 | static_cast<char>(unichar >> 6);
      (out) += 0x80 | static_cast<char>(unichar & 0x3f);
    } else if (unichar < 0x10000) {
      (out) += 0xe0 | static_cast<char>(unichar >> 12);
      (out) += 0x80 | static_cast<char>((unichar >> 6) & 0x3f);
      (out) += 0x80 | static_cast<char>(unichar & 0x3f);
    } else if (unichar < 0x200000) {
      (out) += 0xf0 | static_cast<char>(unichar >> 18);
      (out) += 0x80 | static_cast<char>((unichar >> 12) & 0x3f);
      (out) += 0x80 | static_cast<char>((unichar >> 6) & 0x3f);
      (out) += 0x80 | static_cast<char>(unichar & 0x3f);
    } else {
      return "";
    }
  }

  return out;
}

static void ConvertOldARM64Context(const MDRawContextARM64_Old& old,
                                   MDRawContextARM64* context) {
  context->context_flags = MD_CONTEXT_ARM64;
  if (old.context_flags & MD_CONTEXT_ARM64_INTEGER_OLD) {
    context->context_flags |=
        MD_CONTEXT_ARM64_INTEGER | MD_CONTEXT_ARM64_CONTROL;
  }
  if (old.context_flags & MD_CONTEXT_ARM64_FLOATING_POINT_OLD) {
    context->context_flags |= MD_CONTEXT_ARM64_FLOATING_POINT;
  }

  context->cpsr = old.cpsr;

  static_assert(sizeof(old.iregs) == sizeof(context->iregs),
                "iregs size mismatch");
  memcpy(context->iregs, old.iregs, sizeof(context->iregs));

  static_assert(sizeof(old.float_save.regs) == sizeof(context->float_save.regs),
                "float_save.regs size mismatch");
  memcpy(context->float_save.regs, old.float_save.regs,
         sizeof(context->float_save.regs));
  context->float_save.fpcr = old.float_save.fpcr;
  context->float_save.fpsr = old.float_save.fpsr;

  memset(context->bcr, 0, sizeof(context->bcr));
  memset(context->bvr, 0, sizeof(context->bvr));
  memset(context->wcr, 0, sizeof(context->wcr));
  memset(context->wvr, 0, sizeof(context->wvr));
}

//
// class Minidump
//

bool Minidump::Read() {
  if (!Open()) {
    printf("Error: can not find minidump file %s.\n", filepath_.c_str());
    return false;
  }

  if (!ReadHeader() || !ReadDirectoryList()) {
    return false;
  }

  for (const MDRawDirectory& directory : directories_) {
    switch (directory.stream_type) {
      case MD_THREAD_LIST_STREAM:
        ReadThreadListStream(directory);
        break;
      case MD_MODULE_LIST_STREAM:
        ReadModuleListStream(directory);
        break;
      case MD_MEMORY_LIST_STREAM:
        ReadMemoryListStream(directory);
        break;
      case MD_EXCEPTION_STREAM:
        ReadExceptionStream(directory);
        break;
      case MD_SYSTEM_INFO_STREAM:
        ReadSystemInfoStream(directory);
        break;
      case MD_LINUX_CPU_INFO:
        // TODO
        break;
      case MD_LINUX_PROC_STATUS:
        // TODO
        break;
      case MD_LINUX_CMD_LINE:
        // TODO
        break;
      case MD_LINUX_ENVIRON:
        // TODO
        break;
      case MD_LINUX_AUXV:
        // TODO
        break;
      case MD_LINUX_MAPS:
        // TODO
        break;
      case MD_LINUX_DSO_DEBUG:
        // TODO
        break;
      default:
        // TODO: unknown stream type
        break;
    };
  }

  return true;  // TODO
};

bool Minidump::ReadHeader() {
  if (!ReadBytes(reinterpret_cast<char*>(&header_), sizeof(MDRawHeader))) {
    printf("Error: bad format minidump file(can not read the header).\n");
    return false;
  }

  if (header_.signature != MD_HEADER_SIGNATURE) {
    // TODO: only support little-endian for now
    printf(
        "Error: bad format minidump file(signature mismatch, expect=%d, "
        "actual=%d.\n",
        MD_HEADER_SIGNATURE, header_.signature);
    return false;
  }

  uint16_t version = header_.version & 0x0000ffff;
  if (version != MD_HEADER_VERSION) {
    printf(
        "Error: bad format minidump file(version mismatch, expect=%d, "
        "actual=%d.\n",
        MD_HEADER_VERSION, version);
    return false;
  }

  return true;
}

// static
void Minidump::DumpHeader(const MDRawHeader& header) {
  printf("Header: version=%" PRId16 ", stream_count=%" PRIu32
         ", stream_directory_rva=0x%" PRIx32 ", time_date_stamp=%" PRIu32 "\n",
         header.version & 0x0000ffff, header.stream_count,
         header.stream_directory_rva, header.time_date_stamp);
}

bool Minidump::ReadDirectoryList() {
  if (!SeekTo(header_.stream_directory_rva)) {
    printf("Error: can not seek to stream_directory_rva, off=0x%" PRIx32 "\n",
           header_.stream_directory_rva);
    return false;
  }
  if (header_.stream_count > 1000) {
    printf("Error: too many stream, count=%" PRIu32 "\n", header_.stream_count);
    return false;
  }

  if (header_.stream_count > 0) {
    directories_.resize(header_.stream_count);
    if (!ReadBytes(reinterpret_cast<char*>(directories_.data()),
                   header_.stream_count * sizeof(MDRawDirectory))) {
      printf("Error: can not read all raw directories.\n");
      return false;
    }
  }
  return true;
}

// static
void Minidump::DumpDirectory(const MDRawDirectory& directory) {
  printf("Directory: stream_type=%" PRIu32 "(0x%" PRIx32
         "), location.rva=0x%" PRIx32 ", location.data_size=0x%" PRIx32 "\n",
         directory.stream_type, directory.stream_type, directory.location.rva,
         directory.location.data_size);
}

template <typename T>
static bool ReadStreamSingle(Minidump* minidump,
                             const MDRawDirectory& directory, T* stream) {
  if (!minidump->SeekTo(directory.location.rva)) {
    return false;
  }

  if (directory.location.data_size != sizeof(T)) {
    return false;
  }
  if (!minidump->ReadBytes(reinterpret_cast<char*>(stream), sizeof(T))) {
    return false;
  }

  return true;
}

template <typename T>
static bool ReadStream(Minidump* minidump, const MDRawDirectory& directory,
                       std::vector<T>* stream, size_t item_size,
                       size_t max_item_size) {
  if (stream->size() != 0) {
    printf("Error: expect only one module list stream.\n");
    return false;
  }

  if (!minidump->SeekTo(directory.location.rva)) {
    return false;
  }

  uint32_t item_count;
  if (directory.location.data_size < sizeof(item_count)) {
    return false;
  }
  if (!minidump->ReadBytes(reinterpret_cast<char*>(&item_count),
                           sizeof(item_count))) {
    return false;
  }
  if (item_count > std::numeric_limits<uint32_t>::max() / item_size) {
    return false;
  }
  if (directory.location.data_size !=
      sizeof(item_count) + item_count * item_size) {
    // may be padded with 4 bytes on 64bit ABIs for alignment
    if (directory.location.data_size ==
        sizeof(item_count) + 4 + item_count * item_size) {
      uint32_t padding;
      if (!minidump->ReadBytes(reinterpret_cast<char*>(&padding), 4)) {
        return false;
      }
    } else {
      return false;
    }
  }
  if (item_count > max_item_size) {
    printf("Error: too many items in the stream, count=%" PRIu32 "\n",
           item_count);
    return false;
  }

  if (item_count > 0) {
    stream->resize(item_count);
    for (size_t i = 0; i < item_count; ++i) {
      if (!minidump->ReadBytes(reinterpret_cast<char*>(&(*stream)[i]),
                               item_size)) {
        printf("Error: can not read the item at index %zu.\n", i);
        return false;
      }
    }
  }
  return true;
}

bool Minidump::ReadThreadListStream(const MDRawDirectory& directory) {
  if (!ReadStream<MDRawThread>(this, directory, &threads_, sizeof(MDRawThread),
                               10000)) {
    return false;
  }

  return true;
}

// static
void Minidump::DumpThread(const MDRawThread& thread) {
  printf("\tThread: thread_id=%" PRIu32 ", suspend_count=%" PRIu32
         ", priority_class=%" PRIu32 ", teb=%" PRIu64
         ", stack.start_of_memory_range=0x%" PRIx64
         ", stack.memory.rva=0x%" PRIx32 ", stack.memory.data_size=0x%" PRIx32
         ", stack.thread_context.rva=0x%" PRIx32
         ", stack.thread_context.data_size=0x%" PRIx32 "\n",
         thread.thread_id, thread.suspend_count, thread.priority_class,
         thread.teb, thread.stack.start_of_memory_range,
         thread.stack.memory.rva, thread.stack.memory.data_size,
         thread.thread_context.rva, thread.thread_context.data_size);
}

bool Minidump::ReadModuleListStream(const MDRawDirectory& directory) {
  if (!ReadStream<MDRawModule>(this, directory, &modules_, MD_MODULE_SIZE,
                               10000)) {
    return false;
  }

  return true;
}

// static
void Minidump::DumpModule(const MDRawModule& module) {
  printf("\tModule: base_of_image=0x%" PRIx64 ", size_of_image=0x%" PRIx32
         ", module_name=0x%" PRIx32 "\n",
         module.base_of_image, module.size_of_image, module.module_name_rva);
}

bool Minidump::ReadMemoryListStream(const MDRawDirectory& directory) {
  if (!ReadStream<MDMemoryDescriptor>(this, directory, &memories_,
                                      sizeof(MDMemoryDescriptor), 10000)) {
    return false;
  }

  return true;
}

// static
void Minidump::DumpMemory(const MDMemoryDescriptor& memory) {
  printf("\tMemory: start_of_memory_range=0x%" PRIx64 ", memory.rva=0x%" PRIx32
         ", memory.data_size=0x%" PRIx32 "\n",
         memory.start_of_memory_range, memory.memory.rva,
         memory.memory.data_size);
}

bool Minidump::ReadExceptionStream(const MDRawDirectory& directory) {
  if (!ReadStreamSingle<MDRawExceptionStream>(this, directory, &exception_)) {
    return false;
  }

  MinidumpContext* context = new MinidumpContext();
  if (!context->Read(this, exception_.thread_context)) {
    return false;
  }
  contexts_.insert(std::make_pair(exception_.thread_id, context));
  return true;
}

// static
void Minidump::DumpException(const MDRawExceptionStream& exception) {
  printf("\tException: thread_id=%" PRId32
         ", exception_record.exception_code=%" PRIu32
         ", exception_record.exception_flags=%" PRIu32
         ", exception_record.exception_record=%" PRIu64
         ", exception_record.exception_address=%" PRIu64
         ", exception_record.number_parameters=%" PRIu32
         ", thread_context.rva=0x%" PRIx32
         ", thread_context.data_size=0x%" PRIx32 "\n",
         exception.thread_id, exception.exception_record.exception_code,
         exception.exception_record.exception_flags,
         exception.exception_record.exception_record,
         exception.exception_record.exception_address,
         exception.exception_record.number_parameters,
         exception.thread_context.rva, exception.thread_context.data_size);
}

bool Minidump::ReadSystemInfoStream(const MDRawDirectory& directory) {
  if (!ReadStreamSingle<MDRawSystemInfo>(this, directory, &system_info_)) {
    return false;
  }

  return true;
}

// static
void Minidump::DumpSystemInfo(const MDRawSystemInfo& system_info) {
  printf("\tSystemInfo: processor_architecture=%" PRIu16
         ", processor_level=0x%" PRIu16 ", processor_revision=0x%" PRIu16
         ", number_of_processors=0x%" PRIu8 ", product_type=0x%" PRIu8
         ", major_version=0x%" PRIu32 ", minor_version=0x%" PRIu32
         ", build_number=0x%" PRIu32 ", platform_id=0x%" PRIu32
         ", csd_version_rva=0x%" PRIu32 ", suite_mask=0x%" PRIu16 "\n",
         system_info.processor_architecture, system_info.processor_level,
         system_info.processor_revision, system_info.number_of_processors,
         system_info.product_type, system_info.major_version,
         system_info.minor_version, system_info.build_number,
         system_info.platform_id, system_info.csd_version_rva,
         system_info.suite_mask);
}

std::string Minidump::ReadString(off_t offset) {
  // off_t origin_offset = stream_->tellg();
  if (!SeekTo(offset)) {
    return std::string("");
  }

  uint32_t size;
  if (!ReadBytes(reinterpret_cast<char*>(&size), sizeof(size))) {
    return std::string("");
  }
  if (size % 2 != 0) {
    return std::string("");
  }

  uint32_t utf16_words = size / 2;
  if (utf16_words > 4096) {
    return std::string("");
  }

  std::vector<uint16_t> string_utf16(utf16_words);
  if (utf16_words > 0) {
    if (!ReadBytes(reinterpret_cast<char*>(string_utf16.data()), size)) {
      return std::string("");
    }
  }

  // restore the origin offset
  // SeekTo(origin_offset);

  return UTF16ToUTF8(string_utf16);
}

bool Minidump::Open() {
  if (stream_) {
    printf("Error: this minidump file is already open.\n");
    return false;
  }
  stream_ = new ifstream(filepath_.c_str(), std::ios::in | std::ios::binary);
  if (!stream_ || !stream_->good()) {
    return false;
  }

  return true;
}

bool Minidump::ReadBytes(char* buffer, size_t buffer_size) {
  if (!stream_) {
    return false;
  }

  stream_->read(buffer, buffer_size);
  std::streamsize bytes_read = stream_->gcount();
  return static_cast<size_t>(bytes_read) == buffer_size;
}

bool Minidump::SeekTo(off_t offset) {
  if (!stream_) {
    return false;
  }

  stream_->seekg(offset, std::ios_base::beg);
  if (!stream_->good()) {
    return false;
  }
  return true;
}

MDRawThread* Minidump::GetCrashThread() const {
  return GetThread(exception_.thread_id);
}

MDRawThread* Minidump::GetThread(uint32_t thread_id) const {
  for (const MDRawThread& thread : threads_) {
    if (thread.thread_id == thread_id) {
      return const_cast<MDRawThread*>(&thread);
    }
  }
  return nullptr;
}

MinidumpContext* Minidump::GetCrashContext() const {
  return GetContext(exception_.thread_id);
}

MinidumpContext* Minidump::GetContext(uint32_t thread_id) const {
  if (contexts_.find(thread_id) != contexts_.end()) {
    return contexts_.at(thread_id);
  }
  return nullptr;
}

bool Minidump::GetMemory(uint64_t address, size_t size, char** buffer,
                         size_t* buffer_size) {
  for (const MDMemoryDescriptor& m : memories_) {
    uint64_t start_addr = m.start_of_memory_range;
    uint64_t end_addr = start_addr + m.memory.data_size;
    if (start_addr <= address && address < end_addr &&
        address + size <= end_addr) {
      char* buf = static_cast<char*>(malloc(m.memory.data_size));
      if (SeekTo(m.memory.rva) && ReadBytes(buf, m.memory.data_size)) {
        *buffer_size = m.memory.data_size;
        *buffer = buf;
        return true;
      }
      free(buf);
    }
  }
  return false;
}

bool Minidump::GetThreadStackMemory(MDRawThread* thread, char** buffer,
                                    size_t* buffer_size) {
  return GetMemory(thread->stack.start_of_memory_range,
                   thread->stack.memory.data_size, buffer, buffer_size);
}

void Minidump::FreeMemory(char* buffer) { free(buffer); }

//
// class MinidumpContext
//

MinidumpContext::~MinidumpContext() {
  uint32_t cpu_type = context_flags_ & MD_CONTEXT_CPU_MASK;
  switch (cpu_type) {
    case MD_CONTEXT_X86:
      if (context_.x86) {
        delete context_.x86;
      }
      break;
    case MD_CONTEXT_AMD64:
      if (context_.amd64) {
        delete context_.amd64;
      }
      break;
    case MD_CONTEXT_ARM:
      if (context_.arm) {
        delete context_.arm;
      }
      break;
    case MD_CONTEXT_ARM64:
      if (context_.arm64) {
        delete context_.arm64;
      }
      break;
    default:
      break;
  }
}

bool MinidumpContext::Read(Minidump* minidump,
                           const MDLocationDescriptor& loc) {
  if (!minidump->SeekTo(loc.rva)) {
    return false;
  }

  if (loc.data_size == sizeof(MDRawContextAMD64)) {
    context_.amd64 = new MDRawContextAMD64();
    if (!minidump->ReadBytes(reinterpret_cast<char*>(context_.amd64),
                             sizeof(MDRawContextAMD64))) {
      return false;
    }
    context_flags_ = context_.amd64->context_flags;
  } else if (loc.data_size == sizeof(MDRawContextPPC64)) {
    printf("Error: unsupported cpu arch: PPC64\n");
    return false;
  } else if (loc.data_size == sizeof(MDRawContextARM64_Old)) {
    uint64_t context_flags = 0;
    if (!minidump->ReadBytes(reinterpret_cast<char*>(&context_flags),
                             sizeof(context_flags))) {
      return false;
    }
    uint32_t cpu_type = context_flags & MD_CONTEXT_CPU_MASK;
    if (cpu_type != MD_CONTEXT_ARM64_OLD) {
      return false;
    }

    MDRawContextARM64_Old context_arm64_old = {};
    if (!minidump->ReadBytes(reinterpret_cast<char*>(&context_arm64_old),
                             sizeof(MDRawContextARM64_Old))) {
      return false;
    }

    context_.arm64 = new MDRawContextARM64();
    ConvertOldARM64Context(context_arm64_old, context_.arm64);
    context_flags_ = context_.arm64->context_flags;
  } else {
    if (!minidump->ReadBytes(reinterpret_cast<char*>(context_flags_),
                             sizeof(context_flags_))) {
      return false;
    }

    uint32_t cpu_type = context_flags_ & MD_CONTEXT_CPU_MASK;
    if (cpu_type == 0) {
      return false;
    }

    switch (cpu_type) {
      case MD_CONTEXT_X86:
        context_.x86 = new MDRawContextX86();
        if (!minidump->ReadBytes(reinterpret_cast<char*>(context_.x86),
                                 sizeof(MDRawContextX86))) {
          delete context_.x86;
          return false;
        }
        break;
      case MD_CONTEXT_AMD64:
        // should not happen
        break;
      case MD_CONTEXT_ARM:
        context_.arm = new MDRawContextARM();
        if (!minidump->ReadBytes(reinterpret_cast<char*>(context_.arm),
                                 sizeof(MDRawContextARM))) {
          delete context_.arm;
          return false;
        }
        break;
      case MD_CONTEXT_ARM64:
        context_.arm64 = new MDRawContextARM64();
        if (!minidump->ReadBytes(reinterpret_cast<char*>(context_.arm64),
                                 sizeof(MDRawContextARM64))) {
          delete context_.arm64;
          return false;
        }
        break;
      default:
        printf("Error: unsupported cpu arch: %d\n", cpu_type);
        return false;
    }
  }

  return true;
}

};  // namespace minidump