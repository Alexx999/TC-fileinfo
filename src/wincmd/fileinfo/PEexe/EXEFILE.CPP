//==========================================
// Modified from Matt Pietrek by F.GANNIER
// Microsoft Systems Journal, Feb 1997
// FILE: EXEFILE.CPP
//==========================================
#include "stdafx.h"

#pragma hdrstop
#include "..\exetype.h"
#include "exefile.h"


///=====================================================
EXE_FILE::EXE_FILE( LPCTSTR pszFileName ) : MEMORY_MAPPED_FILE( pszFileName )
{
    m_errorType = errEXE_FILE_FILE_NOT_FOUND;
    m_secondaryHeaderOffset = -1;   // A bogus value to catch bugs
    m_exeType = exeType_Invalid;

	// m_errorType already set to errEXE_FILE_FILE_NOT_FOUND
    if ( FALSE == MEMORY_MAPPED_FILE::IsValid() ) return;     

    // If we get here, the file exists, and was mapped.  We're still not sure that it's a valid EXE though
    m_errorType = errEXE_FILE_INVALID_FORMAT;
    if ( GetFileSize() < sizeof(IMAGE_DOS_HEADER) ) return;
    
    PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER)GetBase();
    if ( IMAGE_DOS_SIGNATURE != pDosHdr->e_magic ) 
		if ( IMAGE_DOS_INVSIGNATURE != pDosHdr->e_magic ) 
			return;

    // If we get here, it's at least a DOS 'MZ' file
    m_errorType = errEXE_FILE_NO_ERROR;

    // Sanity check.  Make sure the "new header" offset isn't past the end of the file
    if ( pDosHdr->e_lfanew + sizeof(IMAGE_NT_HEADERS32) > (ULONG)GetFileSize() )
	{	
		if ( pDosHdr->e_lfarlc < 0x40 ) // Theoretically, this field must be >= 0x40 for it to be a non-DOS executable
		    m_exeType = exeType_DOS;
		else m_exeType = exeType_DOSEXT;
        return;
	}
    // Make a pointer to the secondary header   
    m_secondaryHeaderOffset = pDosHdr->e_lfanew;
    PWORD pSecondHdr = MakePtr( PWORD, GetBase(), m_secondaryHeaderOffset );

// Decide what type of EXE, based on the start of the secondary header
    switch ( *pSecondHdr )
    {
		case IMAGE_OS2_SIGNATURE: m_exeType = exeType_NE; break;
		case IMAGE_LE_SIGNATURE : m_exeType = exeType_LE; break;
		case IMAGE_LX_SIGNATURE : m_exeType = exeType_LX; break; // voir rar32.exe DOS 32b
//		case IMAGE_BW_SIGNATURE : m_exeType = exeType_BW; break; // dos4gw.exe
		case IMAGE_W4_SIGNATURE : m_exeType = exeType_W4; break; 
		case IMAGE_W3_SIGNATURE : m_exeType = exeType_W3; break; 
		case IMAGE_DL_SIGNATURE : m_exeType = exeType_DL; break; 
		case IMAGE_MP_SIGNATURE : m_exeType = exeType_MP; break; 
		case IMAGE_P2_SIGNATURE : m_exeType = exeType_P2; break; 
		case IMAGE_P3_SIGNATURE : m_exeType = exeType_P3; break; 
//		case IMAGE_PW_SIGNATURE : m_exeType = exeType_PW; break; 
		case 0x4558				: m_exeType = exeType_LX; break;     // OS/2 2.X
		case IMAGE_NT_SIGNATURE : m_exeType = exeType_PE; break;
		default : m_exeType = exeType_DOS; m_secondaryHeaderOffset = -1; pSecondHdr = 0;
    }
}

BOOL EXE_FILE::Attach(PVOID base, DWORD size)
{
	if ( !MEMORY_MAPPED_FILE::Attach(base, size)) return FALSE;
    PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER)GetBase();
    if ( IMAGE_DOS_SIGNATURE != pDosHdr->e_magic ) 
	{
		MEMORY_MAPPED_FILE::Detach();
		return FALSE;
	}
	m_secondaryHeaderOffset = pDosHdr->e_lfanew;
	return TRUE;
}

void EXE_FILE::Detach()
{
	MEMORY_MAPPED_FILE::Detach();
    PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER) GetBase();
	m_secondaryHeaderOffset = pDosHdr->e_lfanew;
}

PSTR EXE_FILE::GetFileTypeDescription( void )
{// Returns a static string that describes what type this file is
    switch ( m_exeType )
	{
		case exeType_DOSEXT: return "Special DOS Executable "; // "MZ"
		case exeType_DOS:   return "DOS Executable (MZ)"; // "MZ"
		case exeType_NE:    return "New Executable (NE) (Windows 3.1)"; //"NE";
		case exeType_LE:    return "Linear Executable (LE) (VXD - DOS Extender)"; // "LE"; 
		case exeType_LX:    return "Linear Executable (LX) (OS/2 32 bit)"; // "LX"; 
		case exeType_PE:    return "Portable Executable (PE)";
		case exeType_BW:    return "Linear Executable (BW) (DOS Extender - DOS4GW)"; // 
		case exeType_W3:	return "Windows WIN386.EXE (W3) (Collection of LE File)";
		case exeType_W4:	return "Windows 95/98 (W4) (Collection of LE File)";
		case exeType_DL:	return "HP 100LX/200LX system manager Executable (.EXM)";
		case exeType_MP:	return "Old PharLap Executable (.EXP)"; 
		case exeType_P2:	return "PharLap 286 Executable (.EXP)";
		case exeType_P3:	return "PharLap 386 Executable (.EXP) ";
		case exeType_PW:	return "Packed with PMWLITE";
		default:            return "Invalid or Unknown Executable "; //(please, send it to the author)
    }
}
