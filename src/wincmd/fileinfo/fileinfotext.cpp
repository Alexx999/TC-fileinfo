    /***********************************************************/
    /*****         Sous-Routines :                       *******/
    /***************  Auteur :    Fr GANNIER   *****************/
    /***************     Lab. Physio. Anim.    *****************/
    /***************      Fac. des Sciences    *****************/
    /***************        37000 Tours        *****************/
    /***********************************************************/
#include "stdafx.h"
#include "fileinfotext.h"

#include "..\..\common\ffile.h"
#include "..\..\common\verwin.h"

#include "peexe\dependencylist.h"
#include "peexe\apisetresolve.h"
#include <map>
#include "pedump\exedump.h"
#include "pedump\objdump.h"
#include "pedump\libdump.h"
#include "dosdump.h"
	
/*
#include "objdump.h"
#include "dbgdump.h"
#include "libdump.h"
#include "romimage.h"
#include "extrnvar.h"
*/

//#define   rtfPrefix  "{\\rtf1\\ansi\\deff0\\deftab720{\\fonttbl{\\f0\\fswiss MS Sans Serif;}}"
#define   rtfPrefix  "{\\rtf1\\ansi"
#define   rtfReturn  "\n\\par"
#define   rtfPostfix "\n\\par}"


/************* Image File Header ******************/
CString CreateText1(PVOID ptr, CWait &wait)
{
   CString str ="";  //= rtfPrefix;

   PE_EXE *pPE = (PE_EXE *) ptr;
   EXE_FILE *pEXE = (EXE_FILE *) pPE; // mappedfile((LPSTR) filename);
   if (pEXE->IsValid())
   {
		str += "TECHNICAL FILE INFORMATION : \r\n";
		str += "File Type Description :\t";
		str += pEXE->GetFileTypeDescription();
		str += "\r\n";


		ULONG_PTR base = (ULONG_PTR) pEXE->GetdosHeader();
		PIMAGE_DOS_HEADER pdosHeader = pEXE->GetdosHeader();
		switch( pEXE->GetExeType())
		{
			case exeType_BW:
			case exeType_W3:
			case exeType_W4:
			case exeType_DL:
			case exeType_MP:
			case exeType_P2:
			case exeType_P3:
			case exeType_PW:			
			case exeType_DOSEXT :
			case exeType_DOS: str += DumpMZHeader( pdosHeader );
				break;
			case exeType_NE:  
				str += DumpNEHeader( pdosHeader );  
				str += "\r\nDOS HEADER\r\n";
				str += DumpMZHeader( pdosHeader );
				break;
			case exeType_LE: 
				str += DumpLEHeader( pEXE );   
				str += "\r\nDOS HEADER\r\n";
				str += DumpMZHeader( pdosHeader );
				break;
			case exeType_LX: 
				str += DumpLXHeader( pEXE );
				str += "\r\nDOS HEADER\r\n";
				str += DumpMZHeader( pdosHeader );
				break;
			case exeType_PE:  
				pPE->IsCoded();				
				str += DumpExeFile( *pPE, wait );

				str += "\r\nDOS HEADER\r\n";
				str += DumpMZHeader( pdosHeader ); // A modifier
				break;

/*      else if ( (pImgFileHdr->Machine == IMAGE_FILE_MACHINE_I386)   // FIX THIS!!!
             ||(pImgFileHdr->Machine == IMAGE_FILE_MACHINE_ALPHA) )
      {            // 0 optional header // means it's an OBJ
         if ( 0 == pImgFileHdr->SizeOfOptionalHeader )      
            DumpObjFile( pImgFileHdr );               
         else if (    pImgFileHdr->SizeOfOptionalHeader == IMAGE_SIZEOF_ROM_OPTIONAL_HEADER )
            DumpROMImage( (PIMAGE_ROM_HEADERS)pImgFileHdr );
      }
      else if ( 0 == strncmp((char *)lpFileBase,    IMAGE_ARCHIVE_START, IMAGE_ARCHIVE_START_SIZE ) )
         DumpLibFile( lpFileBase );
*/
		}
   }  
   else {
      switch(pEXE->GetErrorType())
      {
         case 1: return ("File not Found \r\n"); break;
         case 2: return ("Invalid File Format \r\n");break;
         case 3: return ("Truncated or Unrecognized File \r\n");break;
      }
   }
   return str; // + rtfPostfix;
}


/************* File Properties ******************/

