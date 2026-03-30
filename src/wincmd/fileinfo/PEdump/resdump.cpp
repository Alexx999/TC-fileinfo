//==================================
// PEDUMP - Matt Pietrek 1997
// FILE: RESDUMP.C
//==================================
#include "stdafx.h"
#include "..\peexe\peexe.h"

#pragma hdrstop
#include "common.h"
#include "extrnvar.h"
#include "resdump.h"



// Function prototype (necessary because two functions recurse)
CString DumpResourceDirectory
(
    PIMAGE_RESOURCE_DIRECTORY resDir, ULONG_PTR resourceBase,
    DWORD level, DWORD resourceType
);

// The predefined resource types
char *SzResourceTypes[] = {
"???_0",
"CURSOR",
"BITMAP",
"ICON",
"MENU",
"DIALOG",
"STRING",
"FONTDIR",
"FONT",
"ACCELERATORS",
"RCDATA",
"MESSAGETABLE",
"GROUP_CURSOR",
"???_13",
"GROUP_ICON",
"???_15",
"VERSION",
"DLGINCLUDE",
"???_18",
"PLUGPLAY",
"VXD",
"ANICURSOR",
"ANIICON",
"HTML",
"MANIFEST"
};

DWORD cStrResEntries = 0;
DWORD cDlgResEntries = 0;
DWORD cMFTResEntries = 0;

PIMAGE_RESOURCE_DATA_ENTRY GetDataEntryFromResEntry( ULONG_PTR base, ULONG_PTR resourceBase, PIMAGE_RESOURCE_DIRECTORY_ENTRY pResEntry )
{
	// The IMAGE_RESOURCE_DIRECTORY_ENTRY is gonna point to a single
	// IMAGE_RESOURCE_DIRECTORY, which in turn will point to the
	// IMAGE_RESOURCE_DIRECTORY_ENTRY, which in turn will point
	// to the IMAGE_RESOURCE_DATA_ENTRY that we're really after.  In
	// other words, traverse down a level.

	PIMAGE_RESOURCE_DIRECTORY pStupidResDir;
	pStupidResDir = (PIMAGE_RESOURCE_DIRECTORY) (resourceBase + pResEntry->OffsetToDirectory);

    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pStupidResDir + 1);// PTR MATH
	return (PIMAGE_RESOURCE_DATA_ENTRY)(resourceBase +
										 pResDirEntry->OffsetToData);
}
DWORD GetOffsetToDataFromResEntry( ULONG_PTR base, ULONG_PTR resourceBase, PIMAGE_RESOURCE_DIRECTORY_ENTRY pResEntry )
{
	PIMAGE_RESOURCE_DATA_ENTRY pResDataEntry = GetDataEntryFromResEntry( base, resourceBase, pResEntry );
	return pResDataEntry->OffsetToData;
}

CString DumpStringTable( PE_EXE &pe, ULONG_PTR resourceBase, PIMAGE_RESOURCE_DIRECTORY_ENTRY pStrResEntry, DWORD cStrResEntries )
{
   CString str="", strTemp="";
	for ( unsigned i = 0; i < cStrResEntries; i++, pStrResEntry++ )
	{
		DWORD offsetToData = GetOffsetToDataFromResEntry( pe.GetBase(), resourceBase, pStrResEntry );
		PWORD pStrEntry = (PWORD)GetPtrFromRVA(	offsetToData, pe.GetIMAGE_NT_HEADERS32(), pe.GetBase());
		if ( !pStrEntry) break;
		unsigned id = (pStrResEntry->Name - 1) << 4;

		for ( unsigned j = 0; j < 16; j++ )
		{
			if (((ULONG_PTR) pStrEntry - pe.GetBase() ) > pe.GetFileSize())
			{
#ifdef _DEBUG 
				AfxMessageBox("Error in Strings Resource", MB_OK|MB_ICONEXCLAMATION);
#endif
				str = "Possible compressed String resource\r\n";
				return str;
			}
			WORD len = *pStrEntry++;
			if ( len )
			{
				strTemp.Format( "     %-5u: ", id + j );
				str += strTemp;

				for ( unsigned k = 0; k < min(len, (WORD)64); k++ )
				{
					char * s;
					char szBuff[20];
					char c = (char)pStrEntry[k];
					switch( c )
					{
						case '�' : s = "�"; break;
						case '\t': s = "\\t"; break;
						case '\r': s = "\\r"; break;
						case '\n': s = "\\n"; break;
						case '�': s = "�"; break;
						default:
							wsprintf( szBuff, "%c", isprint(c) ? c : '.' );
							s=szBuff;
							break;
					}

					str += s;
				}

				str += ( "\r\n" );
			}

			pStrEntry += len;
		}
	}
	return str;
}

