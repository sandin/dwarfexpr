#include "minidump/minidump.h"

#include <cinttypes>

namespace minidump {

using std::ifstream;

bool Minidump::Read() {
  if (!Open()) {
    printf("Error: can not find minidump file %s.\n", filepath_.c_str());
    return false;
  }

  if (!ReadHeader() || !ReadDirectoryList()) {
    return false;
  }

  for (const MDRawDirectory& directory : directories_) {
    printf("Directory: stream_type=%" PRIu32 ", location.rva=0x%" PRIx32
           ", location.data_size=0x%" PRIx32 "\n",
           directory.stream_type, directory.location.rva,
           directory.location.data_size);
    switch (directory.stream_type) {
      case MD_THREAD_LIST_STREAM:
        if (!ReadThreadListStream(directory)) {
          return false;
        }
        break;
      case MD_MODULE_LIST_STREAM:
        if (!ReadModuleListStream(directory)) {
          return false;
        }
        break;
      case MD_MEMORY_LIST_STREAM:
        if (!ReadMemoryListStream(directory)) {
          return false;
        }
        break;
      case MD_EXCEPTION_STREAM:
        if (!ReadExceptionStream(directory)) {
          return false;
        }
        break;
      case MD_SYSTEM_INFO_STREAM:
        if (!ReadSystemInfoStream(directory)) {
          return false;
        }
        break;
      default:
        // TODO: unknown stream type
        break;
    };
  }

  return false;  // TODO:
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

  printf("Header: version=%" PRId16 ", stream_count=%" PRIu32
         ", stream_directory_rva=0x%" PRIx32 ", time_date_stamp=%" PRIu32 "\n",
         version, header_.stream_count, header_.stream_directory_rva,
         header_.time_date_stamp);

  return true;
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
                   sizeof(MDRawDirectory) * header_.stream_count)) {
      printf("Error: can not read all raw directories.\n");
      return false;
    }
  }
  return true;
}

bool Minidump::ReadThreadListStream(const MDRawDirectory& directory) {
  return false;  // TODO
}

bool Minidump::ReadModuleListStream(const MDRawDirectory& directory) {
  return false;  // TODO
}

bool Minidump::ReadMemoryListStream(const MDRawDirectory& directory) {
  return false;  // TODO
}

bool Minidump::ReadExceptionStream(const MDRawDirectory& directory) {
  return false;  // TODO
}

bool Minidump::ReadSystemInfoStream(const MDRawDirectory& directory) {
  return false;  // TODO
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

};  // namespace minidump