//CString CreateText0(PVOID ptr)
CString CreateText0(LPCTSTR FileToLoad )
{
//	char *FileToLoad = (char *) ptr;
	CString Temp, str;
	str = CString(FileToLoad) + "\r\n";
	str += "on ";
	str += GetSystemVersionName( GetSystemVersion() ) + "\r\n";

	CFileVersionInfo m_info(FileToLoad);
	if (m_info.IsValid())
	{
		str += "File Version Information : \r\n";
//		if (m_info.GetTargetOs()==VOS_DOS_WINDOWS16 && m_info.GetFileType() != VFT_VXD)
		if ((m_info.GetAll()).IsEmpty())
		{
			CString strRes, strItem;
			strRes = Name_STR;
			for (int n = SFI_FIRST; n <= SFI_LAST; ++n)
			{
				if (m_info.IsVersionInfoAvailable(n))
				{
					 AfxExtractSubString(strItem, strRes, n, _T('\n'));
					 str += strItem + "\t:  " + m_info.GetVersionInfo(n) + "\r\n";
				}   
			}
		} else /**/
			str += m_info.GetAll();
   } else str += "No file version information available\r\n\r\n	";
   str +="\r\n";

	FILETIME   ft;
	SYSTEMTIME st;
	HANDLE hFile = CreateFile(FileToLoad, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	BY_HANDLE_FILE_INFORMATION FileInformation;
	if (GetFileInformationByHandle( hFile, &FileInformation ))
	{

		if (FileInformation.ftCreationTime.dwLowDateTime)
		{
			::FileTimeToLocalFileTime(&FileInformation.ftCreationTime, &ft);
			::FileTimeToSystemTime(&ft, &st);
			str += "Creation Date\t: " + FormatShortDate(st);
			str += "\r\n";
		}
		if (FileInformation.ftLastWriteTime.dwLowDateTime)
		{
			::FileTimeToLocalFileTime(&FileInformation.ftLastWriteTime, &ft);
			::FileTimeToSystemTime(&ft, &st);
			str += "Last Modif. Date\t: " + FormatShortDate(st);
			str += "\r\n";
		}
		::FileTimeToLocalFileTime(&FileInformation.ftLastAccessTime, &ft);
		::FileTimeToSystemTime(&ft, &st);
		str += "Last Access Date\t: " + FormatShortDate(st);
		str += "\r\n";
	} else {


	}
		
	str += "FileSize\t:"; DWORD size = GetFileSize(hFile, NULL);
	Temp.Format(_T(" %d bytes ( %.3f KB,  %.3f MB ) \r\n"), size, (float) size/1024.0, (float) size/(1024.0*1024.0));
	str +=Temp;

	CloseHandle( hFile );

	if (!m_info.IsValid()) return str;

	str += _T("FileVersionInfoSize\t:");
	Temp.Format(_T(" %d bytes  \r\n"), m_info.GetFileVersionSize());
	str +=Temp;

	str += _T("File type\t: ") + m_info.GetFileType(IDS_FT);
	Temp.Format(_T(" (0x%X) \r\n"), m_info.GetFileType());
	str +=Temp;


	str += _T("Target OS\t: ") + m_info.GetTargetOs(IDS_OS);
	Temp.Format(_T(" (0x%X) \r\n"), m_info.GetTargetOs());
	str += Temp;

	str += _T("File/Product version\t: ") + m_info.GetFileVersionString() + _T(" / ") + m_info.GetProductVersionString();
	str += _T("\r\n");

	str += _T("Language \t: ") + m_info.GetLanguageName();
	Temp.Format(_T(" (0x%X) \r\n"), m_info.GetLanguageId());
	str +=Temp;

	str += _T("Character Set\t: ") + m_info.GetCharSetName();
	Temp.Format(_T(" (0x%X) \r\n"),m_info.GetCharSet());
	str += Temp;

	str += _T("\r\nBuild Information : \r\n");
	Temp.Format(_T("Debug Version\t: %s \r\n"), (m_info.IsDebugVersion()?_T("yes"):_T("no")));
	str += Temp;
	Temp.Format(_T("Patched Version\t: %s \r\n"), (m_info.IsPatched()?_T("yes"):_T("no")));
	str += Temp;
	Temp.Format(_T("Prerelease Version\t: %s \r\n"), (m_info.IsPreRelease()?_T("yes"):_T("no")));
	str += Temp;
	Temp.Format(_T("Private Version\t: %s \r\n"), (m_info.IsPrivateBuild()?_T("yes"):_T("no")));
	str += Temp;
	Temp.Format(_T("Special Build\t: %s \r\n "), (m_info.IsSpecialBuild()?_T("yes"):_T("no")));
	str += Temp;

	return str;
}   

/*************  disassembler ******************/
#include "disass\cadt.h"
#include "delay.h" // for delay-load lib

FARPROC WINAPI hookDLLLoad(unsigned dliNotify,PDelayLoadInfo  pdli)
{
	char buf[256];
	sprintf(buf,"DLL or Function in API not found.\nDLL name = %s, Message notify = %d, last error = %d",
		pdli->szDll,		dliNotify,		pdli->dwLastError	);
	throw dllload_error(buf);
	return (FARPROC)NULL;
}

ExternC const PfnDliHook __pfnDliFailureHook2 = hookDLLLoad;

CString CreateText3(PVOID ptr, CWait &wait)
{
	CString Temp, str;
	PE_EXE *pPE = (PE_EXE *) ptr;
	pPE->IsCoded();
	PVOID cPtr = (PVOID) pPE->GetTranslatedPtr(pPE->GetEntryPoint());
#ifdef _WIN64

	
#else 
	TCHAR szPath[MAX_PATH];
	TCHAR szOriginalPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, szOriginalPath);  // Save original dir

	LPTSTR pszDontCare;
	if (!SearchPath(0, _T("cadt.dll"), 0, MAX_PATH, szPath, &pszDontCare))
	{
 		if (GetModuleFileName(GetModuleHandle(ModuleName), szPath, MAX_PATH))
		{
			TCHAR *end = _tcsrchr( szPath, _T('\\'));
			*end=_T('\0');
			SetCurrentDirectory( szPath );				 // Switch to app's dir
			_tcsncat( szPath, _T("\\cadt.dll"), 10);
		}
	}
	if (!LoadLibraryEx(szPath, NULL, DONT_RESOLVE_DLL_REFERENCES))
	{
		str = _T("cadt.dll not found");
		return str;
	}

	wait.SetStatus(_T("Disassembling..."));
	try
	{
		char dBuff[1024];

		ULONG Len;
		TDisCommand Command;
		TInstruction Instr;
		TMnemonicOptios Options;

		Options.RealtiveOffsets = TRUE;
		Options.AddAddresPart   = TRUE;
		Options.AlternativeAddres = pPE->GetEntryPoint();
		Options.AddHexDump = TRUE;
		Options.MnemonicAlign = 35;

		Temp.Format( _T("Code Analization and Disassembling Tool (%hs)\n\n"), GetCadtVersion());
		str += Temp;
		str += _T("Entry Point : \t");
		if (pPE->GetOrEntryPoint())
			Temp.Format(_T("%08Xh\r\n"), pPE->GetOrEntryPoint());
		else Temp.Format(_T("%s\r\n"), _T("Invalid or not in CODE section (possible Encrypted or Packed Executable)"));

		str += Temp;
		if (pPE->IsCoded() && pPE->IsAttached())
		{
			Temp.Format( _T("Compressed Executable, Using Decompressed Image\n"));
			str += Temp;
			Temp.Format( _T("Corrected Entry point : %08Xh \n\n"), pPE->GetEntryPoint());
			str += Temp;
		}
		
		if (!pPE->GetEntryPoint())	return str;
		do
      	{
			memset(&Instr, 0, sizeof(TInstruction));
			memset(&Command, 0, sizeof(TDisCommand));
			Len = InstrDecode(cPtr, &Instr, FALSE);

			InstrDasm(&Instr, &Command, FALSE);
			MakeMnemonic(dBuff, &Command, &Options);
			Temp.Format( _T("%hs \n"), dBuff);
			str += Temp;

			if (pPE->IsValidPtr((ULONG_PTR) cPtr + Len))
				cPtr = (PVOID)((ULONG_PTR)cPtr + Len);
			else 
			{
				str += "<----------------   End of file   ---------------->";
				break;
			}
			Options.AlternativeAddres += Len;

      	} while (Command.CmdOrdinal != 0x4C);
	} catch(dllload_error er) {
		SetCurrentDirectory( szOriginalPath );				 // Switch to app's dir

		return (CString) er.what();
	}
	SetCurrentDirectory( szOriginalPath );				 // Switch to app's dir
#endif
	return str;
}   

