//==========================================
// Matt Pietrek
// Microsoft Systems Journal, Feb 1997
// FILE: MODULEFILEINFO.H
//==========================================
#ifndef __MODULEFILEINFO_H__
#define __MODULEFILEINFO_H__

class MODULE_DEPENDENCY_LIST;
//
// This structure represents one executable file in a module dependency list.
// Both the base filename and the complete path are stored.
class MODULE_FILE_INFO
{
private:
    MODULE_FILE_INFO * m_pNext;
	long	m_Address;
	CStringList m_Flist;
	TCHAR	*m_szBaseName;
	TCHAR	m_szFullName[MAX_PATH];
	TCHAR	m_szApiSetName[MAX_PATH];	// Original API Set contract name, if any
	BOOL	m_bFound, m_bIFound, m_Tested;
	
	friend class MODULE_DEPENDENCY_LIST;

public:
    MODULE_FILE_INFO( LPCTSTR pszFileName, long address, BOOL Found);
    ~MODULE_FILE_INFO( void ){}

    LPCTSTR GetBaseName( void ){ return m_szApiSetName[0] ? m_szApiSetName : m_szBaseName; }
    LPCTSTR GetFullName( void ){ return m_szFullName; }
	CString GetDisplayName( int padTo = 0 );
	void SetApiSetName(LPCTSTR pszName) { lstrcpyn(m_szApiSetName, pszName, _countof(m_szApiSetName)); }
	void SetImportedFPresent(BOOL pres) { m_bIFound = pres; };
	BOOL TestFunction( );
	CStringList *GetFList() { return &m_Flist; }

// *** FG	
	BOOL IsIFPresent() { return m_bIFound; };
	BOOL IsModuleFound() { return m_bFound; };
	void AddFunc(CString str) { m_Flist.AddTail(str); }
};

typedef MODULE_FILE_INFO * PMODULE_FILE_INFO;

#endif
