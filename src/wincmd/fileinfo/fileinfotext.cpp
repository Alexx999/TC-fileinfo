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
#include <vector>
#include "pedump\exedump.h"
#include "pedump\objdump.h"
#include "pedump\libdump.h"
#include "pedump\elfdump.h"
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
   } else {
		CStringA elfInfo = GetElfVersionInfo(FileToLoad);
		if (!elfInfo.IsEmpty())
			str += CString(elfInfo);
		else
			str += "No file version information available\r\n\r\n	";
	}
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

//------------------------------------------------------------------
// DllTreeContext: heap-allocated context that survives tree construction
// so the OnItemExpanding handler can lazily populate child nodes.
//------------------------------------------------------------------
// Sentinel value stored via SetItemData on dummy child items that indicate
// "children not yet loaded — populate on expand".
#define DUMMY_CHILD_MARKER ((DWORD_PTR)1)

struct DllTreeContext
{
	CDllPathCache   pathCache;
	CModuleCache    modCache;
	CDllHandleCache handleCache;
	int             maxDepth;
};

// Populates one level of the dependency tree under ParentItem.
// Called both during initial tree construction (depth 0) and lazily on expand.
static void PopulateTreeLevel( CTreeCtrl &tree, HTREEITEM ParentItem,
	MODULE_DEPENDENCY_LIST *pdep, int depth, DllTreeContext &ctx)
{
	HTREEITEM ChidItem;

	// Single pass: collect all modules (regular + delayed) while computing
	// the maximum base-name length for display alignment.
	struct ModEntry {
		PMODULE_FILE_INFO pMod;
		bool bDelayed;
	};
	std::vector<ModEntry> modules;
	int maxNameLen = 0;

	PMODULE_FILE_INFO pModInfo = pdep->GetNextModule( (PMODULE_FILE_INFO) 0 );
	if (pModInfo)
		while ( pModInfo = pdep->GetNextModule( pModInfo ) )
		{
			int len = (int)_tcslen(pModInfo->GetBaseName());
			if (len > maxNameLen) maxNameLen = len;
			modules.push_back({ pModInfo, false });
		}
	pModInfo = pdep->GetNextDelayedModule( (PMODULE_FILE_INFO) 0 );
	if (pModInfo)
		while ( pModInfo = pdep->GetNextDelayedModule( pModInfo ) )
		{
			int len = (int)_tcslen(pModInfo->GetBaseName());
			if (len > maxNameLen) maxNameLen = len;
			modules.push_back({ pModInfo, true });
		}

	int padTo = maxNameLen + 2;  // 2 spaces after the longest name

	// Insert into tree with aligned display names
	for (const auto& entry : modules)
	{
		PMODULE_FILE_INFO pMod = entry.pMod;
		// Icon indices: regular  0=found, 1=notfound, 4=missing-func
		//               delayed  2=found, 3=notfound, 5=missing-func
		int icoFound    = entry.bDelayed ? 2 : 0;
		int icoNotFound = entry.bDelayed ? 3 : 1;
		int icoMissing  = entry.bDelayed ? 5 : 4;

		if (pMod->IsModuleFound())
		{
			// Run TestFunction to determine icon (found vs missing-func)
			int ico = pMod->TestFunction(&ctx.handleCache) ? icoFound : icoMissing;
			ChidItem = tree.InsertItem( pMod->GetDisplayName(padTo), ico, ico, ParentItem );
			// Store MODULE_FILE_INFO* for lazy child population via GetFullName()
			tree.SetItemData( ChidItem, (DWORD_PTR) pMod );

			// Insert a dummy child to show the expand arrow.
			// Real children will be populated lazily when the user expands.
			if ( depth + 1 < ctx.maxDepth )
			{
				HTREEITEM hDummy = tree.InsertItem( _T(""), 0, 0, ChidItem );
				tree.SetItemData( hDummy, DUMMY_CHILD_MARKER );
			}
		}
		else
			ChidItem = tree.InsertItem( pMod->GetDisplayName(padTo), icoNotFound, icoNotFound, ParentItem );
	}
}

void CreateParentTree( PVOID ptr, CTreeCtrl &tree, CWait &wait)
{
	PE_EXE *pPE = (PE_EXE *) ptr;

	ASSERT(tree);
    if (!tree.m_hWnd) return;
    wait.SetStatus("Analysing Modules...");

    HTREEITEM ParentItem = tree.InsertItem( pPE->GetName()); //>GetPath() + pPE->GetBaseName());
	pPE->IsCoded();		//Test compressed and decompress

	// Create heap-allocated context that survives tree construction,
	// allowing the OnItemExpanding handler to populate children lazily.
	DllTreeContext *pCtx = new DllTreeContext;
	pCtx->maxDepth = MaxDepth;

	pPE->SetPathCache(&pCtx->pathCache);
    MODULE_DEPENDENCY_LIST *pDep = pPE->GetDepends();
    if (pDep->IsValid())
		PopulateTreeLevel(tree, ParentItem, pDep, 0, *pCtx);

	// Store context pointer on the root tree item so CPageTree can
	// retrieve it for lazy child population and clean it up later.
	tree.SetItemData( ParentItem, (DWORD_PTR) pCtx );
}

