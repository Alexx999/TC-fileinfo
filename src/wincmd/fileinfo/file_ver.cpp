//		File:		file_ver.cpp
//					------------
//		Content:	Implementation of class CFileVersionInfo.
//					Manages the retrieval of various information from a
//					Win32 file image.
//
//		Created:		01/27/2002
//						Thomas Weller
//
//		Last modified:	02/02/2002
//						Thomas Weller
// ----------------------------------------------------------------------------
//	Remarks : The "version.lib" library module must be added to project settings.
// ----------------------------------------------------------------------------

#include "stdafx.h"
#include "file_ver.h"

/*
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
*/

// Format string for browsing the variable resource info block
// ***********************************************************
const LPCTSTR CFileVersionInfo::m_lpszFmtStringFileInfo = _T("\\StringFileInfo\\%04x%04x\\%s");

// Specific pieces of version information (corresponding to SFI_xxx constants)
// ***************************************************************************
const LPCTSTR CFileVersionInfo::m_lpszStringFileInfos[] =
{ 
	_T("CompanyName"),			// SFI_COMPANYNAME
	_T("FileDescription"),		// SFI_FILEDESCRIPTION
	_T("FileVersion"),			// SFI_FILEVERSION
	_T("InternalName"),			// SFI_INTERNALNAME
	_T("LegalCopyRight"),		// SFI_LEGALCOPYRIGHT
	_T("OriginalFileName"),		// SFI_ORIGINALFILENAME
	_T("ProductName"),			// SFI_PRODUCTNAME
	_T("ProductVersion"),		// SFI_PRODUCTVERSION
	_T("Comments"),				// SFI_COMMENTS
	_T("LegalTrademarks"),		// SFI_LEGALTRADEMARKS
	_T("PrivateBuild"),			// SFI_PRIVATEBUILD
	_T("SpecialBuild")			// SFI_SPECIALBUILD
};

// standard constructor
CFileVersionInfo::CFileVersionInfo()
{
	m_pVersionInfo = NULL;
	NTver = (GetOSVersion()!=0);
	Clear();	// initialize all to zero
}

// copy constructor
CFileVersionInfo::CFileVersionInfo(const CFileVersionInfo& fvi)
{
	m_pVersionInfo = NULL;
	NTver = (GetOSVersion() != 0);
	CString strFile = fvi.GetFilename();
	if (!strFile.IsEmpty())	ReadVersionInfo(strFile);
	else Clear();	// initialize all to zero
}

// initialize from a specific file (by name)
CFileVersionInfo::CFileVersionInfo(LPCTSTR lpszFile)
{
	m_pVersionInfo = NULL;
	NTver = (GetOSVersion() != 0);
	if (lpszFile && _tcslen(lpszFile))
	{
//		EXE_FILE le( (char *) lpszFile );
		PE_EXE pPE( lpszFile );
		
		if ((pPE.GetExeType()==exeType_LE) && NTver)
			if (ReadVersionInfo16(pPE))
				m_strFilename = lpszFile;
//		if (pPE.Is64bits() && !NTver) NTver = FALSE;
		ReadVersionInfo(lpszFile);
	}
	else Clear();	// initialize all to zero
}

BOOL CFileVersionInfo::GetOSVersion()
{
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx (&osvi);
//		if ((osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) && (osvi.dwMajorVersion > 4))
//		if ((osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) && (osvi.dwMajorVersion > 4))
	if ((osvi.dwPlatformId == VER_PLATFORM_WIN32_NT))
		return TRUE;
	return FALSE;
}

// perform clean up as necessary
CFileVersionInfo::~CFileVersionInfo()
{
	Clear();
}

// clear all data
void CFileVersionInfo::Clear()
{
	m_strFilename.Empty();
	m_dwTransInfo = 0L;
	memset(&m_fixedFileInfo, 0, sizeof(VS_FIXEDFILEINFO));
	if (m_pVersionInfo)
	{
		delete m_pVersionInfo;
		m_pVersionInfo = NULL;
	}
}	// Clear


#pragma pack( push, pre_vxd_ver, 1 )
typedef struct _VXD_VERSION_RESOURCE {
  char  cType;
  WORD  wID;
  char  cName;
  WORD  wOrdinal;
  WORD  wFlags;
  DWORD dwResSize;
  BYTE  bVerData;
} VXD_VERSION_RESOURCE, *PVXD_VERSION_RESOURCE;
#pragma pack( pop, pre_vxd_ver )


