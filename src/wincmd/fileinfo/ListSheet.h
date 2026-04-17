#if !defined( __listsheet_H )
#define __listsheet_H

#if !defined(AFX_LISTSHEET_H__F0510B14_F615_46D5_BF14_A5E8433B7BD4__INCLUDED_)
#define AFX_LISTSHEET_H__F0510B14_F615_46D5_BF14_A5E8433B7BD4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ListSheet.h : header file
//
#include "..\common\PropertySheetRz.h"
#include "..\common\ListPagePty.h"
#include "ListExport.h"
#include "PageTree.h"
#include "ListOCX.h"
#include "Option.h"
#include "About.h"
/////////////////////////////////////////////////////////////////////////////
// CListSheet

class CListSheet : public CPropertySheetRz
{
	DECLARE_DYNAMIC(CListSheet)
// Var privées
	bool	m_bDarkMode;
	CBrush	m_brDarkBgAlt;
	WNDPROC	m_pfnOrigTabProc;	// Original tab control wndproc for subclassing
	static LRESULT CALLBACK DarkTabProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Construction
public:
	CListSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
// Attributes
public:
	CListpagePty	m_fi;
	CListpagePty	m_fi2;
	CPageTree		m_dll;
	CListExport		m_export;
	CListpagePty	m_clr;
	CPageTree		m_clr_deps;
	CListOcx		m_ocx;
	CListpagePty	m_disass;
    COption			m_option;
	CListpagePty	m_manifest;
	CAbout			m_about;

protected:
	void CleanUp();

// Operations
public:
	void Renew(PE_EXE *pPE);
	void PreSetDarkMode(bool bDark);
	void SetDarkMode(bool bDark);
	void ApplyDarkMode();
	bool IsDarkMode() const { return m_bDarkMode; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListSheet)
	protected:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CListSheet();
	afx_msg void OnDestroy( );
	BOOL SetPageTitle (int nPage, LPTSTR pszText);
	// Generated message map functions
protected:
	//{{AFX_MSG(CListSheet)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LISTSHEET_H__F0510B14_F615_46D5_BF14_A5E8433B7BD4__INCLUDED_)
#endif 