CString DumpDialogs(PE_EXE &pe, ULONG_PTR resourceBase, PIMAGE_RESOURCE_DIRECTORY_ENTRY pDlgResEntry, DWORD cDlgResEntries )
{
   CString str="", strTemp="";
	for ( unsigned i = 0; i < cDlgResEntries; i++, pDlgResEntry++ )
	{
		DWORD offsetToData = GetOffsetToDataFromResEntry( pe.GetBase(), resourceBase, pDlgResEntry );
		PDWORD pDlgStyle = (PDWORD)GetPtrFromRVA(	offsetToData, pe.GetIMAGE_NT_HEADERS32(), pe.GetBase());
		if ( !pDlgStyle )break;
		if (((ULONG_PTR) pDlgStyle - pe.GetBase()) > pe.GetFileSize())
		{
#ifdef _DEBUG 
			AfxMessageBox("Error in Dialogs Resource", MB_OK|MB_ICONEXCLAMATION);
#endif
			str = "Possible compressed Dialog resource\r\n";
			return str;
		}
													
		str += ( "  ====================\r\n" );
		if ( HIWORD(*pDlgStyle) != 0xFFFF )
		{
			//	A regular DLGTEMPLATE
			DLGTEMPLATE * pDlgTemplate = ( DLGTEMPLATE * )pDlgStyle;

			if ( pDlgTemplate->cx <= 0 || pDlgTemplate->cy <= 0 
					|| pDlgTemplate->x < 0 || pDlgTemplate->y < 0 
					|| pDlgTemplate->cdit > 255 )
			{
	#ifdef _DEBUG 
				AfxMessageBox("Error in Dialogs Resource", MB_OK|MB_ICONEXCLAMATION);
#endif
				str += "Possible compressed Dialog resource\r\n";
				return str;
			}
			strTemp.Format( "\tstyle: %08Xh\r\n", pDlgTemplate->style );			
			str += strTemp;
			strTemp.Format( "\textended style: %08Xh\r\n", pDlgTemplate->dwExtendedStyle );			
			str += strTemp;
			strTemp.Format( "\tcontrols: %u\r\n", pDlgTemplate->cdit );
			str += strTemp;
			strTemp.Format( "\t(%u,%u) - (%u,%u)\r\n",
						pDlgTemplate->x, pDlgTemplate->y,
						pDlgTemplate->x + pDlgTemplate->cx,
						pDlgTemplate->y + pDlgTemplate->cy );
			str += strTemp;
			PWORD pMenu = (PWORD)(pDlgTemplate + 1);	// ptr math!

			//
			// First comes the menu
			//
			if ( *pMenu )
			{
				if ( 0xFFFF == *pMenu )
				{
					pMenu++;
					strTemp.Format( "\tordinal menu: %u\r\n", *pMenu );
					str += strTemp;
				}
				else
				{
					str += ( "\tmenu: " );
					str += strTemp;
					while ( *pMenu )
					{
						strTemp.Format( "%c", LOBYTE(*pMenu++) );				
						str += strTemp;
					}

					pMenu++;
					str += ( "\r\n" );
				}
			}
			else
				pMenu++;	// Advance past the menu name

			//
			// Next comes the class
			//			
			PWORD pClass = pMenu;
						
			if ( *pClass )
			{
				if ( 0xFFFF == *pClass )
				{
					pClass++;
					strTemp.Format( "\tordinal class: %u\r\n", *pClass );
					str += strTemp;
				}
				else
				{
					str += ( "\tclass: " );
					while ( *pClass )
					{
						strTemp.Format( "%c", LOBYTE(*pClass++) );				
						str += strTemp;
					}		
					pClass++;
					str += ( "\r\n" );
				}
			}
			else
				pClass++;	// Advance past the class name
			
			//
			// Finally comes the title
			//

			PWORD pTitle = pClass;
			if ( *pTitle )
			{
				str += ( "\ttitle: " );

				while ( *pTitle )
				{
					strTemp.Format( "%c", LOBYTE(*pTitle++) );
					str += strTemp;
				}	
				pTitle++;
			}
			else
				pTitle++;	// Advance past the Title name

			str += ( "\r\n" );

			PWORD pFont = pTitle;
						
			if ( pDlgTemplate->style & DS_SETFONT )
			{
				strTemp.Format( "\tFont: %u point ",  *pFont++ );
				str += strTemp;
				while ( *pFont )
				{
					strTemp.Format( "%c", LOBYTE(*pFont++) );
					str += strTemp;
				}

				pFont++;
				str += ( "\r\n" );
			}
	        else
    	        pFont = pTitle; 

			// DLGITEMPLATE starts on a 4 byte boundary
			LPDLGITEMTEMPLATE pDlgItemTemplate = (LPDLGITEMTEMPLATE)pFont;
			
			for ( unsigned i=0; i < pDlgTemplate->cdit; i++ )
			{
				// Control item header....
				pDlgItemTemplate = (DLGITEMTEMPLATE *)
									(((ULONG_PTR)pDlgItemTemplate+3) & ~(ULONG_PTR)3);
				
				strTemp.Format( "\t\tstyle: %08Xh\r\n", pDlgItemTemplate->style );			
				str += strTemp;
				strTemp.Format( "\t\textended style: %08Xh\r\n", pDlgItemTemplate->dwExtendedStyle );			
				str += strTemp;

				strTemp.Format( "\t\t(%u,%u) - (%u,%u)\r\n",
							pDlgItemTemplate->x, pDlgItemTemplate->y,
							pDlgItemTemplate->x + pDlgItemTemplate->cx,
							pDlgItemTemplate->y + pDlgItemTemplate->cy );
				str += strTemp;
				strTemp.Format( "\t\tid: %u\r\n", pDlgItemTemplate->id );
				str += strTemp;
				
				//
				// Next comes the control's class name or ID
				//			
				PWORD pClass = (PWORD)(pDlgItemTemplate + 1);
				if ( *pClass )
				{							
					if ( 0xFFFF == *pClass )
					{
						pClass++;
						strTemp.Format( "\t\tordinal class: %u", *pClass++ );
						str += strTemp;
					}
					else
					{
						str += ( "\t\tclass: " );
						while ( *pClass )
						{
							strTemp.Format( "%c", LOBYTE(*pClass++) );
							str += strTemp;
						}
						pClass++;
						str += ( "\r\n" );
					}
				}
				else
					pClass++;
					
				str += ( "\r\n" );			

				//
				// next comes the title
				//

				PWORD pTitle = pClass;
				
				if ( *pTitle )
				{
					str += ( "\t\ttitle: " );
					if ( 0xFFFF == *pTitle )
					{
						pTitle++;
						strTemp.Format( "%u\r\n", *pTitle++ );
						str += strTemp;
					}
					else
					{
						while ( *pTitle )
							strTemp.Format( "%c", LOBYTE(*pTitle++) );
							str += strTemp;
						pTitle++;
						str += ( "\r\n" );
					}
				}
				else	
					pTitle++;	// Advance past the Title name

				str += ( "\r\n" );
				
				PBYTE pCreationData = (PBYTE)(((ULONG_PTR)pTitle + 1) & ~(ULONG_PTR)1);
				
				if ( *pCreationData )
					pCreationData += *pCreationData;
				else
					pCreationData++;

				pDlgItemTemplate = (DLGITEMTEMPLATE *)pCreationData;	
				
				str += ( "\r\n" );
			}
			
			str += ( "\r\n" );
		}
		else
		{
			// A DLGTEMPLATEEX		
		}
		
		str += ( "\r\n" );
	}
	return str;
}

