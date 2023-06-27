#ifndef MINIDUMP_MINIDUMP_H
#define MINIDUMP_MINIDUMP_H

#include <fstream>
#include <string>
#include <vector>

#include "minidump/breakpad/minidump_format.h"

namespace minidump {

class Minidump {
 public:
  explicit Minidump(const std::string& filepath) : filepath_(filepath), stream_(nullptr) {}
  virtual ~Minidump() {
    if (stream_) {
      delete stream_;
    }
  }

  bool Read();

 private:
  bool Open();
  bool ReadBytes(char* buffer, size_t buffer_size);
  bool SeekTo(off_t offset);

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

};  // class Minidump

}  // namespace minidump

#endif  // MINIDUMP_MINIDUMP_H