void DeleteDllTreeContext(DllTreeContext* p) { delete p; }

BOOL DllTreeContext_ExpandNode(DllTreeContext* pCtx, CTreeCtrl& tree, HTREEITEM hParent)
{
	// Check if the first child is the dummy marker (lazy placeholder)
	HTREEITEM hFirstChild = tree.GetChildItem(hParent);
	if (!hFirstChild) return FALSE;
	if (tree.GetItemData(hFirstChild) != DUMMY_CHILD_MARKER)
		return FALSE;  // Already populated

	// Delete the dummy child
	tree.DeleteItem(hFirstChild);

	// Get the MODULE_FILE_INFO* stored on the parent item
	DWORD_PTR dwData = tree.GetItemData(hParent);
	if (!dwData || dwData == DUMMY_CHILD_MARKER) return FALSE;
	MODULE_FILE_INFO* pParentMod = (MODULE_FILE_INFO*) dwData;

	// Resolve dependencies for this module (cached if already seen)
	MODULE_DEPENDENCY_LIST* pdep = pCtx->modCache.GetDepends(
		pParentMod->GetFullName(), &pCtx->pathCache);
	if (!pdep || !pdep->IsValid()) return TRUE;

	// Count depth from root to determine MaxDepth constraint
	int depth = 0;
	HTREEITEM hWalk = hParent;
	while ((hWalk = tree.GetParentItem(hWalk)) != NULL)
		depth++;

	// Populate one level of children (with TestFunction + dummy children)
	PopulateTreeLevel(tree, hParent, pdep, depth, *pCtx);
	return TRUE;
}


/************* LIB, OBJ ******************/

OBJ_FILE_TYPE DisplayObjectFile( MEMORY_MAPPED_FILE *pmmf )
{
    PBYTE pFileBase = (PBYTE)pmmf->GetBase();
    if ( !pFileBase )
        return OBJ_UNKNOWN;
    if ( pmmf->GetFileSize() >= 4 &&
         0 == memcmp(pFileBase, "\x7F" "ELF", 4) )
        return OBJ_ELF;
    if ( 0 == strncmp((PSTR)pFileBase, IMAGE_ARCHIVE_START, IMAGE_ARCHIVE_START_SIZE) )
    {
        // Verify it's a COFF library, not just any ar archive (e.g. .deb packages).
        // COFF libraries always start with a linker member named "/".
        if ( pmmf->GetFileSize() > IMAGE_ARCHIVE_START_SIZE + IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR )
        {
            PIMAGE_ARCHIVE_MEMBER_HEADER pFirstMember =
                (PIMAGE_ARCHIVE_MEMBER_HEADER)(pFileBase + IMAGE_ARCHIVE_START_SIZE);
            if ( pFirstMember->Name[0] == '/' )
                return OBJ_COFF_LIB;
        }
        return OBJ_UNKNOWN;
    }
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

    if( FALSE == libFile->IsValid() )
    {
		str.Format( _T("Unable to access file <%s>."), libFile->GetName());
		str += sFormat;
        return str;
    }

    OBJ_FILE_TYPE ft = DisplayObjectFile( libFile );
    LPCTSTR label = (ft == OBJ_ELF) ? _T("ELF") : _T("FILE");
    str.Format( _T("%s: %s\n\n"), label, libFile->GetName());

    switch ( ft )
    {
	case OBJ_COFF_OBJ:
		{
			PIMAGE_FILE_HEADER pImgFileHdr = (PIMAGE_FILE_HEADER)libFile->GetBase();
			PIMAGE_OPTIONAL_HEADER32 pImgOptHdr = (PIMAGE_OPTIONAL_HEADER32)(pImgFileHdr + 1);
			str += DumpObjFile(pImgFileHdr, pImgOptHdr, libFile->GetBase()+libFile->GetFileSize());
			break;
		}
        case OBJ_COFF_LIB: //        DumpCOFFObjFile( (LPVOID) libFile->GetBase() );
//			str += DumpLibFile( (LPVOID) libFile->GetBase() ); break;
			str += DumpLibFile( ptr ); break;
//        case OBJ_OMF_OBJ: pszFileType = "OMF OBJ"; break; // DumpIntelOMFFile( (LPVOID) libFile->GetBase());
//        case OBJ_OMF_LIB: pszFileType = "OMF LIB"; break;
//        case OBJ_OMF_IMPLIB: pszFileType = "OMF IMPORT LIB"; break;
        case OBJ_ELF:
            str += DumpElfFile( (const BYTE *)libFile->GetBase(), libFile->GetFileSize() );
            break;
        default: str += "Not a recognized object file.";
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