// Wait.cpp : implementation file
//

#include "stdafx.h"
#include "Wait.h"
#include "DarkMode.h"
#include "ResizePage.h"
#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWait dialog


CWait::CWait(CWnd* pParent /*=NULL*/)
	: CDialog(CWait::IDD, pParent)
{
	//{{AFX_DATA_INIT(CWait)
	m_str = _T("");
	//}}AFX_DATA_INIT
	m_bDarkMode = false;
	// Check if parent is a dark-mode CResizePage
	if (pParent && pParent->IsKindOf(RUNTIME_CLASS(CResizePage))) {
		m_bDarkMode = ((CResizePage*)pParent)->IsDarkMode();
	}
	if (m_bDarkMode) {
		DarkModeColors dc = GetDarkColors();
		m_brDarkBg.CreateSolidBrush(dc.crBackground);
	}
	Create(IDD, pParent);
}


void CWait::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWait)
	DDX_Text(pDX, IDC_status, m_str);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWait, CDialog)
	//{{AFX_MSG_MAP(CWait)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWait message handlers

BOOL CWait::OnInitDialog()
{
	CDialog::OnInitDialog();
	if (m_bDarkMode) {
		ModifyStyle(WS_BORDER | DS_MODALFRAME, 0, SWP_FRAMECHANGED);
		ModifyStyleEx(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_DLGMODALFRAME, 0, SWP_FRAMECHANGED);
		SetWindowTheme(m_hWnd, L"", L"");
	}
	return TRUE;
}

HBRUSH CWait::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (m_bDarkMode) {
		DarkModeColors dc = GetDarkColors();
		pDC->SetTextColor(dc.crText);
		pDC->SetBkColor(dc.crBackground);
		return (HBRUSH)m_brDarkBg.GetSafeHandle();
	}
	return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}