/************* Dll Dependencies ******************/

extern int MaxDepth;

//------------------------------------------------------------------
// CModuleCache: caches PE_EXE objects so each DLL is parsed only once
// during a single dependency-tree analysis.
//------------------------------------------------------------------
class CModuleCache
{
	std::map<CString, PE_EXE*, CStringNoCaseLess> m_cache;
public:
	~CModuleCache()
	{
		for (auto& pair : m_cache)
			delete pair.second;
	}

	// Returns the dependency list for the given DLL path.
	// Creates and caches the PE_EXE if not already seen.
	MODULE_DEPENDENCY_LIST* GetDepends(LPCTSTR pszFullPath, CDllPathCache* pPathCache)
	{
		CString key(pszFullPath);
		auto it = m_cache.find(key);
		if (it != m_cache.end())
			return it->second->GetDepends();

		PE_EXE* pPE = new PE_EXE(pszFullPath);
		pPE->SetPathCache(pPathCache);
		m_cache[key] = pPE;
		return pPE->GetDepends();
	}
};

static void CreateChildTreeCached( CTreeCtrl &tree, HTREEITEM &ParentItem,
	MODULE_DEPENDENCY_LIST *pdep, int depth,
	CModuleCache &modCache, CDllPathCache &pathCache, CDllHandleCache &handleCache)
{
	HTREEITEM ChidItem;

	// First pass: find longest base name among all siblings for alignment
	int maxNameLen = 0;
	PMODULE_FILE_INFO pModInfo = pdep->GetNextModule( (PMODULE_FILE_INFO) 0 );
	if (pModInfo)
		while ( pModInfo = pdep->GetNextModule( pModInfo ) )
		{
			int len = (int)_tcslen(pModInfo->GetBaseName());
			if (len > maxNameLen) maxNameLen = len;
		}
	pModInfo = pdep->GetNextDelayedModule( (PMODULE_FILE_INFO) 0 );
	if (pModInfo)
		while ( pModInfo = pdep->GetNextDelayedModule( pModInfo ) )
		{
			int len = (int)_tcslen(pModInfo->GetBaseName());
			if (len > maxNameLen) maxNameLen = len;
		}
	int padTo = maxNameLen + 2;  // 2 spaces after the longest name

	// Second pass: insert into tree with aligned display names
	pModInfo = pdep->GetNextModule( (PMODULE_FILE_INFO) 0 );
	if (pModInfo )
		while ( pModInfo = pdep->GetNextModule( pModInfo ) )
		{
			if (pModInfo->IsModuleFound())
			{
				MODULE_DEPENDENCY_LIST *pDepChild = modCache.GetDepends(pModInfo->GetFullName(), &pathCache);

				if (pModInfo->TestFunction(&handleCache))
					ChidItem = tree.InsertItem( pModInfo->GetDisplayName(padTo), 0, 0, ParentItem );
				else ChidItem = tree.InsertItem( pModInfo->GetDisplayName(padTo), 4, 4, ParentItem );/**/
				if ( pDepChild->IsValid() )
				{
					if ( depth + 1 < MaxDepth )
						CreateChildTreeCached(tree, ChidItem, pDepChild, depth + 1,
							modCache, pathCache, handleCache);
				}
			}
			else ChidItem = tree.InsertItem( pModInfo->GetDisplayName(padTo), 1, 1, ParentItem );/**/
		}

	pModInfo = pdep->GetNextDelayedModule( (PMODULE_FILE_INFO) 0 );
	if (pModInfo )
		while ( pModInfo = pdep->GetNextDelayedModule( pModInfo ) )
		{
			if (pModInfo->IsModuleFound())
			{
				MODULE_DEPENDENCY_LIST *pDepChild = modCache.GetDepends(pModInfo->GetFullName(), &pathCache);
				if (pModInfo->TestFunction(&handleCache))
					ChidItem = tree.InsertItem( pModInfo->GetDisplayName(padTo), 2, 2, ParentItem );
				else ChidItem = tree.InsertItem( pModInfo->GetDisplayName(padTo), 5, 5, ParentItem );
				if ( pDepChild->IsValid() )
				{
					if (depth + 1 < MaxDepth)
						CreateChildTreeCached(tree, ChidItem, pDepChild, depth + 1,
							modCache, pathCache, handleCache);
				}
			}
			else
				ChidItem = tree.InsertItem( pModInfo->GetDisplayName(padTo), 3, 3, ParentItem );
		}
}