BOOL CFileVersionInfo::ReadVersionInfo16(EXE_FILE &le)
{
	Clear();			// clear previous data first

	PIMAGE_VXD_HEADER pLXHeader = (PIMAGE_VXD_HEADER) le.GetTranslatedPtr(le.GetSecondaryHeaderOffset());
	PVXD_VERSION_RESOURCE   pVerRes    = NULL;
	LPVOID pRawRes;
	UINT	m_fileVersionSize = pLXHeader->e32_winreslen;
	m_fileVersionBlockSize = m_fileVersionSize;
	if (!m_fileVersionSize) return FALSE;	// no version information available

	pVerRes = (VXD_VERSION_RESOURCE *) le.GetTranslatedPtr((DWORD) pLXHeader->e32_winresoff);
	if (pVerRes->cName > 0)
	{  // ???????????????????
		size_t szKeySize = strlen(&pVerRes->cName);
#ifdef _DEBUG
		AfxMessageBox(_T("??????"));
#endif
		pVerRes = (VXD_VERSION_RESOURCE *) (((BYTE *) pVerRes) + 13 );
	}
	if (pVerRes->dwResSize <= m_fileVersionSize)
		m_fileVersionSize = pVerRes->dwResSize;
	pRawRes = &(pVerRes->bVerData);
	m_pVersionInfo = new BYTE[m_fileVersionSize ];
	CopyMemory(	m_pVersionInfo, pRawRes, m_fileVersionSize );

	LPVOID	lpVoid;
	if (::VerQueryValue(m_pVersionInfo, _T ("\\"), &lpVoid, &m_fileVersionSize))
		memcpy(&m_fixedFileInfo, lpVoid, m_fileVersionSize); /**/
#ifdef _DEBUG
	else AfxMessageBox(_T("Pas de struct m_fixedFileInfo"));
#endif
	if (ReadBlock()) return TRUE;
	return FALSE;
}

// read the file's version information
BOOL CFileVersionInfo::ReadVersionInfo(const CString& strFile)
{
	DWORD	dwDummy;
	LPVOID	lpVoid;
	all = _T("");
	Clear();			// clear previous data first
	
	UINT	m_fileVersionSize;
	m_fileVersionSize = ::GetFileVersionInfoSize((LPTSTR) (LPCTSTR) strFile, &dwDummy);
	m_fileVersionBlockSize = m_fileVersionSize;
	if (!m_fileVersionSize) return FALSE;	// no version information available
	m_pVersionInfo = new BYTE[m_fileVersionSize];

	// retrieve version information block
	if (!::GetFileVersionInfo ((LPTSTR) (LPCTSTR) strFile,  0L, m_fileVersionSize, m_pVersionInfo)) 
	{
		delete m_pVersionInfo;
		m_pVersionInfo = NULL;
		return FALSE;
	}

	// Extract VS_FIXEDFILEINFO part from memory block. This part contains
	// language independant version information.
	if (!::VerQueryValue(m_pVersionInfo, _T ("\\"), &lpVoid, &m_fileVersionSize))
	{
		delete m_pVersionInfo;
		m_pVersionInfo = NULL;
		return FALSE;
	}
	memcpy(&m_fixedFileInfo, lpVoid, m_fileVersionSize);

	// Extract translation "table" (Contains two WORD entries: Language Id and Charset).
	// This information is used to retrieve language dependant parts
	// of version info (see "GetVersionInfo" member function).
	// Parameter nSize indicates the number of available version info blocks.
	// (4 bytes for each, see Remarks above).
	// **********************************************************************
	BOOL bvi = (NTver)?ReadBlockNT():ReadBlock();
	if (::VerQueryValue (m_pVersionInfo, _T("\\VarFileInfo\\Translation"), &lpVoid, &m_fileVersionSize))
	{
		bvi++;
		memcpy(&m_dwTransInfo, lpVoid, sizeof(DWORD));
	}

	if (!bvi)
	{
		Clear();
		return FALSE;
	}

	// Everything is OK, now save filename
	m_strFilename = strFile;
	return TRUE;
}	// ReadVersionInfo

