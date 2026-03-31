//==========================================
// Modified from Matt Pietrek by F.GANNIER
// Microsoft Systems Journal, Feb 1997
// FILE: PEEXE.CPP
//==========================================
#include "stdafx.h"

#include <windows.h>
#include <stdlib.h>
#pragma hdrstop
#include "peexe.h"

PE_EXE::PE_EXE( LPCTSTR pszFileName ) : EXE_FILE( pszFileName )
{
    m_pNtHdr = 0;
	m_pNtHdr64 = (PIMAGE_NT_HEADERS64) m_pNtHdr;
	m_pDec = NULL;
	m_pdep = NULL;
	m_pPathCache = NULL;
	m_btested = FALSE;

    if ( FALSE == EXE_FILE::IsValid() )	return;
    if ( GetExeType() != exeType_PE )
    {
        m_errorType = errPE_EXE_FILE_INVALID_FORMAT; // for valid EXE_file
        return;
    }
    m_pNtHdr = MakePtr(PIMAGE_NT_HEADERS32, GetBase(), GetSecondaryHeaderOffset());
	m_pNtHdr64 = (PIMAGE_NT_HEADERS64) m_pNtHdr;
	EntryPointRAW = GetEntryPoint();
	EntryPointRVA = GetAddressOfEntryPoint();

}

PE_EXE::~PE_EXE( )
{ 
	if (m_pdep) { delete m_pdep; m_pdep = NULL; }
	if (m_pDec) { Detach(); delete m_pDec ; m_pDec = NULL; } 
}

BOOL PE_EXE::Attach(PVOID base, DWORD size)
{
	if (!EXE_FILE::Attach(base, size)) return FALSE;
	m_pNtHdr = MakePtr(PIMAGE_NT_HEADERS32, GetBase(), GetSecondaryHeaderOffset());
	m_pNtHdr64 = (PIMAGE_NT_HEADERS64) m_pNtHdr;	
	return TRUE;
}

void PE_EXE::Detach()
{	
	EXE_FILE::Detach();
	m_pNtHdr = MakePtr(PIMAGE_NT_HEADERS32, GetBase(), GetSecondaryHeaderOffset());
	m_pNtHdr64 = (PIMAGE_NT_HEADERS64) m_pNtHdr;
}

DWORD PE_EXE::GetEntryPoint( void )
{
	DWORD i=0, EntryPoint_RAW = 0;
    PIMAGE_SECTION_HEADER pSectHdr = IMAGE_FIRST_SECTION(m_pNtHdr);
	while (i < GetNumberOfSections())
	{
		i++;		
		if ((pSectHdr->VirtualAddress <= GetAddressOfEntryPoint()) && 
				(pSectHdr->SizeOfRawData + pSectHdr->VirtualAddress >= GetAddressOfEntryPoint()))
				{
					EntryPoint_RAW = GetAddressOfEntryPoint() - pSectHdr->VirtualAddress + pSectHdr->PointerToRawData;
					if (pSectHdr->Characteristics && 0x20) return EntryPoint_RAW ;
				}
	} 
	return EntryPoint_RAW ;
}

DWORD PE_EXE::GetDataDirectoryEntryRVA( DWORD id )
{   // Given a IMAGE_DIRECTORY_ENTRY_XXX value (see WINNT.H), retrive the RVA stored in the corresponding slot
    if ( id >= IMAGE_NUMBEROF_DIRECTORY_ENTRIES )
        return (DWORD)-1;
	if (Is64bits())
		return m_pNtHdr64->OptionalHeader.DataDirectory[id].VirtualAddress;
	return m_pNtHdr->OptionalHeader.DataDirectory[id].VirtualAddress;
}

PVOID PE_EXE::GetDataDirectoryEntryPointer( DWORD id )
{   // Given a IMAGE_DIRECTORY_ENTRY_XXX value (see WINNT.H), return a pointer to memory that corresponds to the RVA in the specified slot.
    if ( id >= IMAGE_NUMBEROF_DIRECTORY_ENTRIES )
        return (PVOID)-1;
	DWORD va;
	if (Is64bits())
		va = m_pNtHdr64->OptionalHeader.DataDirectory[id].VirtualAddress;
	else va = m_pNtHdr->OptionalHeader.DataDirectory[id].VirtualAddress;
    if ( !va )	return 0;
    return GetReadablePointerFromRVA( va );
}

DWORD PE_EXE::GetDataDirectoryEntrySize( DWORD id )
{	// Given a IMAGE_DIRECTORY_ENTRY_XXX value (see WINNT.H), retrive the	size value stored in the corresponding slot
    if ( id >= IMAGE_NUMBEROF_DIRECTORY_ENTRIES ) return (DWORD)-1;
	if (Is64bits())
		return m_pNtHdr64->OptionalHeader.DataDirectory[id].Size;
	return m_pNtHdr->OptionalHeader.DataDirectory[id].Size;
}

