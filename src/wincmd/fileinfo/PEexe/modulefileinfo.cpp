//==========================================
// Matt Pietrek
// Microsoft Systems Journal, Feb 1997
// FILE: MODULEFILEINFO.CPP
//==========================================
#include "stdafx.h"

#include <windows.h>
#include "modulefileinfo.h"

#include "..\..\..\common\ffile.h"
extern BOOL b_W95Protect;

MODULE_FILE_INFO::MODULE_FILE_INFO( LPCTSTR pszFileName, long address, BOOL found)
{
    m_pNext = NULL;
	m_bFound = found;
	m_Address = address;
    // Initialize the new MODULE_FILE_INFO, and stick it at the head of the list.
    lstrcpyn( m_szFullName, pszFileName, _countof(m_szFullName) );
	if (found) m_szBaseName = (TCHAR *) ::GetBaseName(m_szFullName);
	else m_szBaseName = m_szFullName;
}

BOOL MODULE_FILE_INFO::TestFunction( )
{
	BOOL ret=TRUE;
	if ( m_bFound )
	{
		m_Tested = TRUE;
		POSITION pos = m_Flist.GetHeadPosition();
		if (pos)
		{
			LPTSTR pszDontCare;
			TCHAR szPath[MAX_PATH];
			TCHAR szOriginalPath[MAX_PATH];

			BOOL fHasPath = FALSE;
			LPTSTR pszJustPath = _tcsdup( m_szFullName );
			LPTSTR pszEnd = _tcsrchr( pszJustPath, _T('\\') );
			if ( pszEnd )
			{
				*pszEnd = 0;	 /// Strip off the filename
				fHasPath = TRUE;
			}

			if ( fHasPath )
			{
				GetCurrentDirectory(MAX_PATH, szOriginalPath);  // Save original dir
				SetCurrentDirectory( pszJustPath  );				 // Switch to app's dir
			}
			free (pszJustPath  );

			if (SearchPath(0, m_szFullName, 0, MAX_PATH, szPath, &pszDontCare))
			{
				HINSTANCE h;
				if (b_W95Protect)
					h = LoadLibraryEx(szPath, NULL, LOAD_LIBRARY_AS_DATAFILE);
				else 
					h = LoadLibraryEx(szPath, NULL, DONT_RESOLVE_DLL_REFERENCES);  				
				if (h)
				{
					CString func;
					do {
						func = m_Flist.GetNext(pos);
						if (_tcsncmp(func, _T("<invalid name>"), 14) == 0) continue;
						if (_tcsncmp(_T("ordinal "), func, 8)==0)
						{
							if (!GetProcAddress( h, MAKEINTRESOURCEA(_ttoi((LPCTSTR) func + 8))))
							{ pos = NULL; ret = FALSE; }
						}
						else if (!GetProcAddress( h, CT2A(func)))
						{ ret = FALSE; pos = NULL; }
					} while( pos );
					FreeLibrary( h );		// FG 
				} 
			} 
			if ( fHasPath ) SetCurrentDirectory( szOriginalPath );
		}
	}
	return (m_bIFound = ret);
}
