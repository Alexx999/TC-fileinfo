// FileinfoListWnd.cpp : implementation file
//

#include "stdafx.h"
#include "FileinfoListWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFileinfoListWnd
CFileinfoListWnd::CFileinfoListWnd(PE_EXE *pPE):CListWnd()
{	
	m_ppropSheet = new CListSheet(_T("CList"));
	m_propSheet = m_ppropSheet;
/*  // all are dynamically called
	m_ppropSheet->m_ocx.SetPtr( pPE );
	m_ppropSheet->m_export.SetPtr( pPE );
	m_ppropSheet->m_fi2.SetPtr( pPE );
	m_ppropSheet->m_dll.SetPtr( pPE );	
*/
	m_pPE = pPE;
}

void CFileinfoListWnd::Renew(PE_EXE *pPE)
{
	if ( m_pPE ) { delete m_pPE; m_pPE = NULL; }
	m_pPE = pPE;
	m_ppropSheet->Renew(pPE);
}

/*
CFileinfoListWnd::CFileinfoListWnd(CWnd *wnd):CListWnd(wnd)
{
	m_ppropSheet = new CListSheet(_T("CList"));
	m_propSheet = m_ppropSheet;
	m_pe = NULL;
	InitSheet();
}
*/

CFileinfoListWnd::~CFileinfoListWnd()
{
// !!Attention : m_ppropSheet del by baseclass	
	CleanUp();
}

void CFileinfoListWnd::OnDestroy() 
{
	TRACE0("CFileinfoListWnd : OnDestroy \n");
	CleanUp();
}

void CFileinfoListWnd::CleanUp()
{
	TRACE0("CFileinfoListWnd : CleanUp \n");
	if ( m_pPE ) { delete m_pPE; m_pPE = NULL; }
	if(m_propSheet) 
	{
		m_propSheet->DestroyWindow();
		delete m_propSheet;
	}
	m_propSheet=NULL;
}

void CFileinfoListWnd::Save()
{
	if ( m_pPE )
		if( m_pPE->IsCoded())		//Test compressed and decompress
		{
			CString ext = GetExtName(m_pPE->GetBaseName());
			static TCHAR BASED_CODE szFilter[] = _T("EXE Files (*.exe)|*.exe|DLL Files (*.dll)|*.dll|All Files (*.*)|*.*||");
			CFileDialog fd( FALSE, ext, m_pPE->GetBaseName(), OFN_HIDEREADONLY, szFilter, this);
			// Initializes m_ofn structure
			fd.m_ofn.lpstrTitle = _T("Save Unpacked PE file...");
			if ( fd.DoModal() == IDOK)
			{
				m_pPE->Save(fd.GetPathName());
			}
		}
}

BEGIN_MESSAGE_MAP(CFileinfoListWnd, CListWnd)
	//{{AFX_MSG_MAP(CFileinfoListWnd)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CFileinfoListWnd::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message == WM_KEYDOWN ) 
		if ((0x8000 & GetKeyState(VK_CONTROL))) // if CTRL
			if (pMsg->wParam >= 'S') 
			{
				Save();
				// return TRUE;
			}
	return CListWnd::PreTranslateMessage(pMsg);     
}

/////////////////////////////////////////////////////////////////////////////
// CFileinfoListWnd message handlers