//***************************************************************
#define getval(n) ((sTemp[n] > '9')?(sTemp[n]>'F'? sTemp[n]-87:sTemp[n]-55):sTemp[n]-'0')
#define BufferLen 1024 
VS_Dflt_NT *CFileVersionInfo::ReadStringFI_NT(VS_Dflt_NT *vi_st)
{
	WCHAR   *vi_string;
	BYTE    *decal = (BYTE *) vi_st;
	BYTE    *start = (BYTE *) vi_st;
	size_t	szKeySize;
	INT		szSize = vi_st->wLength;
	CString	temp;

	szKeySize = ((wcslen(vi_st->szKey) + 1) << 1) + sizeof(VS_Dflt_NT);
	while (szKeySize%4)	szKeySize++;
	decal += szKeySize;
	//StringTable
	vi_st = (VS_Dflt_NT *) ( decal );
	do
	{
		if (vi_st->wValueLength == 0 && vi_st->szKey[0] == 48)
		{
			WCHAR szLang[5];
			wcsncpy(szLang, vi_st->szKey, 4);
			szLang[4] = 0;
			UINT id = (UINT)wcstoul(szLang, NULL, 16);
			temp.Format(_T("\nVersion language : %s\n"), (LPCTSTR)GetLanguageName(id));
			all += temp;

			szKeySize = ((wcslen(vi_st->szKey) + 1) << 1) + sizeof(VS_Dflt_NT);
			while (szKeySize%4)	szKeySize++;
			decal += szKeySize;
			vi_st = (VS_Dflt_NT *) ( decal );
		}
//		if (vi_st->wType == 1)
		{
			temp.Format(_T("      %ls"), vi_st->szKey);
			all += temp;
			szKeySize = ((wcslen(vi_st->szKey) + 1) << 1) + sizeof(VS_Dflt_NT);
			while (szKeySize%4)	szKeySize++;
			decal += szKeySize;

			//String
			vi_string = (WCHAR *) ( decal );
			if (vi_st->wValueLength)
			{
				CString strVal(vi_string, vi_st->wValueLength);
				temp.Format(_T("\t: %s\n"), (LPCTSTR)strVal);
				all += temp;
				szKeySize = vi_st->wLength - szKeySize;
			} else
			{
				all += _T("\t:\n");
				szKeySize = 0;
			}
			while (szKeySize%4)	szKeySize++;
			decal += szKeySize;
		}
		// next
		vi_st = (VS_Dflt_NT *) ( decal );
	} while (szSize > (decal - start));
	return vi_st;
}

VS_Dflt_NT *CFileVersionInfo::ReadVarFI_NT(VS_Dflt_NT *vi_st)
{
	DWORD    *vi_val;
	BYTE		*decal;
	BYTE		*start = (BYTE *) vi_st;
	size_t		szKeySize;
	INT		szSize = vi_st->wLength;
	CString	temp;

	decal = (BYTE *) vi_st;
	do
	{
		szKeySize = ((wcslen(vi_st->szKey) + 1) << 1) + sizeof(VS_Dflt_NT);
		while (szKeySize%4)	szKeySize++;
		decal += szKeySize;

		vi_st = (VS_Dflt_NT *) ( decal );
		szKeySize = ((wcslen(vi_st->szKey) + 1) << 1) + sizeof(VS_Dflt_NT);
		while (szKeySize%4)	szKeySize++;
		decal += szKeySize;

		vi_val = (DWORD *) ( decal );
		decal += (vi_st->wValueLength);

		// next
		vi_st = (VS_Dflt_NT *) ( decal );
	} while (szSize > (decal - start));
	return vi_st;
}


UINT CFileVersionInfo::TestBlockFI(WCHAR *szKey) /// NT ver
{
	if (!wcscmp(szKey, L"StringFileInfo"))
		return 1;
	if (!wcscmp(szKey, L"VarFileInfo"))
		return 2;
	return 0;
}

BOOL CFileVersionInfo::ReadBlockNT()
{
   VS_Dflt_NT *vi_st ;
   UINT      szSize=0;
   CString temp;
	BOOL ret = FALSE;

   if (!m_pVersionInfo) return FALSE;
   VS_VERSIONINFO_NT  *vi = (VS_VERSIONINFO_NT *) m_pVersionInfo;
   if (wcscmp(vi->szKey, L"VS_VERSION_INFO")) return FALSE;

   vi_st = (VS_Dflt_NT *) ( m_pVersionInfo + sizeof(VS_VERSIONINFO_NT) );
   do {
      switch(TestBlockFI(vi_st->szKey))
      {
         case 0: return ret;
         case 1:    //StringFileInfo
               vi_st = ReadStringFI_NT(vi_st);
			   ret++;
            break;
         case 2:  //VarFileInfo
               vi_st =  ReadVarFI_NT(vi_st);
			   ret++;
            break;
      }
// test vi_st  m_fileVersionBlockSize
	  if ((((BYTE *) m_pVersionInfo + m_fileVersionBlockSize) - (BYTE *) vi_st) <= 0)
	  {
#ifdef _DEBUG
		AfxMessageBox(_T("??????"));
#endif
		  return ret;
	  }
   } while (TRUE);
   return TRUE;
}


