/**
 * @file include/retdec/dwarfparser/dwarf_search.h
 * @brief Search dwarf die
 */

#ifndef RETDEC_DWARFPARSER_DWARF_SEARCH_H
#define RETDEC_DWARFPARSER_DWARF_SEARCH_H

#include <string>

#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

namespace retdec {
namespace dwarfparser {

class DwarfCUContext {
	public:
		DwarfCUContext(Dwarf_Debug in_dbg, Dwarf_Addr in_pc, int in_is_info, int in_in_level, int in_cu_number) :
            dbg(in_dbg),
            target_pc(in_pc),
            cu_die(nullptr),
            func_die(nullptr),
            is_info(in_is_info),
            in_level(in_in_level),
            cu_number(in_cu_number) {
		}

		~DwarfCUContext() {
			if (cu_die != nullptr) {
				dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
			}
			if (func_die != nullptr) {
				dwarf_dealloc(dbg, func_die, DW_DLA_DIE);
			}
		}

    public:
		Dwarf_Debug dbg;
        Dwarf_Addr target_pc;

		Dwarf_Die cu_die;
		Dwarf_Die func_die;

        int is_info;
        int in_level;
        int cu_number;

        Dwarf_Unsigned cu_header_length = 0;
        Dwarf_Unsigned abbrev_offset = 0;
        Dwarf_Half     address_size = 0;
        Dwarf_Half     version_stamp = 0;
        Dwarf_Half     offset_size = 0;
        Dwarf_Half     extension_size = 0;
        Dwarf_Unsigned typeoffset = 0;
        Dwarf_Half     header_cu_type = DW_UT_compile;
        Dwarf_Sig8     signature = {0};
};

class DwarfSearcher {
    public:
        DwarfSearcher(Dwarf_Debug dbg);
        ~DwarfSearcher();

        bool searchFunction(Dwarf_Addr pc, Dwarf_Die* out_cu_die, Dwarf_Die* out_func_die, Dwarf_Error *errp);

    private:
        int searchInDieTree(DwarfCUContext* ctx, Dwarf_Die in_die, Dwarf_Error *errp);
        int searchInDie(DwarfCUContext* ctx, Dwarf_Die in_die, Dwarf_Error *errp);
        int matchFuncDie(DwarfCUContext* ctx, Dwarf_Die in_die, Dwarf_Error *errp);
        int matchCUDie(DwarfCUContext* ctx, Dwarf_Die in_die, Dwarf_Error *errp);

    private:
        Dwarf_Debug m_dbg;
  

}; // class DwarfSearcher

} // namespace dwarfparser
} // namespace retdec

#endif // RETDEC_DWARFPARSER_DWARF_SEARCH_H
