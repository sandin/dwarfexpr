#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include <cstdint>

#include "retdec/dwarfparser/dwarf_file.h"


using namespace retdec::dwarfparser;

const char USAGE[] = "USAGE: dwarf2line [options] [addresses]\n"
                     " Options:\n"
                     "  -e --exe <executable>   Set the input filename\n"
                     "  -f --functions          Show function names\n";
 
int main(int argc, char** argv) 
{
    // parse args
    std::string input;
    bool print_func_name = false;
    std::vector<uint64_t> addresses;
    for (int i = 1; i< argc; ++i) {
        if (!strcmp(argv[i], "-e") || !strcmp(argv[i], "--exe")) {
            ++i;
            if (i >= argc) {
                printf("Error: missing `-e` arg.\n");
            }
            input = argv[i];
        } else if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--functions")) {
            print_func_name = true;
        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            printf("%s", USAGE);
            return 0;
        } else {
            std::string arg = argv[i];
            uint64_t addr = std::stoul(arg, 0, 16);
            addresses.emplace_back(addr);
        }
    }
    if (input.empty()) {
        printf("Error: missing the input `-e` arg.\n");
        printf("%s", USAGE);
        return -1;
    }
    if (addresses.empty()) {
        printf("Error: missing address arg.\n");
        printf("%s", USAGE);
        return -1;
    }

    DwarfFile dwarf_file(input);

    for (uint64_t address : addresses) {
        DwarfFilter filter = {"", "", address};
        dwarf_file.loadFile(filter);
        for (DwarfFunction* func : *dwarf_file.getFunctions()) {
            if (func->lowAddr <= address && address < func->highAddr) {
                if (print_func_name) {
                    printf("%s\n", func->getName().c_str());
                }
                printf("%s:%llu\n", func->file.c_str(), func->line);
            }
        }
    }

    return 0;
}