CString DumpManifest(PE_EXE &pe, ULONG_PTR resourceBase, PIMAGE_RESOURCE_DIRECTORY_ENTRY pResEntry, DWORD cResEntries )
{
	CString str="", strTemp="";
	for ( unsigned i = 0; i < cResEntries; i++, pResEntry++ )
	{
		PIMAGE_RESOURCE_DATA_ENTRY pMFTDataEntry = GetDataEntryFromResEntry( pe.GetBase (), resourceBase, pResEntry );
		ULONG size = pMFTDataEntry->Size - 8;

		DWORD offsetToData = GetOffsetToDataFromResEntry( pe.GetBase(), resourceBase, pResEntry );
		PCHAR pStart = (PCHAR) GetPtrFromRVA( offsetToData, pe.GetIMAGE_NT_HEADERS32(), pe.GetBase());
		PCHAR pChar = pStart;
		if ( !pChar) {
#ifdef _DEBUG 
			AfxMessageBox("Error in Manifest Resource", MB_OK|MB_ICONEXCLAMATION);
#endif
			str = "Possible compressed resource\r\n";
			return str;
		}
		ULONG level = 0;
		str += ("        ");
		while(pChar - pStart < (size+8))
		{	
			if (*pChar==0x0D) {
				pChar++;
				str += "\r\n";
			} else if (*pChar==0x09) {
				pChar++;
				str += ("   ");
			} else if (*pChar==0x0A) {
				pChar++;
				for ( i=0; i <= level; i++ ) str += ("    ");
			} else
				str += *pChar++;
			}
/*
			while (*pChar!='<') {  pChar++; }
			if ((*pChar == '<') && (*(pChar+1) != '/')) level++;
			for ( i=0; i <= level; i++ ) str += ("   ");
			if ((*pChar == '<') && (*(pChar+1) == '/')) level--;

			while (*pChar!='>')
			{
				if (*pChar==0x0D) {
					pChar++;
					str += "\r\n";
				} else if (*pChar==0x09) {
					pChar++;
					str += (" ");
				} else if (*pChar==0x0A) {
					pChar++;
					for ( i=0; i <= level; i++ ) str += ("   ");
				} else
					str += *pChar++;
			}
			if ((*(pChar-1) == '/')) level--;
			str += *pChar++;
			str += "\r\n"; /**/
	}
	return str;
}
// Get an ASCII string representing a resource type
void GetResourceTypeName(DWORD type, PSTR buffer, UINT cBytes)
{
    if ( type <= (WORD)RT_MANIFEST) // RT_ANIICON )  // RT_MANIFEST
        strncpy(buffer, SzResourceTypes[type], cBytes);
    else
		if ( type == 240) 
			strncpy(buffer, "DLGINIT", cBytes);
    else
		if ( type == 241) 
			strncpy(buffer, "TOOLBAB", cBytes);
	else
        sprintf(buffer, "%X", type);
}