void CreateParentTree( PVOID ptr, CTreeCtrl &tree, CWait &wait)
{
	PE_EXE *pPE = (PE_EXE *) ptr;

	ASSERT(tree);
    if (!tree.m_hWnd) return;
    wait.SetStatus("Analysing Modules...");

	// Suppress redraws during tree population for performance
	tree.SetRedraw(FALSE);

    HTREEITEM ParentItem = tree.InsertItem( pPE->GetName()); //>GetPath() + pPE->GetBaseName());
	pPE->IsCoded();		//Test compressed and decompress

	// Create per-analysis caches (auto-destroyed when scope exits)
	CDllPathCache pathCache;
	CModuleCache modCache;
	CDllHandleCache handleCache;

	pPE->SetPathCache(&pathCache);
    MODULE_DEPENDENCY_LIST *pDep = pPE->GetDepends();
    if (pDep->IsValid())
		CreateChildTreeCached(tree, ParentItem, pDep, 0, modCache, pathCache, handleCache);

	// Re-enable redraws and force a single repaint
	tree.SetRedraw(TRUE);
	tree.Invalidate();
}


/************* LIB, OBJ ******************/

OBJ_FILE_TYPE DisplayObjectFile( MEMORY_MAPPED_FILE *pmmf )
{
    PBYTE pFileBase = (PBYTE)pmmf->GetBase();
    if ( !pFileBase )
        return OBJ_UNKNOWN;
    if ( 0 == strncmp((PSTR)pFileBase, IMAGE_ARCHIVE_START, IMAGE_ARCHIVE_START_SIZE) )
        return OBJ_COFF_LIB;
    if ( IMAGE_FILE_MACHINE_I386 == *(PWORD)pFileBase )
        return OBJ_COFF_OBJ;
    if ( (0xF0 == *(PBYTE)pFileBase) || (0x80 == *(PBYTE)pFileBase) )
        return OBJ_OMF_LIB;  // DumpIntelOMFFile( pFileBase );
	if ( 0 == strncmp((PSTR)pFileBase, "MSFT", 4))
		return OBJ_TL;
    return OBJ_UNKNOWN;
}

