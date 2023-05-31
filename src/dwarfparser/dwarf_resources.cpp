/**
 * @file src/dwarfparser/dwarf_resources.cpp
 * @brief Implementation of classes representing resources.
 * @copyright (c) 2017 Avast Software, licensed under the MIT license
 */

#include <iostream>

#include "retdec/dwarfparser/dwarf_resources.h"

using namespace std;

namespace retdec {
namespace dwarfparser {

/**
 * @brief Print contents of this class.
 */
void DwarfResources::dump()
{
	cout << endl;
	cout << "==================== Resources ====================" << endl;
	cout << endl;

	cout << "Register mapping:" << endl;
	const char *s0 = "Idx";
	const char *s1 = "Dwarf num.";
	const char *s2 = "Array num.";
	const char *s3 = "Name";
	printf("\t%4s %10s %10s  %-30s\n", s0, s1, s2, s3);

	unsigned idx = 0;
	map<Dwarf_Half, RealReg>::iterator mIter;
	for(mIter=m_regMaps.begin(); mIter!=m_regMaps.end(); ++mIter)
	{
		printf("\t%4d %10d %10d  %-30s\n",
				idx,
				(*mIter).first,
				(*mIter).second.arrayNum,
				(*mIter).second.name.c_str()
		);
		idx++;
	}

	cout << endl;
}

/**
 * @brief Gets DWARF register number and returns its content.
 * @param n DWARF register number.
 * @return value of the given register.
 */
Dwarf_Signed DwarfResources::getReg(Dwarf_Half n)
{
	(void) n;
	return EMPTY_SIGNED;
}

/**
 * @brief Gets name of register array and number in register array and returns content of that register.
 * @param name is a name of register array.
 * @param number is a number in register array.
 * @return value of the given register.
 */
Dwarf_Signed DwarfResources::getReg(std::string name, Dwarf_Addr number)
{
	(void) name;
	(void) number;
	return EMPTY_SIGNED;
}

/**
 * @brief Gets address and returns its content.
 * @param a Address to get.
 * @return Value on the given address.
 * @note It use only one (always the first) address name space at the moment.
 */
Dwarf_Signed DwarfResources::getAddr(Dwarf_Addr a)
{
	(void) a;
	return EMPTY_SIGNED;
}

/**
 * @brief Gets content of program counter register.
 * @return Value of program counter.
 */
Dwarf_Addr DwarfResources::getPcReg()
{
	return EMPTY_SIGNED;
}

/**
 * @brief Gets DWARF register number and sets name of register array and
 *        its index number used in architecture.
 * @param reg Register number used by DWARF.
 * @param n   Name of register array in architecture to set.
 * @param a   Index number of register in register array to set.
 */
void DwarfResources::setReg(Dwarf_Half reg, string *n, Dwarf_Addr *a)
{
	map<Dwarf_Half, RealReg>::iterator iter;
	iter = m_regMaps.find(reg);

	if (iter == m_regMaps.end())
	{
		DWARF_ERROR("DwarfResources::setReg(): DWARF register num is not mapped to a register.");
	}
	else
	{
		RealReg rr = (*iter).second;
		*n = rr.name;
		*a = rr.arrayNum;
	}
}

/**
 * @brief Init mapping of DWARF register numbers to names using
 *        default -- well known mapping for architectures.
 * @param m Architecture which default mapping will be used.
 */
void DwarfResources::initMappingDefault(eDefaultMap m)
{

	switch (m)
	{

		//
		// Mapping is the same as the one generated by compiler.
		// MIPS-32.
		//
		case MIPS:
		{
			unsigned i = 0;
			string general = "gpregs";
			for (unsigned j=0; j<32; j++, i++)
				m_regMaps[i] = RealReg(general, j);

			// Mapping same as in files from compiler.
			// But according to tests, fpu registers begin on number 32.
			// Single precision.
			string fpu_s = "fpuregs_s";
			for (unsigned j=0; j<32; j++, i++)
				m_regMaps[i] = RealReg(fpu_s, j);

			// Double precision - on MIPS 32.
			string fpu_d = "fpuregs_d";
			i = 32;
			for (unsigned j=0; j<16; j++, i=i+2)
			{
				m_regMaps[i+i+1] = RealReg(fpu_d, i-32);
			}

			break;
		}

		//
		// Only first 16 general registers are mapped.
		// Name of register array was taken from ARM semantics specification (resource_name record).
		//
		case ARM:
		{
			unsigned i;
			string general = "regs";
			for (i=0; i<16; i++)
				m_regMaps[i] = RealReg(general, i);

			m_regMaps[128] = RealReg("spsr", 0);

			break;
		}

		//
		// Only 8 general registers (32-bit) are mapped.
		// Name of register array was taken from x86 semantics specification (resource_name record).
		//
		case X86:
		{

			unsigned i;
			string general = "gp32";
			for (i=0; i<8; i++)
				m_regMaps[i] = RealReg(general, i);

			break;
		}

		//
		// Default.
		//
		default:
		{
			DWARF_ERROR("DwarfResources::initMappingDefault: unrecognized type of default mapping used.");
			initMappingDefault(MIPS);
			break;
		}
	}
}

} // namespace dwarfparser
} // namespace retdec
