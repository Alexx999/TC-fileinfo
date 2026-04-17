    /***********************************************************/
    /****   Sub-routine :  *****/
    /***************  Auteur :    Fr GANNIER   *****************/
    /***************     Lab. Physio. Anim.    *****************/
    /***************      Fac. des Sciences    *****************/
    /***************        37000 Tours        *****************/
    /***********************************************************/

#if !defined( __FINFO_H )
#define __FINFO_H

#include "..\..\common\fdate.h"
#include "file_ver.h"
#include "..\common\wait.h"

#include "..\common\memorymappedfile.h"

#define ModuleName _T("fileinfo.wlx")

//CString CreateText0(PVOID ptr);
CString CreateText0(LPCTSTR);
CString CreateText1(PVOID ptr, CWait &wait);
CString CreateManifest(PVOID ptr, CWait &wait);

enum OBJ_FILE_TYPE
{
    OBJ_UNKNOWN = 0,
    OBJ_COFF_OBJ,
    OBJ_COFF_LIB,
    OBJ_OMF_OBJ,
    OBJ_OMF_LIB,
    OBJ_OMF_IMPLIB,
	OBJ_TL,
	OBJ_ELF
};

CString CreateText2(PVOID ptr, CWait &wait);
CString CreateText3(PVOID ptr, CWait &wait);
// Wide-string fill: the CLR header renders UTF-8 metadata that can contain
// chars outside the system ANSI codepage. See Utf8ToWide in clrdump.cpp.
CStringW CreateClrHeader(PVOID ptr, CWait &wait);

OBJ_FILE_TYPE DisplayObjectFile( MEMORY_MAPPED_FILE *pmmf );


void CreateParentTree(PVOID ptr, CTreeCtrl &tree, CWait &wait);
void CreateClrDepsTree(PVOID ptr, CTreeCtrl &tree, CWait &wait);
// void CreateOcxList(PVOID ptr, CListCtrl &list, CRichEditCtrl &edit);

// Opaque context for lazy child population in the DLL Dependency tree.
// Created by CreateParentTree, stored on the root tree item, cleaned up by CPageTree.
struct DllTreeContext;
void DeleteDllTreeContext(DllTreeContext* p);
// Lazily populates children of hParent when expanded for the first time.
// Returns TRUE if children were populated, FALSE if already populated or nothing to do.
BOOL DllTreeContext_ExpandNode(DllTreeContext* pCtx, CTreeCtrl& tree, HTREEITEM hParent);

#endif