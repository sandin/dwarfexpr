#include "dwarfexpr/dwarf_utils.h"

#include <cstdlib>
#include <string.h> // strdup
#include <limits> // std::numeric_limits

using namespace std;

namespace dwarfexpr {

#define DWARF_ERROR(s) printf("%s", s.c_str())
constexpr Dwarf_Unsigned MAX_DWARF_UNSIGNED = std::numeric_limits<Dwarf_Unsigned>::max();

/**
 * @brief Get address from attribute.
 * @param attr Address class attribute.
 * @return Address represented by the attribute.
 */
Dwarf_Addr getAttrAddr(Dwarf_Attribute attr, Dwarf_Addr def_val)
{
	Dwarf_Error error = nullptr;
	Dwarf_Addr retVal = 0;

	if (dwarf_formaddr(attr, &retVal, &error) == DW_DLV_OK)
	{
		return retVal;
	}
	else
	{
		DWARF_ERROR(getDwarfError(error));
		return def_val;
	}
}

/**
 * @brief Get value of the attribute.
 * @param attr Constant class attribute.
 * @return Value of the attrinute.
 */
Dwarf_Unsigned getAttrNumb(Dwarf_Attribute attr, Dwarf_Unsigned def_val)
{
	Dwarf_Error error = nullptr;

	Dwarf_Unsigned uRetVal = 0;
	if (dwarf_formudata(attr, &uRetVal, &error) == DW_DLV_OK)
	{
		return uRetVal;
	}

	Dwarf_Signed sRetVal = 0;
	if (dwarf_formsdata(attr, &sRetVal, &error) == DW_DLV_OK)
	{
		return sRetVal;
	}

	DWARF_ERROR(getDwarfError(error));
	return def_val;
}

/**
 * @brief Get string from the attribute.
 * @param attr String class attribute.
 * @return String containing the value of the attribute.
 */
string getAttrStr(Dwarf_Attribute attr, string def_val)
{
	Dwarf_Error error = nullptr;

	char *name = nullptr;
	if (dwarf_formstring(attr, &name, &error) == DW_DLV_OK)
	{
		return string(name);
	}
	else
	{
		DWARF_ERROR(getDwarfError(error));
		return def_val;
	}
}

/**
 * @brief Get reference from the attribute.
 * @param attr Reference class attribute.
 * @return Offset represented by the attribute.
 */
Dwarf_Off getAttrRef(Dwarf_Attribute attr, Dwarf_Off def_val)
{
	Dwarf_Error error = nullptr;
	Dwarf_Bool is_info = true;

	Dwarf_Off retVal;
	if (dwarf_formref(attr, &retVal, &is_info, &error) == DW_DLV_OK)
	{
		return retVal;
	}
	else
	{
		DWARF_ERROR(getDwarfError(error));
		return def_val;
	}
}

/**
 * @brief Get global reference from the attribute.
 * @param attr Reference or other section-references class
 *        attribute.
 * @return Global offset represented by the attribute.
 */
Dwarf_Off getAttrGlobalRef(Dwarf_Attribute attr, Dwarf_Off def_val)
{
	Dwarf_Error error = nullptr;

	Dwarf_Off retVal;
	if (dwarf_global_formref(attr, &retVal, &error) == DW_DLV_OK)
	{
		return retVal;
	}
	else
	{
		DWARF_ERROR(getDwarfError(error));
		return def_val;
	}
}

/**
 * @brief Get flag from the attribute.
 * @param attr Attribute.
 * @return True if attribute has a non-zero value, else false.
 */
Dwarf_Bool getAttrFlag(Dwarf_Attribute attr)
{
	Dwarf_Error error = nullptr;

	Dwarf_Bool retVal;
	if (dwarf_formflag(attr, &retVal, &error) == DW_DLV_OK)
	{
		return retVal;
	}
	else
	{
		DWARF_ERROR(getDwarfError(error));
		return false;
	}
}

/**
 * @brief Get block from the attribute.
 * @param attr Block class attribute.
 * @return Pointer to block structure represented by the attribute,
 */
Dwarf_Block *getAttrBlock(Dwarf_Attribute attr)
{
	Dwarf_Error error = nullptr;

	Dwarf_Block *retVal = nullptr;
	if (dwarf_formblock(attr, &retVal, &error) == DW_DLV_OK)
	{
		return retVal;
	}
	else
	{
		DWARF_ERROR(getDwarfError(error));
		return nullptr;
	}
}

/**
 * @brief Get 8 byte signature from the attribute.
 * @param attr Attribute of DW_FORM_ref_sig8 form.
 * @return 8 byte signature represented by the attribute.
 */
Dwarf_Sig8 getAttrSig(Dwarf_Attribute attr)
{
	Dwarf_Error error = nullptr;

	Dwarf_Sig8 retVal;
	if (dwarf_formsig8(attr, &retVal, &error) == DW_DLV_OK)
	{
		return retVal;
	}
	else
	{
		DWARF_ERROR(getDwarfError(error));
		return Dwarf_Sig8();
	}
}

/**
 * @brief Gets length of the location expression and pointer to
 *        the bytes of the location expression from the attribute.
 * @param attr    Attribute of DW_FORM_exprloc form.
 * @param exprlen Pointer to location expression length to set.
 * @param ptr     Pointer to location expression pointer to set.
 */
void getAttrExprLoc(Dwarf_Attribute attr, Dwarf_Unsigned *exprlen,
		Dwarf_Ptr *ptr)
{
	Dwarf_Error error = nullptr;

	if (dwarf_formexprloc(attr, exprlen, ptr, &error) != DW_DLV_OK)
	{
		DWARF_ERROR(getDwarfError(error));
		// non-recoverable --  exit(1) ???
	}
}

/**
 * @brief Gets libdwarf error message to provided error code.
 * @param error Error code.
 * @return Error message as strings.
 */
string getDwarfError(Dwarf_Error &error)
{
	return string(dwarf_errmsg(error));
}

/**
 * @brief Gets DIE from provided offset.
 * @param dbg Debug file.
 * @param off Offset.
 * @param die Found die.
 * @return True if success, false otherwise.
 */
bool getDieFromOffset(Dwarf_Debug dbg, Dwarf_Off off, Dwarf_Die &die)
{
	bool is_info = true;
	Dwarf_Error error;
	int res = dwarf_offdie_b(dbg, off, is_info, &die, &error);

	if (res == DW_DLV_ERROR)
	{
		DWARF_ERROR(getDwarfError(error));
		return false;
	}

	if (res == DW_DLV_NO_ENTRY)
	{
		return false;
	}

	return true;
}

int getLowAndHighPc(Dwarf_Debug dbg, Dwarf_Die die, bool* have_pc_range, Dwarf_Addr* lowpc_out, Dwarf_Addr* highpc_out, Dwarf_Error* error) {
    Dwarf_Addr hipc = 0;
    int res = 0;
    Dwarf_Half form = 0;
    enum Dwarf_Form_Class formclass = DW_FORM_CLASS_UNKNOWN;

    *have_pc_range = false;
    res = dwarf_lowpc(die,lowpc_out,error);
    if (res == DW_DLV_OK) {
        res = dwarf_highpc_b(die,&hipc,&form,&formclass,error);
        if (res == DW_DLV_OK) {
            if (formclass == DW_FORM_CLASS_CONSTANT) {
                hipc += *lowpc_out;
            }
            *highpc_out = hipc;
            *have_pc_range = true;
            return DW_DLV_OK;
        }
    }
    /*  Cannot check ranges yet, we don't know the ranges base
        offset yet. */
    return DW_DLV_NO_ENTRY;
}

int getRnglistsBase(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Off* base_out, Dwarf_Error* error) {
	int res;
	*base_out = 0;

	Dwarf_Attribute attr;
	res = dwarf_attr(die, DW_AT_rnglists_base, &attr, error);
	if (res != DW_DLV_OK) {
		res = dwarf_attr(die, DW_AT_GNU_ranges_base, &attr, error);
		if (res != DW_DLV_OK) {
			dwarf_dealloc(dbg, attr, DW_DLA_LIST);
			return res;
		}
	}
	// else: res == DW_DLV_OK
    res = dwarf_global_formref(attr, base_out, error);
	dwarf_dealloc(dbg, attr, DW_DLA_LIST);
	return res;
}

std::string getDeclFile(Dwarf_Debug dbg, Dwarf_Die cu_die, Dwarf_Die func_die, std::string def_val) {
	Dwarf_Error error = nullptr;
   	Dwarf_Attribute file_attr = nullptr;

    if (dwarf_attr(func_die, DW_AT_decl_file, &file_attr, &error) == DW_DLV_OK) {
        Dwarf_Unsigned fileNum = getAttrNumb(file_attr, MAX_DWARF_UNSIGNED);
    	dwarf_dealloc(dbg, file_attr, DW_DLA_ATTR);

        if (fileNum != MAX_DWARF_UNSIGNED) {
			Dwarf_Signed fileIdx = fileNum - 1;

            // Get all source files of cu.
            char **srcFiles;
            Dwarf_Signed fileCnt;
            if (dwarf_srcfiles(cu_die, &srcFiles, &fileCnt, &error) == DW_DLV_OK) {
                if (fileIdx < fileCnt) {
                    std::string retVal = srcFiles[fileIdx];
                	dwarf_dealloc(dbg, srcFiles, DW_DLA_LIST);
					return retVal;
                }
                dwarf_dealloc(dbg, srcFiles, DW_DLA_LIST);
            } 
        }
    } 

	DWARF_ERROR(getDwarfError(error));
	return def_val;
}

Dwarf_Unsigned getDeclLine(Dwarf_Debug dbg, Dwarf_Die func_die, Dwarf_Unsigned def_val) {
	Dwarf_Error error = nullptr;
	Dwarf_Attribute line_attr = nullptr;
	Dwarf_Unsigned retVal = 0;

	if (dwarf_attr(func_die, DW_AT_decl_line, &line_attr, &error) == DW_DLV_OK) {
		retVal = getAttrNumb(line_attr, def_val);
		dwarf_dealloc(dbg, line_attr, DW_DLA_ATTR);
		return retVal;
	} else {
		DWARF_ERROR(getDwarfError(error));
		return def_val;
	}
}

} // namespace dwarfexpr
