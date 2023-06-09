# dwarfexpr

Develop some tools using DWARF debug info in ELF or dSYM file.

## Build

Requirements:
* [libdwarf](https://github.com/davea42/libdwarf-code)
* [cmake](https://cmake.org/)

Build on linux & macOS:
```
$ mkdir build && cd build
$ cmake ..
$ cmake --build .
```

## Usage

### Basic usage

Similar to addr2line:

```
$ dwarf2line -f -C -e dwarf2line 0x4026ab
```
output:
```
main
/home/lds/project/cpp/dwarfexpr/src/dwarf2line/dwarf2line.cpp:38
```

### Advanced usage

Print parameters and local variables of the target function:

```
$ dwarf2line -l -p -f -C -e dwarf2line 0x4026ab
```
output:
```
main
/home/lds/project/cpp/dwarfexpr/src/dwarf2line/dwarf2line.cpp:38
params:
  int argc (4 bytes)
  char** argv (8 bytes)
locals:
  string input (18446744073709551615 bytes)
  bool print_func_name (1 bytes)
  bool demangle (1 bytes)
  bool show_locals (1 bytes)
  bool show_params (1 bytes)
  bool debug (1 bytes)
  vector<unsigned long, std::allocator<unsigned long> > addresses (24 bytes)
  Dwarf_Debug dbg (8 bytes)
  Dwarf_Error error (8 bytes)
  int res (4 bytes)
  DwarfSearcher searcher (8 bytes)
```
