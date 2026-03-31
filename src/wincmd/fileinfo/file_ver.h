// ----------------------------------------------------------------------------
//		File:		file_ver.h
//					----------
//
//		Content:	Declaration of class CFileVersionInfo and inline funcs.
//					Manages the retrieval of various information from a
//					Win32 file image.
//
//
//		Created:		01/24/2002
//						Thomas Weller
//
//		Last modified:	02/02/2002
//						Thomas Weller
// ----------------------------------------------------------------------------
//	Remarks
//	-------
//
//	- The "version.lib" library module must be added to project settings.
//
//	- A file's version resource may contain more than one version info block.
//	  If so, only the first one will be retrieved.
//
//	- Functions marked as "// inline" are defined in file_ver.inl.
// ----------------------------------------------------------------------------

#if !defined(AFX_FILE_VER_H__BBB1EB1A_076F_405A_B71D_FDFA4256E34A__INCLUDED_)
#define AFX_FILE_VER_H__BBB1EB1A_076F_405A_B71D_FDFA4256E34A__INCLUDED_

//-----------------------------------------------------------------------------
//		includes
//-----------------------------------------------------------------------------

#include <winver.h>		// Win32 API File Installation Library
//#include "fileinfo\EXEFILE.H"
#include "peexe\PEEXE.H"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Add FG 
#define IDS_INFOITEMS                   1
#define IDS_OS                          2
#define IDS_FT                          3

#define Name_STR _T("Company Name\nFile Description\nFile Version\nInternal Name\nLegal Copyright\nOriginal Filename\nProduct Name\nProduct Version\nComments\nLegal Trademarks\nPrivate Build\nSpecial Build")
#define OS_STR _T("MS-DOS\nWin16\nWin32\nWin16 (MS-DOS)\nWin32 API (MS-DOS)\nOS/2 16bit\nOS/2 32bit\nWindows NT\nWin32 API (Windows NT)\nUnknown")
#define Type_STR _T("Display Driver\nInstallable Driver\nKeyboard Driver\nLanguage Driver\nMouse Driver\nNetwork Driver\nPrinter Driver\nSound Driver\nSystem Driver\nComm Driver\nInput Method Driver\nVersioned Printer Driver\nUnknown Driver Type\nRaster Font\nTruetype Font\nVector Font\nFont\nApplication\nDynamic Link Library\nStatic Link Library\nVirtual Device\nUnknown File Type")
// BOOL AfxExtractSubString(CString& rString, LPCTSTR lpszFullString, int iSubString, TCHAR chSep = '\n');

//*****************************************************************************
//							Constants
//*****************************************************************************

// OS_xxx: various types of operating systems.
// Indices are intended to correspond to an application supplied string 
// resource which contains a list of names.
// See the GetTargetOs member function for details.
// *********************************************************************
#define OS_DOS					0
#define OS_WIN16				1
#define OS_WIN32				2
#define OS_WIN16_DOS			3
#define OS_WIN32_DOS			4
#define OS_OS216				5
#define OS_OS232				6
#define OS_WINNT				7
#define OS_WIN32_NT				8
#define OS_UNKNOWN				9


// FT_xxx: various file types.
// Indices are intended to correspond to an application supplied string 
// resource which contains a list of names.
// See the GetFiletype member function for details.
// *********************************************************************
#define FT_DRV_DISPLAY			 0
#define FT_DRV_INSTALLABLE		 1
#define FT_DRV_KEYBOARD			 2
#define FT_DRV_LANGUAGE			 3
#define FT_DRV_MOUSE			 4
#define FT_DRV_NETWORK			 5
#define FT_DRV_PRINTER			 6
#define FT_DRV_SOUND			 7
#define FT_DRV_SYSTEM			 8
#define FT_DRV_COMM				 9
#define FT_DRV_INPUTMETHOD		10
#define FT_DRV_VERSIONED_PRINTER 11
#define FT_DRV_UNKNOWN			12
#define FT_FONT_RASTER			13
#define FT_FONT_TRUETYPE		14
#define FT_FONT_VECTOR			15
#define FT_FONT_UNKNOWN			16
#define FT_APP					17
#define FT_DLL					18
#define FT_STATIC_LIB			19
#define FT_VXD					20
#define FT_UNKNOWN				21
#define FT_FIRST				FT_DRV_DISPLAY
#define FT_LAST					FT_UNKNOWN

// SFI_xxx: specific parts of information from a file's version info block.
// Indices correspond to m_lpszStringFileInfos[] string array.
// See GetVersionInfo member function for usage.
//*************************************************************************
#define SFI_COMPANYNAME			 0
#define SFI_FILEDESCRIPTION		 1
#define SFI_FILEVERSION			 2
#define SFI_INTERNALNAME		 3
#define SFI_LEGALCOPYRIGHT		 4
#define SFI_ORIGINALFILENAME	 5
#define SFI_PRODUCTNAME			 6
#define SFI_PRODUCTVERSION		 7
#define SFI_COMMENTS			 8
#define SFI_LEGALTRADEMARKS		 9
#define SFI_PRIVATEBUILD		10
#define SFI_SPECIALBUILD		11
#define SFI_FIRST				SFI_COMPANYNAME
#define SFI_LAST				SFI_SPECIALBUILD
		