/***********  FG  ***************/
PVOID PE_EXE::GetReadablePointerFromVA( PVOID va )
{
	if (va == 0) return NULL;
	P_IB ImageBase = GetImageBase();
	DWORD rva = (DWORD)( (ULONG_PTR) va - ImageBase );
	return GetReadablePointerFromRVA( rva );
}
/***********  FG  ***************/

PVOID PE_EXE::GetReadablePointerFromRVA( DWORD rva ) 
{   // Given an RVA, translate it into a pointer within our linear memory mapping for the executable.
    DWORD fileOffset = RVAToFileOffset( rva );
    if ( (DWORD)-1 == fileOffset )	return 0;
	return (PVOID) GetTranslatedPtr( fileOffset);
}

DWORD PE_EXE::RVAToFileOffset( DWORD rva ) 
{   // Given an RVA, figure out which section encompasses it.  Next, using the PointerToRawData field for the found section, return an actual file offset that corresponds to the RVA
    PIMAGE_SECTION_HEADER pSectHdr = GetEnclosingSectionHeader( rva );
	if ( pSectHdr )
	{
		DWORD PtrtoRawD = pSectHdr->PointerToRawData;
/*
#ifdef _DEBUG 
		PtrtoRawD &= 0xFFFFFFE0
		if (PtrtoRawD != pSectHdr->PointerToRawData)
			AfxMessageBox("Raw Data Corrected", MB_OK|MB_ICONEXCLAMATION);
#endif /**/
		return PtrtoRawD + (rva - pSectHdr->VirtualAddress);
	}
/*	else
	{
		pSectHdr = IMAGE_FIRST_SECTION(m_pNtHdr);
		if (rva < pSectHdr->VirtualAddress) 
		{
#ifdef _DEBUG 
			AfxMessageBox("rva return low value", MB_OK|MB_ICONEXCLAMATION);
#endif
			return rva;
		}
	} /**/
	return (DWORD)-1;   // RVA not found in the section table... Ooops!
}


PIMAGE_SECTION_HEADER PE_EXE::GetSectionHeaderbyName(PSTR name)
{
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION( m_pNtHdr );
    for (unsigned i=0; i < m_pNtHdr->FileHeader.NumberOfSections; i++, section++ )
        if ( 0 == strncmp((char *)section->Name,name,IMAGE_SIZEOF_SHORT_NAME) )
            return section;
    return 0;
}

PIMAGE_SECTION_HEADER PE_EXE::GetEnclosingSectionHeader( DWORD rva )
{
    PIMAGE_SECTION_HEADER pSectHdr = IMAGE_FIRST_SECTION(m_pNtHdr);
	unsigned NbSct = GetNumberOfSections();

    for (unsigned  i=0; i < NbSct; i++, pSectHdr++ )
    {	// Is the RVA within this section?
        DWORD cbMaxOnDisk = min( pSectHdr->Misc.VirtualSize, pSectHdr->SizeOfRawData );
		if ( !cbMaxOnDisk ) cbMaxOnDisk = max( pSectHdr->Misc.VirtualSize, pSectHdr->SizeOfRawData );
        DWORD startSectRVA = pSectHdr->VirtualAddress;
        DWORD endSectRVA = startSectRVA + cbMaxOnDisk;

        if ( (rva >= startSectRVA) && (rva < (endSectRVA)))
            return pSectHdr;
    }
    return 0;
}

BOOL PE_EXE::IsCoded()
{
	if (!m_btested) 
		if (!IsAttached())
		{
			m_btested = TRUE;
			m_pDec = new DecPlugin (this);
			if (!m_pDec->IsComp()) 
			{
				delete m_pDec; m_pDec = NULL;
				return FALSE;
			}
			if (m_pDec->GetNewHeader() )
				Attach( m_pDec->GetNewHeader(), m_pDec->GetSize());
			return TRUE;
		} 
	return (m_pDec != NULL);
}

MODULE_DEPENDENCY_LIST *PE_EXE::GetDepends()
{
	if ( m_pdep == NULL )
	{
		m_pdep = new (MODULE_DEPENDENCY_LIST);
		if (m_pPathCache)
			m_pdep->SetPathCache(m_pPathCache);
		m_pdep->LoadPE(this);
	}
	return m_pdep;
}

void PE_EXE::Save(CString st)
{
	DWORD ecrit;
	HANDLE hFile = CreateFile(st, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)0);
	if (hFile)
	{
		WriteFile(hFile, (LPVOID) GetBase(), GetFileSize(), &ecrit, NULL);
		CloseHandle(hFile);
	}
}

bool PE_EXE::IsGoodExport()
{
	PIMAGE_EXPORT_DIRECTORY exportDir = GetExportsDesc();
	if ( !exportDir )   return FALSE;
	if (!IsValidPtr(( ULONG_PTR) exportDir )) return FALSE;
	PSTR	filename = (PSTR) GetReadablePointerFromRVA( exportDir->Name );
	if ( ((ULONG_PTR) filename > (ULONG_PTR) exportDir + GetFileSize()) || ((ULONG_PTR) filename < (ULONG_PTR) exportDir))
		return FALSE;
	return TRUE;
}