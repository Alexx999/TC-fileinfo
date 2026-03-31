// ResizePage.cpp : implementation file
//

#include "stdafx.h"
#include "ResizePage.h"
#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CResizePage property page

IMPLEMENT_DYNCREATE(CResizePage, CPropertyPage)

CResizePage::CResizePage(UINT nIDTemplate): CPropertyPage(nIDTemplate)
{
    m_first = TRUE;
    m_editmode = FALSE;
	m_forceredraw = FALSE;
	m_bDarkMode = false;
   //{{AFX_DATA_INIT(CResizePage)
      // NOTE: the ClassWizard will add member initialization here
   //}}AFX_DATA_INIT
//   Construct(nIDTemplate);
}

CResizePage::~CResizePage()
{
//   TRACE0("Delete ResizePage \n");
//	CleanUp();
}

void CResizePage::OnDestroy( )
{
//	TRACE0("CResizePage : OnDestroy \n");
//	CleanUp();
	m_first = TRUE;
}

void CResizePage::CleanUp()
{
	TRACE0("CResizePage : CleanUp \n");
}

void CResizePage::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CResizePage)
      // NOTE: the ClassWizard will add DDX and DDV calls here
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CResizePage, CPropertyPage)
   //{{AFX_MSG_MAP(CResizePage)
   ON_WM_SHOWWINDOW()
	ON_WM_DESTROY()
	ON_WM_CTLCOLOR()
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResizePage message handlers


BOOL CResizePage::OnInitDialog() 
{
   CPropertyPage::OnInitDialog();
   GetWindowRect(&m_oldRect);
   GetParent()->ScreenToClient(&m_oldRect);
   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CResizePage::Resize(CRect &rectPage)
{ 
   m_sizeRelChange.cx = m_oldRect.right - rectPage.right;
   m_sizeRelChange.cy = m_oldRect.bottom - rectPage.bottom;
   MoveWindow(&rectPage);
   m_oldRect = rectPage; 
}


void CResizePage::OnShowWindow(BOOL bShow, UINT nStatus)
{
   CPropertyPage::OnShowWindow(bShow, nStatus);

   if (m_first)
   {
      CRect TempRect;
      GetWindowRect(&TempRect);
      GetParent()->ScreenToClient(&TempRect);
      m_oldRect.top += TempRect.top;
      m_oldRect.left += TempRect.left;
      m_oldRect.bottom += TempRect.top;
      m_oldRect.right += TempRect.left;
      m_first=FALSE;
   }
}

void CResizePage::SetDarkMode(bool bDark)
{
	m_bDarkMode = bDark;
	m_brDarkBg.DeleteObject();
	if (bDark) {
		DarkModeColors dc = GetDarkColors();
		m_brDarkBg.CreateSolidBrush(dc.crBackground);
	}
	if (m_hWnd) {
		// Apply dark/light theme to all child controls (buttons, checkboxes, statics, etc.)
		CWnd* pChild = GetWindow(GW_CHILD);
		while (pChild) {
			DarkMode_AllowForWindow(pChild->m_hWnd, bDark);
			SetWindowTheme(pChild->m_hWnd,
				bDark ? L"DarkMode_Explorer" : NULL, NULL);
			pChild = pChild->GetNextWindow();
		}
		Invalidate(TRUE);
	}
}

HBRUSH CResizePage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (m_bDarkMode) {
		DarkModeColors dc = GetDarkColors();
		pDC->SetTextColor(dc.crText);
		pDC->SetBkColor(dc.crBackground);
		return (HBRUSH)m_brDarkBg.GetSafeHandle();
	}
	return CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);
}

/////////////////////////////////////////////////////////////////////////////
// Shared dark mode helpers for RichEdit controls

void ApplyDarkTextFormat(CRichEditCtrl &redit, bool bDark)
{
	if (!redit.m_hWnd) return;
	CHARFORMAT cf;
	ZeroMemory(&cf, sizeof(cf));
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR;
	if (bDark) {
		DarkModeColors dc = GetDarkColors();
		cf.crTextColor = dc.crText;
		cf.dwEffects &= ~CFE_AUTOCOLOR;
	} else {
		cf.dwEffects = CFE_AUTOCOLOR;
	}
	redit.SetSel(0, -1);
	redit.SetSelectionCharFormat(cf);
	redit.SetDefaultCharFormat(cf);
	redit.SetSel(0, 0);
}

void ApplyDarkRichEdit(CRichEditCtrl &redit, bool bDark)
{
	if (!redit.m_hWnd) return;
	if (bDark) {
		DarkModeColors dc = GetDarkColors();
		redit.SetBackgroundColor(FALSE, dc.crBackground);
		redit.ModifyStyle(WS_BORDER, 0, SWP_FRAMECHANGED);
		redit.ModifyStyleEx(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE, 0, SWP_FRAMECHANGED);
	} else {
		redit.SetBackgroundColor(TRUE, 0);
		redit.ModifyStyle(0, WS_BORDER, SWP_FRAMECHANGED);
	}
	ApplyDarkTextFormat(redit, bDark);
	SetWindowTheme(redit.m_hWnd, bDark ? L"DarkMode_Explorer" : NULL, NULL);
}