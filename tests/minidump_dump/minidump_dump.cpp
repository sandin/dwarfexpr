#include "minidump/minidump.h"

using minidump::Minidump;

const char USAGE[] = "Usage: minidump_dump <minidump_file>";

int main(int argc, char* argv[]) {
  printf("minidump dump\n");
  if (argc < 2) {
    printf(USAGE);
    return -1;
  }

  char* filename = argv[1];
  Minidump minidump(filename);
  bool success = minidump.Read();
  printf("parse minidump result: %d\n", success);

  return 0;
}