//
// If a resource entry has a string name (rather than an ID), go find
// the string and convert it from unicode to ascii.
//
void GetResourceNameFromId( DWORD id, ULONG_PTR resourceBase, PSTR buffer, UINT cBytes)
{
    PIMAGE_RESOURCE_DIR_STRING_U prdsu;

    // If it's a regular ID, just format it.
    if ( !(id & IMAGE_RESOURCE_NAME_IS_STRING) )
    {
        sprintf(buffer, "%X", id);
        return;
    }
    
    id &= 0x7FFFFFFF;
    prdsu = (PIMAGE_RESOURCE_DIR_STRING_U)(resourceBase + id);

    // prdsu->Length is the number of unicode characters
    WideCharToMultiByte(CP_ACP, 0, prdsu->NameString, prdsu->Length,
                        buffer, cBytes, 0, 0);
    buffer[ min(cBytes-1,prdsu->Length) ] = 0;  // Null terminate it!!!
}

//
// Dump the information about one resource directory entry.  If the
// entry is for a subdirectory, call the directory dumping routine
// instead of printing information in this routine.
//
CString DumpResourceEntry ( PIMAGE_RESOURCE_DIRECTORY_ENTRY resDirEntry, ULONG_PTR resourceBase, DWORD level )
{
    CString str="", strTemp="";
    UINT i;
    char nameBuffer[128];
    PIMAGE_RESOURCE_DATA_ENTRY pResDataEntry;
    
    if ( resDirEntry->OffsetToData & IMAGE_RESOURCE_DATA_IS_DIRECTORY )
    {
        str += DumpResourceDirectory( (PIMAGE_RESOURCE_DIRECTORY)
            ((resDirEntry->OffsetToData & 0x7FFFFFFF) + resourceBase),
			resourceBase, level, resDirEntry->Name);
        return str;
    }

    // Spit out the spacing for the level indentation
    for ( i=0; i <= level; i++ ) str += ("    ");

    if ( resDirEntry->Name & IMAGE_RESOURCE_NAME_IS_STRING )
    {
        GetResourceNameFromId(resDirEntry->Name, resourceBase, nameBuffer, sizeof(nameBuffer));
        strTemp.Format("Name: %s    DataEntryOffs: %08Xh\r\n", nameBuffer, resDirEntry->OffsetToData);
		str += strTemp;
    }
    else
    {
        strTemp.Format("ID: %08X    DataEntryOffs: %08Xh\r\n", resDirEntry->Name, resDirEntry->OffsetToData);
		str += strTemp;
    }
    
    // the resDirEntry->OffsetToData is a pointer to an IMAGE_RESOURCE_DATA_ENTRY.  
	// Go dump out that information.  First, spit out the proper indentation
    for ( i=0; i <= level; i++ )
		str += ("    ");
    
    pResDataEntry = (PIMAGE_RESOURCE_DATA_ENTRY) (resourceBase + resDirEntry->OffsetToData);
    strTemp.Format("DataRVA: %05Xh    DataSize: %05Xh    CodePage: %X\r\n",
            pResDataEntry->OffsetToData, pResDataEntry->Size,
            pResDataEntry->CodePage);
	str += strTemp;
	return str;
}

