//==========================================
// clrdump.h - .NET / CLR managed-assembly dumper
// Renders IMAGE_COR20_HEADER + metadata-root state for the m_clr tab,
// and builds the AssemblyRef tree for the m_clr_deps tab.
//==========================================
#ifndef __CLRDUMP_H__
#define __CLRDUMP_H__

#include "..\PEexe\PEEXE.H"

// Produce the CLR-header / metadata summary for the m_clr tab.
// Caller has already verified pe.HasClrHeader() is TRUE.
CStringA DumpClrHeader(PE_EXE &pe);

// Populate the Assembly-References tree for the m_clr_deps tab.
// The passed tree is freshly constructed (no existing items).
void BuildAssemblyRefTree(PE_EXE &pe, CTreeCtrl &tree);

#endif
