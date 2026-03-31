//==========================================
// Matt Pietrek
// Microsoft Systems Journal, Feb 1997
// FILE: MODULEFILEINFO.CPP
//==========================================
#include "stdafx.h"

#include <windows.h>
#include "modulefileinfo.h"
#include "apisetresolve.h"

#include "..\..\..\common\ffile.h"
extern BOOL b_W95Protect;

MODULE_FILE_INFO::MODULE_FILE_INFO( LPCTSTR pszFileName, long address, BOOL found)
{
    m_pNext = NULL;
	m_bFound = found;
	m_bIFound = TRUE;
	m_Tested = FALSE;
	m_Address = address;
	m_szApiSetName[0] = _T('\0');
    // Initialize the new MODULE_FILE_INFO, and stick it at the head of the list.
    lstrcpyn( m_szFullName, pszFileName, _countof(m_szFullName) );
	if (found) m_szBaseName = (TCHAR *) ::GetBaseName(m_szFullName);
	else m_szBaseName = m_szFullName;
}

CString MODULE_FILE_INFO::GetDisplayName( int padTo )
{
	LPCTSTR displayBase = m_szApiSetName[0] ? m_szApiSetName : m_szBaseName;
	// Show "name (resolved path)" when they differ
	if (m_bFound && _tcsicmp(displayBase, m_szFullName) != 0)
	{
		CString name;
		int len = (int)_tcslen(displayBase);
		int pad = (padTo > len) ? (padTo - len) : 2;  // At least 2 spaces
		name.Format(_T("%s%*s%s"), displayBase, pad, _T(""), m_szFullName);
		return name;
	}
	return displayBase;
}

BOOL MODULE_FILE_INFO::TestFunction( CDllHandleCache* pHandleCache )
{
	if ( m_Tested )
		return m_bIFound;

	BOOL ret=TRUE;
	if ( m_bFound )
	{
		// If this is an unresolved API Set name (no physical file found,
		// but marked as found because it's a valid virtual DLL), try to
		// resolve it now. If still unresolvable, assume functions are present.
		if (IsApiSetName(m_szBaseName))
		{
			TCHAR szResolved[MAX_PATH];
			if (ResolveApiSetDll(m_szFullName, szResolved, MAX_PATH))
				lstrcpyn(m_szFullName, szResolved, _countof(m_szFullName));
			else
			{
				m_Tested = TRUE;
				return (m_bIFound = TRUE);
			}
		}

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
				if (pHandleCache)
				{
					h = pHandleCache->GetHandle(szPath, b_W95Protect);
				}
				else
				{
					if (b_W95Protect)
						h = LoadLibraryEx(szPath, NULL, LOAD_LIBRARY_AS_DATAFILE);
					else
						h = LoadLibraryEx(szPath, NULL, DONT_RESOLVE_DLL_REFERENCES);
				}
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
					if (!pHandleCache)
						FreeLibrary( h );	// Only free if not cached
				}
			}
			if ( fHasPath ) SetCurrentDirectory( szOriginalPath );
		}
	}
	return (m_bIFound = ret);
}