PIMAGE_RESOURCE_DIRECTORY_ENTRY pStrResEntries = 0;
PIMAGE_RESOURCE_DIRECTORY_ENTRY pDlgResEntries = 0;
PIMAGE_RESOURCE_DIRECTORY_ENTRY pMFTResEntries = 0;
//
// Dump the information about one resource directory.
//
CString DumpResourceDirectory ( PIMAGE_RESOURCE_DIRECTORY resDir, ULONG_PTR resourceBase, DWORD level, DWORD resourceType)
{
    CString str="", strTemp="";
    PIMAGE_RESOURCE_DIRECTORY_ENTRY resDirEntry;
    char szType[64];
    UINT i;

    // Level 1 resources are the resource types
    if ( level == 1 )
    {
		str = ( "  --------------------------------------------------------------\r\n" );
		if ( resourceType & IMAGE_RESOURCE_NAME_IS_STRING )
			GetResourceNameFromId( resourceType, resourceBase, szType, sizeof(szType) );
		else
	        GetResourceTypeName( resourceType, szType, sizeof(szType) );
	}
    else    // All other levels, just print out the regular id or name
        GetResourceNameFromId( resourceType, resourceBase, szType, sizeof(szType) );
	    
    // Spit out the spacing for the level indentation
    for ( i=0; i <= level; i++ )  str += ("     ");

    strTemp.Format( "ResDir (%s)    Entries:%02Xh  (Named:%02Xh, ID:%02Xh)    TimeDate:%08Xh",
        szType, resDir->NumberOfNamedEntries+ resDir->NumberOfIdEntries,
        resDir->NumberOfNamedEntries, resDir->NumberOfIdEntries,
        resDir->TimeDateStamp );
     str += strTemp;
	if ( resDir->MajorVersion || resDir->MinorVersion )
	{
		strTemp.Format( "  Vers:%u.%02u", resDir->MajorVersion, resDir->MinorVersion );
		str += strTemp;
	}
	if ( resDir->Characteristics)
	{
		strTemp.Format( "  Char:%08Xh", resDir->Characteristics );
		str += strTemp;
	}
	str += ( "\r\n" );

	//
	// The "directory entries" immediately follow the directory in memory
	//
    resDirEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resDir+1);

	// If it's a stringtable, save off info for future use
	if ( level == 1 && (resourceType == (WORD)RT_STRING))
	{
		pStrResEntries = resDirEntry;
		cStrResEntries = resDir->NumberOfIdEntries;
	}

	if ( level == 1 && (resourceType == (WORD)RT_MANIFEST))
	{
		pMFTResEntries = resDirEntry;
		cMFTResEntries = resDir->NumberOfIdEntries;
	}

	// If it's a dialog, save off info for future use
	if ( level == 1 && (resourceType == (WORD)RT_DIALOG))
	{
		pDlgResEntries = resDirEntry;
		cDlgResEntries = resDir->NumberOfIdEntries;
	}
	    
    for ( i=0; i < resDir->NumberOfNamedEntries; i++, resDirEntry++ )
        str += DumpResourceEntry(resDirEntry, resourceBase, level+1);

    for ( i=0; i < resDir->NumberOfIdEntries; i++, resDirEntry++ )
        str += DumpResourceEntry(resDirEntry, resourceBase, level+1);
	return str;
}