//*****************************************************************************
//					CFileVersionInfo class declaration
//*****************************************************************************
struct VS_Dflt_NT { 
	WORD   wLength; 
   WORD   wValueLength; 
   WORD   wType; 
   WCHAR  szKey[]; //16
}; 

struct VS_Dflt { 
	WORD   wLength; 
	BYTE	wValueLength;  
	BYTE	wType;			
	char szKey[];			//  WORD  Padding1[]; 
}; 

struct VS_VERSIONINFO_NT { 
	WORD	wLength;			// 16b
	WORD	wValueLength;  // 16b 
	WORD	wType;			// 16b
	WCHAR szKey[16];			//  WORD  Padding1[]; 
	VS_FIXEDFILEINFO Value; //  WORD  Padding2; 
//	VS_Dflt  Children; //[]
};

struct VS_VERSIONINFO { 
	WORD	wLength;			// 16b
	BYTE	wValueLength;  
	BYTE	wType;			
	char szKey[16];			//  WORD  Padding1[]; 
	VS_FIXEDFILEINFO Value; //  WORD  Padding2; 
//	VS_Dflt  Children; //[]
};


class CFileVersionInfo  
{
// Construction/destruction
public:
	CFileVersionInfo();
	CFileVersionInfo(const CFileVersionInfo& fvi);
	CFileVersionInfo(LPCTSTR lpszFile);
	~CFileVersionInfo();

// Operations
public:
	BOOL		ReadVersionInfo(const CString& strFile);

// Win9x version
	BOOL		ReadBlock();
	VS_Dflt		*ReadStringFI(VS_Dflt *vi_st);
	VS_Dflt		*ReadVarFI(VS_Dflt *vi_st);
// WinNT version
	BOOL		ReadBlockNT();
	UINT		TestBlockFI(WCHAR *SZKey);
	VS_Dflt_NT *ReadStringFI_NT(VS_Dflt_NT *vi_st);
	VS_Dflt_NT *ReadVarFI_NT(VS_Dflt_NT *vi_st);


	BOOL		ReadVersionInfo16(EXE_FILE &le);
	BOOL		GetOSVersion();
	BOOL		RefreshVersionInfo();						// inline		
	void		Clear();

// Attributes
public:

	const CString&	GetFilename()				const;		// inline
	BOOL			IsValid()					const;		// inline

	// Language and Charset
	// ********************
	WORD		GetCharSet()					const;		// inline
	CString		GetCharSetName()				const;		// inline
	WORD		GetLanguageId ()				const;		// inline
	CString		GetLanguageName(WORD  id=0)				const;		// inline

	// File Version
	// ************
	DWORD		GetMajorFileVersion()			const;		// inline
	DWORD		GetMinorFileVersion()			const;		// inline
	CString		GetFileVersionString()			const;		// inline
	CString		GetMajorFileVersionString()		const;		// inline
	CString		GetMinorFileVersionString()		const;		// inline

	// Product Version
	// ***************
	DWORD		GetMajorProductVersion()		const;		// inline
	DWORD		GetMinorProductVersion()		const;		// inline
	CString		GetProductVersionString()		const;		// inline
	CString		GetMajorProductVersionString()	const;		// inline
	CString		GetMinorProductVersionString()	const;		// inline

	// Date
	// ****
	FILETIME	GetFileDate()					const;		// inline
	UINT		GetFileVersionSize()	{	return  m_fileVersionBlockSize; }

	// Operating System
	// ****************
	CString		GetTargetOs(UINT nID)			const;
	DWORD		GetTargetOs()					const;		// inline

	// Filetype
	// ********
	CString		GetFileType(UINT nID)			const;
	DWORD		GetFileType()					const;		// inline
	DWORD		GetFileSubtype()				const;		// inline

	CString		GetVersionInfo(int nIndex)			const;
	BOOL		IsVersionInfoAvailable(int nIndex)	const;

	// Boolean file attributes
	// ***********************
	BOOL		IsDebugVersion()				const;		// inline
	BOOL		IsPatched()						const;		// inline
	BOOL		IsPreRelease()					const;		// inline
	BOOL		IsPrivateBuild()				const;		// inline
	BOOL		IsSpecialBuild()				const;		// inline

	CString		GetAll()			{ return all;}
// Implementation
protected:
	CString	all;
	bool		NTver;
	CString				m_strFilename;
	LPBYTE				m_pVersionInfo;
	DWORD				m_dwTransInfo;
	VS_FIXEDFILEINFO	m_fixedFileInfo;
	UINT				m_fileVersionBlockSize;
	BOOL		IsFlagSet(DWORD dwFlag)			const;		// inline
	
// Static data members
private:
	static const LPCTSTR m_lpszFmtStringFileInfo;
	static const LPCTSTR m_lpszStringFileInfos[];

};	// CFileVersionInfo


#include "file_ver.inl"		// CFileVersionInfo inline functions



#endif // !defined(AFX_FILE_VER_H__BBB1EB1A_076F_405A_B71D_FDFA4256E34A__INCLUDED_)
