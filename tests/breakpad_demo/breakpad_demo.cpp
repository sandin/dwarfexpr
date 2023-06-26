#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "client/linux/handler/exception_handler.h"

#if defined(__GNUC__)
#define BREAKPAD_DEMO_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
// Seems to have been around since at least Visual Studio 2005
#define BREAKPAD_DEMO_NOINLINE __declspec(noinline)
#endif

#define CRASH_WITH_NULL_PTR()                                                  \
  do {                                                                         \
    char filename[PATH_MAX];                                                   \
    sprintf(filename, "./%s.dmp", __FUNCTION__);                               \
    FILE* fp = fopen(filename, "w");                                           \
    google_breakpad::MinidumpDescriptor descriptor(fileno(fp));                \
    google_breakpad::ExceptionHandler eh(descriptor, NULL, dumpCallback, NULL, \
                                         true /* not install */, -1);          \
    volatile int* a = (int*)(NULL);                                            \
    *a = 1;                                                                    \
  } while (0)

#define GEN_MINIDUMP()                                                         \
  do {                                                                         \
    char filename[PATH_MAX];                                                   \
    sprintf(filename, "./%s.dmp", __FUNCTION__);                               \
    FILE* fp = fopen(filename, "w");                                           \
    google_breakpad::MinidumpDescriptor descriptor(fileno(fp));                \
    google_breakpad::ExceptionHandler eh(descriptor, NULL, dumpCallback, NULL, \
                                         false /* not install */, -1);         \
    eh.WriteMinidump();                                                        \
    fclose(fp);                                                                \
  } while (0)

static bool dumpCallback(const google_breakpad::MinidumpDescriptor& descriptor,
                         void* context, bool succeeded) {
  printf("Dump path: %s\n", descriptor.path());
  return succeeded;
}

struct Struct1 {
  int field0;
  int field1;
  int field2;
  int field3;
};

BREAKPAD_DEMO_NOINLINE
void gen_minidump(uint32_t arg0, uint64_t arg1) {
  printf("param0: %lu, param1: %llu\n", arg0, arg1);

  uint32_t local0 = 0x12345678;
  uint64_t local1 = 0x1234567887654321;
  int32_t local2 = -1;
  int64_t local3 = -2;
  float local4 = 1.2f;
  double local5 = 1.2f;
  const char* local6 = "abc";
  std::string local7 = "xyz";
  int local8[] = {1, 3, 5, 7, 9};
  Struct1 local9 = {0, 1, 2, 3};

  printf("local0=%d, local1=%llu, local3=%d, local4=%f, local5=%f\n", local0,
         local1, local2, local3, local4, local5);
  printf("local6=%s, local7=%s, local8=%d, local9=%d\n", local6, local7.c_str(),
         local8[0], local9.field0);

  CRASH_WITH_NULL_PTR();
}

int main(int argc, char* argv[]) {
  printf("breakpad demo\n");

  gen_minidump(0xf0f1f2f3, 0xf0f1f2f3f4f5f6f7);
  return 0;
}