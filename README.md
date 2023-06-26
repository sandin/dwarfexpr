# dwarfexpr

Use dwarf debug info in ELF or dSYM files and minidump to restore the crash scene, include crashed function names, source code line number, and values of function parameters and local variables

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

First you need process the minidump file with breakpad's minidump_stackwalk tool to unwind the crash stack:

```
$ minidump_stackwalk -s minidump.dmp ./symbols > minidump.dmp.txt
```

NOTE: you must pass in the `-s` parameter to print out the stack memory 

And then convert minidump text file to dwarf context binary file use by dwarfexpr:

```
$ python tools/minidump_convertor.py -i minidump.dmp.txt -o minidump.dmp.bin
```

The crash context information is included in this binary file:
```
registers: (33)
  x00 = 0x0000000000000000 x01 = 0x0000000000000000 
  x02 = 0x0000006f48f2ef2c x03 = 0x0000000000000027 
  x04 = 0x0000006f2408b058 x05 = 0x0000006fc993ebc0 
  x06 = 0x0000006f5eb5356c x07 = 0x0000000000000000 
  x08 = 0x0000006f4c10f9c8 x09 = 0x9887759d0c52fb9c 
  x10 = 0x0000006fac41d838 x11 = 0x0000006f00000000 
  x12 = 0x00000001627e8958 x13 = 0x0000000000000001 
  x14 = 0x0000000000000007 x15 = 0x1a8133604dd396f4 
  x16 = 0x0000006f4bf13468 x17 = 0x0000006f48ef1370 
  x18 = 0x0000000000008000 x19 = 0x0000000000000000 
  x20 = 0x0000000000000008 x21 = 0x0000000000000000 
  x22 = 0x0000000000000000 x23 = 0x0000006f4c0c1ab8 
  x24 = 0x0000006f5eb53cc0 x25 = 0x0000006f5eb53cc0 
  x26 = 0x0000006f5eb53ff8 x27 = 0x00000000000fc000 
  x28 = 0x0000006f5ea5b000 x29 = 0x0000006f5eb53940 
  x30 = 0x0000006f48f80020 x31 = 0x0000006f5eb53910 
  x32 = 0x0000006f48f80034 

stack_memory: (64) 
  0x6f5eb53910 b8 1a 0c 4c 6f 00 00 00 00 68 08 24 6f 00 00 00 
  0x6f5eb53920 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
  0x6f5eb53930 08 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
  0x6f5eb53940 70 39 b5 5e 6f 00 00 00 60 0b f8 48 6f 00 00 00 
```

Everything is ready, then you can use dwarfexpr to restore the crash scene, you will see all parameters and local variables of the crashed function:

```
$ dwarf2line -l -p -f -C -c minidump.dmp.bin -e symbol_file 0x4026ab
```

output:
```
ImGui_ImplVulkan_DestroyDeviceObjects()
/home/lds/source/Imgui/imgui_impl_vulkan.cpp:928
params:
locals:
  ImGui_ImplVulkan_Data* bd (8 bytes) = nullptr
  ImGui_ImplVulkan_InitInfo* v (8 bytes) = nullptr
```

Source code of the crashed function:
```c++
924 void    ImGui_ImplVulkan_DestroyDeviceObjects()
925 {
926     ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
927     ImGui_ImplVulkan_InitInfo* v = &bd->VulkanInitInfo;
928     ImGui_ImplVulkanH_DestroyWindowRenderBuffers(v->Device, &bd->MainWindowRenderBuffers, v->Allocator);
```

With the restored local variables, we can easily find the reason for the crash, which is because the `bd` local variable is a null pointer.