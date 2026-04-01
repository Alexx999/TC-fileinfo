//==========================================
// Matt Pietrek
// Microsoft Systems Journal, Feb 1997
// FILE: DEPENDENCYLIST.H
//==========================================
#ifndef __DEPLIST_H__
#define __DEPLIST_H__

#ifndef __MODULEFILEINFO_H__
#include "modulefileinfo.h"
#include "peexe.h"
#endif

class CDllPathCache;  // forward declaration

enum errModuleDependencyList {  errMDL_NO_ERROR,
                                errMDL_FILE_NOT_FOUND,
                                errMDL_NOT_PE_FILE,
                                errMDL_GENERAL_FAILURE };

//
// The MODULE_DEPENDENCY_LIST class creates a linked list of MODULE_FILE_INFO
// structures.  In theory, this list will represent every executable file
// loaded by the Win32 loader when the executable is loaded.  The class creates
// the list by starting with the file passed to the constructor, and recursing
// through all the import tables.
//
class MODULE_DEPENDENCY_LIST
{
protected:
	unsigned            m_type;				// ?
    unsigned            m_cModules;			// Number of modules in list
    PMODULE_FILE_INFO   m_pList;			// Pointer to head of linked list
	PMODULE_FILE_INFO   m_pListDelayed;		// Pointer to head of linked list 2
	unsigned            m_cDelayedModules;	// Number of modules in list 2
	PE_EXE				*m_pPE;				// Pointer to EXE module
	CDllPathCache		*m_pPathCache;		// Optional path resolution cache

public:

    MODULE_DEPENDENCY_LIST( PE_EXE *);
	MODULE_DEPENDENCY_LIST(  );
    void LoadPE( PE_EXE *pPE );
	void SetPathCache(CDllPathCache* p) { m_pPathCache = p; }

	~MODULE_DEPENDENCY_LIST( );
    
    BOOL IsValid( void ){ return (BOOL)(m_errorType == errMDL_NO_ERROR); }
    errModuleDependencyList GetErrorType( void ){ return m_errorType; }
    PSTR GetErrorString( void );
    PMODULE_FILE_INFO GetNextModule( PMODULE_FILE_INFO p ) { return p ? p->m_pNext : m_pList; } 
	PMODULE_FILE_INFO GetNextDelayedModule( PMODULE_FILE_INFO p ) { return p ? p->m_pNext : m_pListDelayed; } 
    PMODULE_FILE_INFO LookupModule( LPCTSTR pszFileName, BOOL fFullName );
	PMODULE_FILE_INFO LookupDelayedModule( LPCTSTR pszFileName );
    unsigned GetNumberOfModules( void ){ return m_cModules; }
	unsigned GetNumberOfDelayedModules( void ){ return m_cDelayedModules; }

private:
//  Recursively adds modules to the list 
//  errModuleDependencyList AddModule( PSTR pszFullName );
	errModuleDependencyList AddDirectModule();
	errModuleDependencyList AddDirectDelayedModule();
	errModuleDependencyList AddForwardedExportModules();
	void DeletePList(PMODULE_FILE_INFO pList);

    errModuleDependencyList m_errorType;    // Error type
};

#endif
