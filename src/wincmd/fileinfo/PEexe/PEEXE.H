//==========================================
// Modified from Matt Pietrek by F.GANNIER
// Microsoft Systems Journal, Feb 1997
// FILE: PEEXE.H
//==========================================
#ifndef __PEEXE_H__
#define __PEEXE_H__

#include "exefile.h"
#include <delayimp.h>
#include "decplug.h"
#include "dependencylist.h"

class PE_EXE : public EXE_FILE
{
	BOOL m_btested;
	DecPlugin			*m_pDec;
	PIMAGE_NT_HEADERS32	m_pNtHdr;
	PIMAGE_NT_HEADERS64	m_pNtHdr64;
	MODULE_DEPENDENCY_LIST *m_pdep;
	DWORD EntryPointRAW, EntryPointRVA;
public:
	PE_EXE( PSTR pszFileName );
	~PE_EXE( );
	BOOL IsValid() { return m_pNtHdr ? TRUE : FALSE; }
	BOOL IsCoded();
	void Save(CString st);
	DecPlugin *GetDecPtr() { return m_pDec; }
	PIMAGE_NT_HEADERS32 GetIMAGE_NT_HEADERS32( void ) { return m_pNtHdr; }
	PIMAGE_NT_HEADERS64 GetIMAGE_NT_HEADERS64( void ) { return m_pNtHdr64; }
	MODULE_DEPENDENCY_LIST *GetDepends();
	BOOL Attach(PVOID base, DWORD size);
	void Detach();
	bool IsGoodExport();
//	bool Is64bits(void) { return (GetMachine()!=0x014c) && (GetMachine()!=0x01a2) && (GetMachine()!=0x01a4) && (GetMachine()!=0x01a6);};
	bool Is64bits(void) { return (GetMachine()!=0x014c) && (GetMagic( )!=0x10b);};
	// IMAGE_FILE_HEADER fields
// if Is64bits() return m_pNtHdr64->FileHeader.Machine; else return m_pNtHdr32->FileHeader.Machine; }
	WORD    GetMachine( void ) { return m_pNtHdr->FileHeader.Machine; }
	WORD    GetMagic( void ) { return m_pNtHdr->OptionalHeader.Magic; }
	WORD    GetNumberOfSections( void ) { return m_pNtHdr->FileHeader.NumberOfSections; }
	DWORD   GetTimeDateStamp(void) { return m_pNtHdr->FileHeader.TimeDateStamp; }
	DWORD   GetCharacteristics( void ) { return m_pNtHdr->FileHeader.Characteristics; }
	// IMAGE_OPTIONAL_HEADER fields
	DWORD   GetSizeOfCode( void ) { return m_pNtHdr->OptionalHeader.SizeOfCode; }
	DWORD   GetSizeOfInitializedData( void ) { return m_pNtHdr->OptionalHeader.SizeOfInitializedData; }
	DWORD   GetSizeOfUninitializedData( void ) { return m_pNtHdr->OptionalHeader.SizeOfUninitializedData; }
	DWORD   GetAddressOfEntryPoint( void )    { return m_pNtHdr->OptionalHeader.AddressOfEntryPoint; }
	DWORD   GetBaseOfCode( void ){ return m_pNtHdr->OptionalHeader.BaseOfCode; }
	DWORD   GetBaseOfData( void )
			{
	if (Is64bits()) return 0; // PE32+ has no BaseOfData field
	return m_pNtHdr->OptionalHeader.BaseOfData;
	}
/**/
#ifdef _WIN64
	typedef ULONGLONG P_IB ;
#else
	typedef DWORD P_IB ;
#endif
	P_IB	GetImageBase( void )
			{ return m_pNtHdr->OptionalHeader.ImageBase; }
/*
	DWORD   GetSectionAlignment( void )
			{ return m_pNtHdr->OptionalHeader.SectionAlignment; }
	DWORD   GetFileAlignment( void )
			{ return m_pNtHdr->OptionalHeader.FileAlignment; }
	WORD    GetMajorOperatingSystemVersion( void )
			{ return m_pNtHdr->OptionalHeader.MajorOperatingSystemVersion; }
	WORD    GetMinorOperatingSystemVersion( void )
			{ return m_pNtHdr->OptionalHeader.MinorOperatingSystemVersion; }
	WORD    GetMajorImageVersion( void )
			{ return m_pNtHdr->OptionalHeader.MajorImageVersion; }
	WORD    GetMinorImageVersion( void )
			{ return m_pNtHdr->OptionalHeader.MinorImageVersion; }
	WORD    GetMajorSubsystemVersion( void )
			{ return m_pNtHdr->OptionalHeader.MajorSubsystemVersion; }
	WORD    GetMinorSubsystemVersion( void )
			{ return m_pNtHdr->OptionalHeader.MinorSubsystemVersion; }
//    DWORD   GetWin32VersionValue( void )
//          { return m_pNtHdr->OptionalHeader.Win32VersionValue; }
	DWORD   GetSizeOfImage( void )
			{ return m_pNtHdr->OptionalHeader.SizeOfImage; }
	DWORD   GetSizeOfHeaders( void )
			{ return m_pNtHdr->OptionalHeader.SizeOfHeaders; }
	WORD    GetSubsystem( void )
			{ return m_pNtHdr->OptionalHeader.Subsystem; }
	DWORD   GetSizeOfStackReserve( void )
			{ return m_pNtHdr->OptionalHeader.SizeOfStackReserve; }
	DWORD   GetSizeOfStackCommit( void )
			{ return m_pNtHdr->OptionalHeader.SizeOfStackCommit; }
	DWORD   GetSizeOfHeapReserve( void )
			{ return m_pNtHdr->OptionalHeader.SizeOfHeapReserve; }
	DWORD   GetSizeOfHeapCommit( void )
			{ return m_pNtHdr->OptionalHeader.SizeOfHeapCommit; }
*/
	WORD    GetSubsystem( void ) { return m_pNtHdr->OptionalHeader.Subsystem; }
	DWORD   GetDataDirectoryEntryRVA( DWORD id );
	PVOID   GetDataDirectoryEntryPointer( DWORD id );
	DWORD   GetDataDirectoryEntrySize( DWORD id );