/*
    switch ( fileType )
    {
        case OBJ_COFF_OBJ: pszFileType = "COFF OBJ"; break;
        case OBJ_COFF_LIB: pszFileType = "COFF LIB"; break;
        case OBJ_OMF_OBJ: pszFileType = "OMF OBJ"; break;
        case OBJ_OMF_LIB: pszFileType = "OMF LIB"; break;
        case OBJ_OMF_IMPLIB: pszFileType = "OMF IMPORT LIB"; break;
        default: pszFileType = "UNKNOWN FILE TYPE"; break;
    }
*/

CString CreateText2(PVOID ptr, CWait &wait)
{	
	CString str, sBuff, sFormat;
	MEMORY_MAPPED_FILE *libFile = ( MEMORY_MAPPED_FILE *) ptr;

	str.Format( _T("LIBRARY: %s\n\n"), libFile->GetName());	

    if( FALSE == libFile->IsValid() )
    {
		str.Format( _T("Unable to access file <%s>."), libFile->GetName());
		str += sFormat;
        return str;
    }

    switch ( DisplayObjectFile( libFile ) )
    {
	case OBJ_COFF_OBJ:
		{
			PIMAGE_FILE_HEADER pImgFileHdr = (PIMAGE_FILE_HEADER)libFile->GetBase();
			PIMAGE_OPTIONAL_HEADER32 pImgOptHdr = (PIMAGE_OPTIONAL_HEADER32)(pImgFileHdr + 1);
			str += DumpObjFile(pImgFileHdr, pImgOptHdr);
			break;
		}
        case OBJ_COFF_LIB: //        DumpCOFFObjFile( (LPVOID) libFile->GetBase() );
//			str += DumpLibFile( (LPVOID) libFile->GetBase() ); break;
			str += DumpLibFile( ptr ); break;
//        case OBJ_OMF_OBJ: pszFileType = "OMF OBJ"; break; // DumpIntelOMFFile( (LPVOID) libFile->GetBase());
//        case OBJ_OMF_LIB: pszFileType = "OMF LIB"; break;
//        case OBJ_OMF_IMPLIB: pszFileType = "OMF IMPORT LIB"; break;
        default: str += "Not a valid COFF LIB file.";
    }
	return str;
}


extern DWORD cMFTResEntries;
extern PIMAGE_RESOURCE_DIRECTORY_ENTRY pMFTResEntries;
CString DumpManifest(PE_EXE &pe, ULONG_PTR resourceBase, PIMAGE_RESOURCE_DIRECTORY_ENTRY pResEntry, DWORD cResEntries );
PIMAGE_RESOURCE_DIRECTORY GetResDir(PE_EXE &pe);
CString CreateManifest(PVOID ptr, CWait &wait)
{
	CString str, sBuff, sFormat;
    wait.SetStatus("Extracting Manifest...");
	PE_EXE *pPE = (PE_EXE *) ptr;
	if( FALSE == pPE->IsValid() )
    {
		str.Format( _T("Unable to access file <%s>."), pPE->GetName());
		str += sFormat;
        return str;
    }
	return DumpManifest(*pPE, (ULONG_PTR) GetResDir(*pPE), pMFTResEntries, cMFTResEntries );

}