#ifndef DWARFEXPR_DWARF_UTILS_H
#define DWARFEXPR_DWARF_UTILS_H

#include <string>

#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

namespace dwarfexpr {

// from retdec/dwarfparser/dwarf_utils.h

Dwarf_Addr getAttrAddr(Dwarf_Attribute attr, Dwarf_Addr def_val);
Dwarf_Unsigned getAttrNumb(Dwarf_Attribute attr, Dwarf_Unsigned def_val);
std::string getAttrStr(Dwarf_Attribute attr, std::string def_val);
Dwarf_Off getAttrRef(Dwarf_Attribute attr, Dwarf_Off def_val);
Dwarf_Off getAttrGlobalRef(Dwarf_Attribute attr, Dwarf_Off def_val);
Dwarf_Bool getAttrFlag(Dwarf_Attribute attr);
Dwarf_Block *getAttrBlock(Dwarf_Attribute attr);
Dwarf_Sig8 getAttrSig(Dwarf_Attribute attr);
void getAttrExprLoc(Dwarf_Attribute attr, Dwarf_Unsigned *exprlen, Dwarf_Ptr *ptr);

std::string getDwarfError(Dwarf_Error &error);

bool getDieFromOffset(Dwarf_Debug dbg, Dwarf_Off off, Dwarf_Die &die);

// end 

int getLowAndHighPc(Dwarf_Debug dbg, Dwarf_Die die, bool* have_pc_range, Dwarf_Addr* lowpc_out, Dwarf_Addr* highpc_out, Dwarf_Error* error);
int getRnglistsBase(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Off* base_out, Dwarf_Error* error);

std::string getDeclFile(Dwarf_Debug dbg, Dwarf_Die cu_die, Dwarf_Die func_die, std::string def_val);
Dwarf_Unsigned getDeclLine(Dwarf_Debug dbg, Dwarf_Die func_die, Dwarf_Unsigned def_val);

} // namespace dwarfexpr

#endif // DWARFEXPR_DWARF_UTILS_H
