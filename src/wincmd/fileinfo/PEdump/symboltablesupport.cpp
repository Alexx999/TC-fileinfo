//==================================
// PEDUMP - Matt Pietrek 1997
// FILE: COMMON.C
//==================================
#include "stdafx.h"

#include "common.h"
#include "COFFSymbolTable.h"
#include "extrnvar.h"
#include "cv_dbg.h"

#include "..\unmangle.h"
//
// Dump the COFF debug information header
//
CStringA DumpCOFFHeader(PIMAGE_COFF_SYMBOLS_HEADER pDbgInfo)
{
    CStringA str="", strTemp="";
    str += ("COFF Debug Info Header\r\n");
    strTemp.Format("\tNumberOfSymbols: \t%08Xh\r\n", pDbgInfo->NumberOfSymbols);
	str += strTemp;
    strTemp.Format("\tLvaToFirstSymbol: \t%08Xh\r\n", pDbgInfo->LvaToFirstSymbol);
	str += strTemp;
    strTemp.Format("\tNumberOfLinenumbers: \t%08Xh\r\n", pDbgInfo->NumberOfLinenumbers);
	str += strTemp;
    strTemp.Format("\tLvaToFirstLinenumber: \t%08Xh\r\n", pDbgInfo->LvaToFirstLinenumber);
	str += strTemp;
    strTemp.Format("\tRvaToFirstByteOfCode: \t%08Xh\r\n", pDbgInfo->RvaToFirstByteOfCode);
	str += strTemp;
    strTemp.Format("\tRvaToLastByteOfCode: \t%08Xh\r\n", pDbgInfo->RvaToLastByteOfCode);
	str += strTemp;
    strTemp.Format("\tRvaToFirstByteOfData: \t%08Xh\r\n", pDbgInfo->RvaToFirstByteOfData);
	str += strTemp;
    strTemp.Format("\tRvaToLastByteOfData: \t%08Xh\r\n", pDbgInfo->RvaToLastByteOfData);
	str += strTemp;

	str += ("\r\n");
	return str;
}


//
// Given a COFF symbol table index, look up its name.  This function assumes
// that the COFFSymbolCount and PCOFFSymbolTable variables have been already
// set up.
//
BOOL LookupSymbolName(DWORD index, PSTR buffer, UINT length)
{
	if ( !g_pCOFFSymbolTable ) return FALSE;
	PCOFFSymbol pSymbol = g_pCOFFSymbolTable->GetSymbolFromIndex( index );
	if ( !pSymbol ) return FALSE;
	lstrcpynA( buffer, pSymbol->GetName(), length );

	delete pSymbol;
    return TRUE;
}

//
// Dump a range of line numbers from the COFF debug information
//
CStringA DumpLineNumbers(PIMAGE_LINENUMBER pln, DWORD count)
{
    CStringA str="", strTemp="";
    char buffer[64];
    DWORD i;
    
    str += ("LINE NUMBERS\r\n");
    for (i=0; i < count; i++)
    {
        if ( pln->Linenumber == 0 ) // A symbol table index
        {
            buffer[0] = 0;
            LookupSymbolName(pln->Type.SymbolTableIndex, buffer, sizeof(buffer));
            strTemp.Format("SymIndex: %X (%s)\r\n", pln->Type.SymbolTableIndex, buffer);
			str += strTemp;
        }
        else        // A regular line number
		{
            strTemp.Format(" Addr: %05Xh  Line: %04u\r\n", pln->Type.VirtualAddress, pln->Linenumber);
			str += strTemp;
		}
        pln++;
    }
	str += ("\r\n");
	return str;
}


//
// Used by the DumpSymbolTable() routine.  It purpose is to filter out
// the non-normal section numbers and give them meaningful names.
//
void GetSectionName(WORD section, PSTR buffer, unsigned cbBuffer)
{
    char tempbuffer[10];
    
    switch ( (SHORT)section )
    {
        case IMAGE_SYM_UNDEFINED: strcpy(tempbuffer, "UNDEF"); break;
        case IMAGE_SYM_ABSOLUTE:  strcpy(tempbuffer, "ABS"); break;
        case IMAGE_SYM_DEBUG:     strcpy(tempbuffer, "DEBUG"); break;
        default: sprintf(tempbuffer, "%X", section);
    }
    
    strncpy(buffer, tempbuffer, cbBuffer-1);
}