//
// Top level routine called to dump out the entire resource hierarchy
//
//CString DumpResourceSection(DWORD base, PIMAGE_NT_HEADERS pNTHeader)
CString DumpResourceSection(PE_EXE &pe)
{
    CString str="", strTemp="";
	cStrResEntries = 0;
	cDlgResEntries = 0;
	cMFTResEntries = 0;
	pStrResEntries = 0;
	pDlgResEntries = 0;
	pMFTResEntries = 0;

	DWORD resourcesRVA;
    PIMAGE_RESOURCE_DIRECTORY resDir;

	resourcesRVA = pe.GetDataDirectoryEntryRVA(IMAGE_DIRECTORY_ENTRY_RESOURCE);
	if ( !resourcesRVA ) return str;
	
//	resDir = (PIMAGE_RESOURCE_DIRECTORY) pe.GetReadablePointerFromRVA( resourcesRVA);
    resDir = (PIMAGE_RESOURCE_DIRECTORY) GetPtrFromRVA( resourcesRVA, pe.GetIMAGE_NT_HEADERS32(), pe.GetBase () );

	if ( !resDir ) return str;
	if ( !pe.IsValidPtr(( ULONG_PTR ) resDir))
	{
#ifdef _DEBUG 
		AfxMessageBox("Error in Resources Section", MB_OK|MB_ICONEXCLAMATION);
#endif
		str += "RESOURCES not found, due to a possible compressed executable\r\n\r\n";
		return str;
	}
	strTemp.Format("RESOURCES  ( RVA: %X )\r\n", resourcesRVA );
	str+=strTemp;
	str += DumpResourceDirectory(resDir, (ULONG_PTR)resDir, 0, 0);
	str += ( "\r\n" );
	if ( cStrResEntries )
	{
		str+=( "String Table (first 67 chars)\r\n" );
//		DWORD offsetToData = GetOffsetToDataFromResEntry( pe.GetBase (), (DWORD)resDir, pStrResEntries );
		str += DumpStringTable( pe, (ULONG_PTR)resDir, pStrResEntries, cStrResEntries );
		str+=( "\r\n" );
	} 

	if ( cDlgResEntries )	
	{
		str+=( "Dialogs\r\n" );
		str+=DumpDialogs( pe, (ULONG_PTR)resDir, pDlgResEntries, cDlgResEntries );
		str+=( "\r\n" );
	}/**/

	if ( cMFTResEntries )
	{
		str+=( "     MANIFEST\r\n" );
		str += "====================\r\n";
		str += DumpManifest( pe, (ULONG_PTR)resDir, pMFTResEntries, cMFTResEntries );
		str += "\r\n====================\r\n\r\n";
	} 

	return str;
}

bool IsMFTres(PE_EXE &pe)
{
	if (!pe.IsValid()) return false;
	DWORD resourcesRVA = pe.GetDataDirectoryEntryRVA(IMAGE_DIRECTORY_ENTRY_RESOURCE);
	if ( !resourcesRVA ) return 0;
	PIMAGE_RESOURCE_DIRECTORY resDir = (PIMAGE_RESOURCE_DIRECTORY) GetPtrFromRVA( resourcesRVA, pe.GetIMAGE_NT_HEADERS32(), pe.GetBase () );
	PIMAGE_RESOURCE_DIRECTORY_ENTRY resDirEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resDir+1);
	if (!resDir) return false;
	for ( DWORD i=0; i < resDir->NumberOfIdEntries; i++, resDirEntry++ )
	{
		if ((resDirEntry->Id == (WORD)RT_MANIFEST))
		{
			DumpResourceEntry(resDirEntry, (ULONG_PTR) resDir, 1); // initialize values
			return true;
		}
	}
	return false;
}

PIMAGE_RESOURCE_DIRECTORY GetResDir(PE_EXE &pe)
{
	DWORD resourcesRVA;resourcesRVA = pe.GetDataDirectoryEntryRVA(IMAGE_DIRECTORY_ENTRY_RESOURCE);
	if ( !resourcesRVA ) return 0;
	else return (PIMAGE_RESOURCE_DIRECTORY) GetPtrFromRVA( resourcesRVA, pe.GetIMAGE_NT_HEADERS32(), pe.GetBase () );
}