//*************   W95/98/Me
#undef getval
#define getval(n) ((vi_st->szKey[n] > '9')?(vi_st->szKey[n]>'F'? vi_st->szKey[n]-87:vi_st->szKey[n]-55):vi_st->szKey[n]-'0')
VS_Dflt *CFileVersionInfo::ReadStringFI(VS_Dflt *vi_st)
{
	char    *vi_string;
	BYTE    *decal = (BYTE *) vi_st;
	BYTE    *start = (BYTE *) vi_st;
	size_t		szKeySize;
	INT		szSize = vi_st->wLength;
	CString	temp;

	szKeySize = strlen(vi_st->szKey)+1;
	while (szKeySize%4)	szKeySize++;
	decal += sizeof(VS_Dflt) + (szKeySize );

	//StringTable
	vi_st = (VS_Dflt *) ( decal );
	do
	{
		if (vi_st->wValueLength == 0 && vi_st->wLength == 0 && vi_st->wType == 0)
		{
			decal += sizeof(VS_Dflt);
			vi_st = (VS_Dflt *) ( decal );
		}
		if (vi_st->wValueLength == 0 && vi_st->szKey[0] == 48)
		{
			UINT id = getval(0)*4096+getval(1)*256+getval(2)*16+getval(3);
			temp.Format(_T("\nVersion language : %s\n"), (LPCTSTR)GetLanguageName(id));
			all += temp;

			szKeySize = strlen(vi_st->szKey)+1;
			while (szKeySize%4)	szKeySize++;
			decal += sizeof(VS_Dflt) + (szKeySize );
			vi_st = (VS_Dflt *) ( decal );
		}

		temp.Format(_T("      %hs"),vi_st->szKey);
		all += temp;
		szKeySize = strlen(vi_st->szKey)+1;
		while (szKeySize%4)	szKeySize++;
		decal += sizeof(VS_Dflt) + (szKeySize );

		//String 
		vi_string = (char *) ( decal );
		if (vi_st->wValueLength)
		{	
			char *pst = new char[vi_st->wValueLength+1];
			strncpy(pst, vi_string, vi_st->wValueLength);
			pst[vi_st->wValueLength]='\0';
			temp.Format(_T("\t: %hs\n"),pst);
			all += temp;
			szKeySize = vi_st->wValueLength;
			delete [] pst;
		} else 
		{
			all += _T("\t:\n");
			szKeySize = 0;
		}
		while (szKeySize%4)	szKeySize++;
		decal += szKeySize;
		// next
		vi_st = (VS_Dflt *) ( decal );
	} while (szSize > (decal - start));
	return vi_st;
}

VS_Dflt *CFileVersionInfo::ReadVarFI(VS_Dflt *vi_st)
{
	DWORD    *vi_val;
	BYTE		*decal;
	BYTE		*start = (BYTE *) vi_st;
	size_t		szKeySize;
	INT		szSize = vi_st->wLength;
//	CString	temp;

	decal = (BYTE *) vi_st;
	do
	{
		szKeySize = strlen(vi_st->szKey)+1;
		while (szKeySize%4)	szKeySize++;
		decal += sizeof(VS_Dflt) + (szKeySize );

		vi_st = (VS_Dflt *) ( decal );
		szKeySize = strlen(vi_st->szKey)+1;
		while (szKeySize%4)	szKeySize++;
		decal += sizeof(VS_Dflt) + (szKeySize );

		vi_val = (DWORD *) ( decal );
		decal += (vi_st->wValueLength);

		// next
		vi_st = (VS_Dflt *) ( decal );
	} while (szSize > (decal - start));
	return vi_st;
}


BOOL CFileVersionInfo::ReadBlock()
{
	VS_Dflt *vi_st ;
	UINT		szSize=0;
	CString temp;
	BOOL Return = FALSE;

	if (!m_pVersionInfo) return FALSE;
	VS_VERSIONINFO  *vi = (VS_VERSIONINFO *) m_pVersionInfo;
		if (strcmp(vi->szKey, "VS_VERSION_INFO")) return FALSE;
	vi_st = (VS_Dflt *) ( m_pVersionInfo + sizeof(VS_VERSIONINFO) );
	do {	
		if (!strcmp(vi_st->szKey, "StringFileInfo"))
		{
			vi_st = ReadStringFI(vi_st);
			Return = TRUE;
		}
		else if (!strcmp(vi_st->szKey, "VarFileInfo"))
			vi_st =  ReadVarFI(vi_st);
		else return Return;
	} while (TRUE);
}