//
// Dumps a COFF symbol table from an EXE or OBJ
//
CStringA DumpSymbolTable( PCOFFSymbolTable pSymTab )
{
    CStringA str="", strTemp="";

    strTemp.Format( "SYMBOL TABLE - %X entries  (* = auxillary symbol)\r\n", pSymTab->GetNumberOfSymbols() );
	str += strTemp;

    str += (    "\tIndex   Section\tValue \tType \t\tStorage \t\tName\r\n" 
				"\t--------------------\t------------- \t---------- \t-------------- \t\t-------------\r\n");

	PCOFFSymbol pSymbol = pSymTab->GetNextSymbol( 0 );
	while ( pSymbol )
	{
	    char szSection[10];
        GetSectionName(pSymbol->GetSectionNumber(),szSection,sizeof(szSection));

		char Textout[1024];
		if ( fShowUndecorated && strncmp(szSection, "ABS", 3))
			Undecorate( pSymbol->GetName(), Textout, 1024);
		else 
			strncpy( Textout, pSymbol->GetName(), 1024);

        strTemp.Format( "\t%04Xh    %5s \t%08Xh  \t%s \t%s \t%s\r\n",
        		pSymbol->GetIndex(), szSection, pSymbol->GetValue(),
        		pSymbol->GetTypeName(), pSymbol->GetStorageClassName(),
        		Textout); //pSymbol->GetName() 
		str += strTemp;
		
		if ( pSymbol->GetNumberOfAuxSymbols() )
		{
			char szAuxSymbol[1024];
			if (pSymbol->GetAuxSymbolAsString(szAuxSymbol,sizeof(szAuxSymbol)))
			{
				strTemp.Format( "     * %s\r\n", szAuxSymbol );			
				str += strTemp;
			}
		}
		
		pSymbol = pSymTab->GetNextSymbol( pSymbol );

	}

	str += ("\r\n");
	return str;
}

CStringA DumpMiscDebugInfo( PIMAGE_DEBUG_MISC pMiscDebugInfo )
{
    CStringA str="", strTemp="";
	if ( IMAGE_DEBUG_MISC_EXENAME != pMiscDebugInfo->DataType )
	{
		strTemp.Format( "Unknown Miscellaneous Debug Information type: %u\r\n",  pMiscDebugInfo->DataType );
		str += strTemp;
		return str;
	}

	str +=  "Miscellaneous Debug Information :\r\n";
	strTemp.Format( "  %s\r\n", pMiscDebugInfo->Data );
	str += strTemp;
	str += ("\r\n");
	return str;	
}

CStringA DumpCVDebugInfo( PDWORD pCVHeader )
{
    CStringA str="", strTemp="";

	strTemp.Format( "CODEVIEW Signature:\t%08Xh (\"%.4s\")\r\n", *pCVHeader, pCVHeader );
	str += strTemp;
	switch (*pCVHeader)
	{
		case '01BN': 
			{
				PPDB_INFO pPDBInfo;

				pPDBInfo = (PPDB_INFO)pCVHeader;
				strTemp.Format( "\tOffset: %08Xh     Signature: %08Xh     Age: %d\r\n", pPDBInfo->Offset, pPDBInfo->sig, pPDBInfo->age );
				str += strTemp;
				strTemp.Format( "\tFile: %s\r\n", pPDBInfo->PdbName );
				break;
			}
		case 'SDSR' :
			{
				PRSDS_INFO pPDBInfo;

				pPDBInfo = (PRSDS_INFO)pCVHeader;

				strTemp.Format( "\tGUID: \"%08X-%04X-%04X-%08X%08X\"     Age: %d\r\n", pPDBInfo->guid1, pPDBInfo->guid2 >> 16, (pPDBInfo->guid2 << 16) >> 16, pPDBInfo->guid3, pPDBInfo->guid4, pPDBInfo->age );
				str += strTemp;
				strTemp.Format( "\tFile: %s\r\n", pPDBInfo->PdbName );				 
				break;
			}
		default :
				strTemp.Format( "Unhandled CodeView Information format \"%.4s\"", pCVHeader );
	}
	str += strTemp;
	str += ("\r\n");
	return str;
}

