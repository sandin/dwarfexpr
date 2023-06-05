#include <algorithm>
#include <cstdlib>
#include <sstream>

#include "dwarfexpr/dwarf_utils.h" // getLowAndHighPc, getRnglistsBase
#include "dwarfexpr/dwarf_searcher.h"

/* Adding return codes to DW_DLV, relevant to our purposes here. */
#define NOT_THIS_CU 10
#define IN_THIS_CU 11
#define FOUND_SUBPROG 12

using namespace std;

namespace dwarfexpr {

DwarfSearcher::DwarfSearcher(Dwarf_Debug dbg) : m_dbg(dbg) {

}

DwarfSearcher::~DwarfSearcher() {
	// NOTE: DwarfSearcher does not own m_dbg, DO NOT FREE IT!
}

int DwarfSearcher::matchCUDie(DwarfCUContext* ctx, Dwarf_Die in_die, Dwarf_Error *errp) {
	int res = DW_DLV_NO_ENTRY;
    bool have_pc_range = false;
    Dwarf_Addr lowpc = 0;
    Dwarf_Addr highpc = 0;
    Dwarf_Off real_ranges_offset = 0;

	res = getLowAndHighPc(ctx->dbg, in_die, &have_pc_range, &lowpc, &highpc, errp);
    if (res == DW_DLV_OK) {
        if (have_pc_range) {
            if (ctx->target_pc < lowpc ||
                ctx->target_pc >= highpc) {
                /* We have no interest in this CU */
                res = NOT_THIS_CU;
            } else {
                /* td->td_cu_haslowhighpc = have_pc_range; */
                res = IN_THIS_CU;
            }
        }
    }

    Dwarf_Attribute ranges_attr;
    res = dwarf_attr(in_die, DW_AT_ranges, &ranges_attr, errp);
	if (res == DW_DLV_OK) {
        res = dwarf_global_formref(ranges_attr, &real_ranges_offset, errp);
        if (res != DW_DLV_OK) {
            dwarf_dealloc(ctx->dbg, ranges_attr, DW_DLA_ATTR);
            return res;
        }
        dwarf_dealloc(ctx->dbg, ranges_attr, DW_DLA_ATTR);

		Dwarf_Off cu_ranges_base = 0;
		res = getRnglistsBase(ctx->dbg, in_die, &cu_ranges_base, errp);
        /*  We can do get ranges now as we already saw
            ranges base above (if any). */
        int resr = 0;
        Dwarf_Ranges *ranges = 0;
        Dwarf_Signed  ranges_count =0;
        Dwarf_Unsigned  byte_count =0;
        Dwarf_Off     actualoffset = 0;
        Dwarf_Signed k = 0;
        bool done = false;

        resr = dwarf_get_ranges_b(ctx->dbg, real_ranges_offset,
            in_die,
            &actualoffset,
            &ranges,
            &ranges_count,
            &byte_count,
            errp);
        if (resr != DW_DLV_OK) {
            /* Something badly wrong here. */
            return res;
        }
        for (k = 0; k < ranges_count && !done; ++k) {
            Dwarf_Ranges *cur = ranges+k;
            //Dwarf_Addr lowpcr = 0;
            //Dwarf_Addr highpcr = 0;
            Dwarf_Addr baseaddr = cu_ranges_base;

            switch(cur->dwr_type) {
            case DW_RANGES_ENTRY:
                lowpc = cur->dwr_addr1 + baseaddr;
                highpc = cur->dwr_addr2 + baseaddr;
                //printf("CU range: [0x%llx - 0x%llx]\n", lowpc, highpc);
                if (ctx->target_pc < lowpc ||
                    ctx->target_pc >= highpc) {
                    /* We have no interest in this CU */
                    break;
                }
                done = true;
                res = IN_THIS_CU;
                break;
            case DW_RANGES_ADDRESS_SELECTION:
                baseaddr = cur->dwr_addr2;
                break;
            case DW_RANGES_END:
                break;
            default:
                res = DW_DLV_ERROR;
            }
        }
        dwarf_dealloc_ranges(ctx->dbg, ranges, ranges_count);
	}
	return res;
}

int DwarfSearcher::matchFuncDie(DwarfCUContext* ctx, Dwarf_Die in_die, Dwarf_Error *errp) {
	int res = DW_DLV_NO_ENTRY;
    bool have_pc_range = false;
    Dwarf_Addr lowpc = 0;
    Dwarf_Addr highpc = 0;
    //Dwarf_Off real_ranges_offset = 0;
    //Dwarf_Bool has_ranges_attr = false;

	res = getLowAndHighPc(ctx->dbg, in_die, &have_pc_range, &lowpc, &highpc, errp);
    if (res == DW_DLV_OK) {
        if (have_pc_range) {
            if (ctx->target_pc < lowpc ||
                ctx->target_pc >= highpc) {
                /* We have no interest in this subprogram */
                res = DW_DLV_OK;
            } else {
				ctx->func_die = in_die;
                res = FOUND_SUBPROG;
            }
        }
    }

	// TODO: Do we need to check the `DW_AT_ranges` attr for function?

	return res;
}

int DwarfSearcher::searchInDie(DwarfCUContext* ctx, Dwarf_Die in_die, Dwarf_Error *errp) {
    Dwarf_Half tag = 0;
    int res = 0;

    res = dwarf_tag(in_die, &tag, errp);
    if (res != DW_DLV_OK) {
		return res;
    }

    if (tag == DW_TAG_subprogram ||
        tag == DW_TAG_inlined_subroutine) {
        res = matchFuncDie(ctx, in_die, errp);
        if (res == FOUND_SUBPROG) {
			ctx->func_die = in_die;
            return res;
        }
        else if (res == DW_DLV_ERROR)  {
            return res;
        }
        else if (res ==  DW_DLV_NO_ENTRY) {
            /* impossible? */
            return res;
        }
        else if (res ==  NOT_THIS_CU) {
            /* impossible */
            return res;
        }
        else if (res ==  IN_THIS_CU) {
            /* impossible */
            return res;
        } else {
            /* DW_DLV_OK */
        }
        return DW_DLV_OK;
    } else if (tag == DW_TAG_compile_unit ||
        tag == DW_TAG_partial_unit ||
        tag == DW_TAG_type_unit) {

        if (ctx->in_level) {
            /*  Something badly wrong, CU DIEs are only
                at level 0. */
            return NOT_THIS_CU;
        }
        res = matchCUDie(ctx, in_die, errp);
        return res;
    } 
    /*  Keep looking */
    return DW_DLV_OK;
}

/*  Recursion, following DIE tree.
    On initial call in_die is a cu_die (in_level 0 )
*/
int DwarfSearcher::searchInDieTree(DwarfCUContext* ctx, Dwarf_Die in_die, Dwarf_Error *errp) {
    int res = DW_DLV_ERROR;
    Dwarf_Die cur_die = in_die;
    Dwarf_Die child = 0;

    res = searchInDie(ctx, in_die, errp);
	if (res == DW_DLV_ERROR) {
		return res;
    }
    if (res == DW_DLV_NO_ENTRY) {
        return res;
    }
    if (res == NOT_THIS_CU) {
        return res;
    }
    if (res == IN_THIS_CU) {
        /*  Fall through to examine details. */
    } else if (res == FOUND_SUBPROG) {
        return res;
    } else {
        /*  ASSERT: res == DW_DLV_OK */
        /*  Fall through to examine details. */
    }

    /*  Now look at the children of the incoming DIE */
    for (;;) {
		// Get child.
        Dwarf_Die sib_die = nullptr;
        res = dwarf_child(cur_die, &child, errp);
        if (res == DW_DLV_ERROR) {
			return res;
        }
		// Has child -> recursion.
        if (res == DW_DLV_OK) {
            int res2 = 0;

			ctx->in_level += 1;
            res2 = searchInDieTree(ctx, child, errp);
            if (child != ctx->cu_die &&
                child != ctx->func_die) {
                /* No longer need 'child' die. */
                dwarf_dealloc(ctx->dbg, child, DW_DLA_DIE);
            }
            if (res2 == FOUND_SUBPROG) {
                return res2;
            }
            else if (res2 == IN_THIS_CU) {
                /* fall thru */
            }
            else if (res2 == NOT_THIS_CU) {
                return res2;
            }
            else if (res2 == DW_DLV_ERROR) {
                return res2;
            }
            else if (res2 == DW_DLV_NO_ENTRY) {
                /* fall thru */
            }
            else { /* DW_DLV_OK */
                /* fall thru */
            }
            child = 0;
        }

		// No entry -> no child.
        res = dwarf_siblingof_b(ctx->dbg, cur_die, ctx->is_info, &sib_die, errp);
        if (res == DW_DLV_ERROR) {
			return res;
        }
        if (res == DW_DLV_NO_ENTRY) {
            /* Done at this level. */
			ctx->in_level -= 1;
            break;
        }
        /* res == DW_DLV_OK */
        if (cur_die != in_die) {
            if (child != ctx->cu_die &&
                child != ctx->func_die) {
                dwarf_dealloc(ctx->dbg, cur_die, DW_DLA_DIE);
            }
            cur_die = nullptr;
        }
        cur_die = sib_die;
		res = searchInDie(ctx, cur_die, errp);
        if (res == DW_DLV_ERROR) {
            return res;
        }
        else if (res == DW_DLV_OK) {
            /* fall through */
			// continue;
        }
        else if (res == FOUND_SUBPROG) {
            return res;
        }
        else if (res == NOT_THIS_CU) {
            /* impossible */
        }
        else if (res == IN_THIS_CU) {
            /* impossible */
        } else  {/* res == DW_DLV_NO_ENTRY */
            /* impossible */
            /* fall through */
        }
    }
	return DW_DLV_OK;
}

bool DwarfSearcher::searchFunction(Dwarf_Addr pc, Dwarf_Die* out_cu_die, Dwarf_Die* out_func_die, Dwarf_Error *errp) {
	// loop all cu
    int cu_number = 0;
    for (;;++cu_number) {
		DwarfCUContext ctx(m_dbg, pc, 1 /* is_info */, 0 /* in_level */, cu_number);
        Dwarf_Die no_die = 0;
        int res = DW_DLV_ERROR;

        res = dwarf_next_cu_header_d(m_dbg, ctx.is_info, &ctx.cu_header_length,
            &ctx.version_stamp, &ctx.abbrev_offset,
            &ctx.address_size, &ctx.offset_size,
            &ctx.extension_size,&ctx.signature,
            &ctx.typeoffset, 0,
            &ctx.header_cu_type,errp);
        if (res == DW_DLV_ERROR) {
			return false;
        }
        if (res == DW_DLV_NO_ENTRY) {
            /* Done. */
			return false;
        }

        /* The CU will have a single sibling, a cu_die. */
        res = dwarf_siblingof_b(m_dbg, no_die, ctx.is_info, &ctx.cu_die,errp);
        if (res == DW_DLV_ERROR) {
			return false;
        }
        if (res == DW_DLV_NO_ENTRY) {
            /* Impossible case. */
			return false;
        }

        res = searchInDieTree(&ctx, ctx.cu_die, errp);
        if (res == FOUND_SUBPROG) {
			// transfer ownership, the caller is responsible for free them
			*out_cu_die = ctx.cu_die; 
			ctx.cu_die = nullptr;
			*out_func_die = ctx.func_die; 
			ctx.func_die = nullptr;
			return true;
        }
        else if (res == IN_THIS_CU) {
			return false;
        }
        else if (res == NOT_THIS_CU) {
            /* This is normal. Keep looking */
        }
        else if (res == DW_DLV_ERROR) {
			return false;
        }
        else if (res == DW_DLV_NO_ENTRY) {
            /* This is odd. Assume normal. */
        } else {
            /* DW_DLV_OK. normal. */
        }
		// free ctx.cu_die
    }

	return false;
}

} // namespace dwarfexpr