//********************************************

typedef struct
{
    WORD    flag;
    PSTR    name;
} CHARSET_DESCRIPTIONS;


CHARSET_DESCRIPTIONS CharSetName[] = 
{
	{874, "874 (ANSI - Thai)"},
	{932, "932 (ANSI - Japan (Shift - JIS X-0208))"},
	{936, "936 (ANSI - Chinese (PRC, Singapore))"},
	{937, "937 (ANSI - Chinese Host merged: CP 037 + CP 835"}, 
	{939, "939 (ANSI - Japanese Host merged CP 1027 + CP 4396)"}, 
	{949, "949 (ANSI - Korean (Shift - KSC 5601))"},
	{950, "950 (ANSI - Chinese (Taiwan Region; Hong Kong SAR, PRC))"},
	{1026, "1026 (ANSI - IBM EBCDIC Turkish)"}, 
	{1200, "1200 (ANSI - Unicode (BMP of ISO 10646))"},
	{1250, "1250 (ANSI - Latin-2 (Eastern European) )"},
	{1251, "1251 (ANSI - Cyrillic)"},
	{1252, "1252 (ANSI - Latin 1 (US, Western Europe))"},
	{1253, "1253 (ANSI - Greek)"},
	{1254, "1254 (ANSI - Turkish)"},
	{1255, "1255 (ANSI - Hebrew)"},
	{1256, "1256 (ANSI - Arabic)"},
	{1257, "1257 (ANSI - Baltic)"},
	{1258, "1258 (ANSI - MS Windows Vietnamese)"}, 
	{1361, "1361 (ANSI - Korean (Johab)"}
};

#define NUMBER_CHARSET_DESCRIPTIONS \
    (sizeof(CharSetName) / sizeof(CHARSET_DESCRIPTIONS))

CString CFileVersionInfo::GetCharSetName() const
{ 
	WORD cs = GetCharSet();
    for (int i=0; i < NUMBER_CHARSET_DESCRIPTIONS ; i++ )
        if ( cs == CharSetName[i].flag )
            return CharSetName[i].name;
	return _T("Not referenced");
}	// GetCharSetName


// Get a specific aspect of the file version resource (see above)
CString CFileVersionInfo::GetVersionInfo(int nIndex) const
{
	// First check if object data and index range are valid
	if ((nIndex < SFI_FIRST)	|| 
		(nIndex > SFI_LAST)		||
		(!m_pVersionInfo)
	   ) return CString(_T(""));

	CString strRes,
			strFmt;
	LPVOID	lpInfo;
	UINT	nLen;

	// Create a search string for the VerQueryValue function 
	// (See SDK documentation for details)
	strFmt.Format(m_lpszFmtStringFileInfo, 
				  GetLanguageId(),
				  GetCharSet(),
				  m_lpszStringFileInfos[nIndex]
				 );

	// Get the requested version information string
	// ********************************************
	if (::VerQueryValue(m_pVersionInfo, strFmt.GetBuffer(_MAX_PATH), 
						&lpInfo, &nLen)
	   ) strRes = (const TCHAR*) lpInfo;

	return strRes;

}	// GetVersionInfo

// Determine, if a specific aspect of the file version info is present
BOOL CFileVersionInfo::IsVersionInfoAvailable(int nIndex) const
{
	// First check if object data and index range are valid
	// ****************************************************
	if ((nIndex < SFI_FIRST)	|| 
		(nIndex > SFI_LAST)		||
		(!m_pVersionInfo)
	   ) return FALSE;

	CString strRes,
			strFmt;
	LPVOID	lpInfo;
	UINT	nLen;

	// Create a search string for the VerQueryValue function 
	// (See SDK documentation for details)
	// *****************************************************
	strFmt.Format(m_lpszFmtStringFileInfo, 
				  GetLanguageId(),
				  GetCharSet(),
				  m_lpszStringFileInfos[nIndex]
				 );

	// See if requested version information string can be found
	// ********************************************************
	return ::VerQueryValue(m_pVersionInfo, strFmt.GetBuffer(_MAX_PATH), 
						   &lpInfo, &nLen );

}	// IsVersionInfoAvailable