	PVOID   GetReadablePointerFromRVA( DWORD rva );
	PVOID 	GetReadablePointerFromVA( PVOID va );

/************ ADD FG ********************/
	DWORD GetOrEntryPointRVA( void ) {return EntryPointRVA; }
	DWORD GetOrEntryPoint( void ) {return EntryPointRAW; }
	DWORD GetEntryPoint( void );
	PIMAGE_SECTION_HEADER GetSectionHeaderbyName(PSTR name);
	PIMAGE_SECTION_HEADER GetEnclosingSectionHeader( DWORD rva );
	PIMAGE_IMPORT_DESCRIPTOR GetImportsDesc() // OK test�
	{ 
		return (PIMAGE_IMPORT_DESCRIPTOR) GetDataDirectoryEntryPointer(IMAGE_DIRECTORY_ENTRY_IMPORT);
	}
	DWORD GetImportsSz() // OK test�
	{ 
		return GetDataDirectoryEntrySize(IMAGE_DIRECTORY_ENTRY_IMPORT);
	}

	PIMAGE_EXPORT_DIRECTORY GetExportsDesc() // OK test�
	{ 
		return (PIMAGE_EXPORT_DIRECTORY) GetDataDirectoryEntryPointer(IMAGE_DIRECTORY_ENTRY_EXPORT);
	}

	PImgDelayDescr GetDelayImportsDesc()
	{
		return ( PImgDelayDescr ) GetDataDirectoryEntryPointer(IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
	}

	DWORD   RVAToFileOffset( DWORD rva );
};

#endif
