#ifndef MINIDUMP_MINIDUMP_H
#define MINIDUMP_MINIDUMP_H

#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "minidump/breakpad/minidump_format.h"

namespace minidump {

class MinidumpContext;  // forward declaration

class Minidump {
 public:
  explicit Minidump(const std::string& filepath)
      : filepath_(filepath),
        stream_(nullptr),
        header_(),
        exception_(),
        system_info_() {}
  virtual ~Minidump() {
    if (stream_) {
      delete stream_;
    }
    for (const auto& p : contexts_) {
      delete p.second; // TODO:
    }
  }

  bool Read();

  std::string ReadString(off_t offset);

  bool ReadBytes(char* buffer, size_t buffer_size);
  bool SeekTo(off_t offset);

  MDRawThread* GetCrashThread() const;
  MDRawThread* GetThread(uint32_t thread_id) const;

  MinidumpContext* GetCrashContext() const;
  MinidumpContext* GetContext(uint32_t thread_id) const;

  bool GetMemory(uint64_t address, size_t size, char** buffer,
                 size_t* buffer_size);
  void FreeMemory(char* buffer);

  const MDRawHeader& GetHeader() const { return header_; }
  const std::vector<MDRawDirectory>& GetDirectories() const { return directories_; }
  const std::vector<MDRawThread>& GetThreads() const { return threads_; }
  const std::vector<MDRawModule>& GetModules() const { return modules_; }
  const std::vector<MDMemoryDescriptor>& GetMemories() const { return memories_; }
  const MDRawExceptionStream& GetException() const { return exception_; }
  const MDRawSystemInfo& GetSystemInfo() const { return system_info_; }

  static void DumpHeader(const MDRawHeader& header);
  static void DumpDirectory(const MDRawDirectory& directory);
  static void DumpThread(const MDRawThread& thread);
  static void DumpModule(const MDRawModule& module);
  static void DumpMemory(const MDMemoryDescriptor& memory);
  static void DumpException(const MDRawExceptionStream& exception);
  static void DumpSystemInfo(const MDRawSystemInfo& system_info);

 private:
  bool Open();

  bool ReadHeader();
  bool ReadDirectoryList();

  bool ReadThreadListStream(const MDRawDirectory& directory);
  bool ReadModuleListStream(const MDRawDirectory& directory);
  bool ReadMemoryListStream(const MDRawDirectory& directory);
  bool ReadExceptionStream(const MDRawDirectory& directory);
  bool ReadSystemInfoStream(const MDRawDirectory& directory);

  const std::string filepath_;
  std::istream* stream_;
  MDRawHeader header_;
  std::vector<MDRawDirectory> directories_;
  std::vector<MDRawThread> threads_;
  std::vector<MDRawModule> modules_;
  std::vector<MDMemoryDescriptor> memories_;
  MDRawExceptionStream exception_;
  MDRawSystemInfo system_info_;
  std::map<uint32_t, MinidumpContext*> contexts_;

};  // class Minidump

class MinidumpContext {
 public:
  MinidumpContext() : context_({nullptr}), context_flags_(0) {}
  ~MinidumpContext();

  bool Read(Minidump* minidump, const MDLocationDescriptor& loc);
  uint32_t GetCpuType() const { return context_flags_ & MD_CONTEXT_CPU_MASK; }

  const MDRawContextX86* GetContextX86() const { return context_.x86; };
  const MDRawContextAMD64* GetContextAMD64() const { return context_.amd64; };
  const MDRawContextARM* GetContextARM() const { return context_.arm; };
  const MDRawContextARM64* GetContextARM64() const { return context_.arm64; };

  bool GetInstructionPointer(uint64_t* ip) const;
  bool GetStackPointer(uint64_t* sp) const;
  bool GetFramePointer(uint64_t* fp) const;

 private:
  union {
    MDRawContextX86* x86;
    MDRawContextAMD64* amd64;
    MDRawContextARM* arm;
    MDRawContextARM64* arm64;
  } context_;

  uint32_t context_flags_ = 0;
};  // class MinidumpContext

}  // namespace minidump

#endif  // MINIDUMP_MINIDUMP_H
