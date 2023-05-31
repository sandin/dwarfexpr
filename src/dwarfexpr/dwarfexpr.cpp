#include <iostream>
#include <string>

#include "retdec/dwarfparser/dwarf_file.h"


using namespace retdec::dwarfparser;

const char USAGE[] = "USAGE: dwarfexpor <elf_symbol_file>" "\n";
 
int main(int argc, char** argv) 
{
    if (argc < 2) {
        printf("%s", USAGE);
        exit(-1);
    }

    std::string filename = argv[1];
    printf("parse dwarf file: %s\n", filename.c_str());
    DwarfFile dwarf_file(filename);

    DwarfFilter filter = {"", "", 0};
    dwarf_file.loadFile(filter);

    for (DwarfCU* cu : *dwarf_file.getCUs()) {
        printf("cu: %s\n", cu->getName().c_str());
        for (size_t i = 0; i < cu->srcFilesCount(); ++i) {
            printf("\tfile: %s\n", cu->getSrcFile(i)->c_str());
        }
    }

    for (DwarfFunction* func : *dwarf_file.getFunctions()) {
        printf("func: %s\n", func->getName().c_str());
        printf("\tfile: %s, line: %llu\n", func->file.c_str(), func->line);
        printf("\tlowAddr: 0x%llx, highAddr: 0x%llx\n", func->lowAddr, func->highAddr);
        for (DwarfVar* var : *func->getParams()) {
            printf("\tparam: %s\n", var->getName().c_str());
        }
        for (DwarfVar* var : *func->getVars()) {
            printf("\tvar: %s\n", var->getName().c_str());
        }
    }

    return 0;
}