// get name of targwet operating system
CString CFileVersionInfo::GetTargetOs(UINT nID) const
{
	CString str, 
			strRes;
	short   n = -1;
//	if (!str.LoadString(nID)) return CString(_T(""));
	str = OS_STR;

	switch (m_fixedFileInfo.dwFileOS)
	{
	case VOS_DOS:				n = OS_DOS;			break;
	case VOS_DOS_WINDOWS16:		n = OS_WIN16_DOS;	break;
	case VOS_DOS_WINDOWS32:		n = OS_WIN32_DOS;	break;
	case VOS__WINDOWS16:		n = OS_WIN16;		break;
	case VOS__WINDOWS32:		n = OS_WIN32;		break;
	case VOS__PM16:				n = OS_PM16;		break;
	case VOS__PM32:				n = OS_PM32;		break;
	case VOS_OS216:				n = OS_OS216;		break;
	case VOS_OS232:				n = OS_OS232;		break;
	case VOS_OS216_PM16:		n = OS_OS216_PM16;	break;
	case VOS_OS232_PM32:		n = OS_OS232_PM32;	break;
	case VOS_NT:				n = OS_WINNT;		break;
	case VOS_NT_WINDOWS32:		n = OS_WIN32_NT;	break;
	default:					n = OS_UNKNOWN;		break;
	}
	return (AfxExtractSubString(strRes, str, n)) ? strRes : CString(_T(""));
}	// GetTargetOs


// get file type description
CString CFileVersionInfo::GetFileType(UINT nID) const
{
	CString str, 
			strRes;
	short   n = -1;
//	if (!str.LoadString(nID)) return CString(_T(""));
	str = Type_STR;

#ifndef VFT2_DRV_COMM
#define VFT2_DRV_COMM              0x0000000AL
#endif
#ifndef VFT2_DRV_INPUTMETHOD
#define VFT2_DRV_INPUTMETHOD       0x0000000BL
#endif
#ifndef VFT2_DRV_VERSIONED_PRINTER
#define VFT2_DRV_VERSIONED_PRINTER 0x0000000CL
#endif

	switch (m_fixedFileInfo.dwFileType)
	{
	case VFT_DRV:
		switch (m_fixedFileInfo.dwFileSubtype)
		{
		case VFT2_DRV_DISPLAY:		n = FT_DRV_DISPLAY;		break;
		case VFT2_DRV_INSTALLABLE:	n = FT_DRV_INSTALLABLE;	break;
		case VFT2_DRV_KEYBOARD:		n = FT_DRV_KEYBOARD;	break;
		case VFT2_DRV_LANGUAGE:		n = FT_DRV_LANGUAGE;	break;
		case VFT2_DRV_MOUSE:		n = FT_DRV_MOUSE;		break;
		case VFT2_DRV_NETWORK:		n = FT_DRV_NETWORK;		break;
		case VFT2_DRV_PRINTER:		n = FT_DRV_PRINTER;		break;
		case VFT2_DRV_SOUND:		n = FT_DRV_SOUND;		break;
		case VFT2_DRV_SYSTEM:		n = FT_DRV_SYSTEM;		break;
		case VFT2_DRV_COMM:			n = FT_DRV_COMM;		break;
		case VFT2_DRV_INPUTMETHOD:	n = FT_DRV_INPUTMETHOD;	break;
		case VFT2_DRV_VERSIONED_PRINTER: n = FT_DRV_VERSIONED_PRINTER; break;
		case VFT2_UNKNOWN:
		default:					n = FT_DRV_UNKNOWN;		break;
		}
		break;
	case VFT_FONT:
		switch (m_fixedFileInfo.dwFileSubtype)
		{
		case VFT2_FONT_RASTER:		n = FT_FONT_RASTER;		break;
		case VFT2_FONT_TRUETYPE:	n = FT_FONT_TRUETYPE;	break;
		case VFT2_FONT_VECTOR:		n = FT_FONT_VECTOR;		break;
		case VFT2_UNKNOWN:
		default:					n = FT_FONT_UNKNOWN;	break;
		}
		break;
	case VFT_APP:					n = FT_APP;				break;
	case VFT_DLL:					n = FT_DLL;				break;
	case VFT_STATIC_LIB:			n = FT_STATIC_LIB;		break;
	case VFT_VXD:					n = FT_VXD;				break;
	case VFT_UNKNOWN:
	default:						n = FT_UNKNOWN;			break;
	}	
	return (AfxExtractSubString(strRes, str, n)) ? strRes :
												   CString(_T(""));
}	// GetFileType