# dwarfexpr

Develop some tools using DWARF debug info.

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

addr2line:

```
$ dwarf2line -f -e <elf_symbol_file> <address>
```

