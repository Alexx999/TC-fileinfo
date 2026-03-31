// ListSheet.cpp : implementation file
//

#include "stdafx.h"
#include "ListSheet.h"
#include "..\common\HyperLink.h"
#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CListSheet

IMPLEMENT_DYNAMIC(CListSheet, CPropertySheetRz)

CListSheet::CListSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
   :CPropertySheetRz(pszCaption, pParentWnd, iSelectPage)
{
	m_bDarkMode = false;
	m_pfnOrigTabProc = NULL;
	AddPage( &m_fi );
	AddPage( &m_fi2 );
//	m_fi2.m_psp.dwFlags |= PSP_USETITLE;
//	m_fi2.m_psp.pszTitle ="Image File Header";
}

BOOL CListSheet::SetPageTitle (int nPage, LPTSTR pszText)
{
    CTabCtrl* pTab = GetTabControl();
    ASSERT (pTab);

    TC_ITEM ti;
    ti.mask = TCIF_TEXT;
    ti.pszText = pszText;
    VERIFY (pTab->SetItem (nPage, &ti));

    return TRUE;
}

CListSheet::~CListSheet()
{
	TRACE0("CListSheet : destructor \n");
	CleanUp();
}

void CListSheet::OnDestroy() 
{
	TRACE0("CListSheet : OnDestroy \n");
}

void CListSheet::Renew(PE_EXE *pPE) 
{
	TRACE0("CListSheet : Renew \n");
	CleanUp();
	m_fi.Renew(pPE);
	m_fi2.Renew(pPE);
}

void CListSheet::CleanUp()
{
	TRACE0("CListSheet : CleanUp \n");
	UINT i=GetPageCount();
	SetActivePage( 0 );
	if (i--)
		for(i; i>1; i--)
		{
//			((CResizePage *)GetPage( i ))->Init();
			RemovePage(i); 
		}
}

#define WM_RESIZEPAGE (WM_USER + 111)
BEGIN_MESSAGE_MAP(CListSheet, CPropertySheetRz)
   //{{AFX_MSG_MAP(CListSheet)
   	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_DRAWITEM()
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CListSheet message handlers

void CListSheet::PreSetDarkMode(bool bDark)
{
	// Set dark mode flags on the sheet and all pages BEFORE HWNDs are created.
	// This ensures OnInitDialog() can detect dark mode and apply it immediately.
	m_bDarkMode = bDark;

	m_brDarkBgAlt.DeleteObject();
	if (bDark) {
		DarkModeColors dc = GetDarkColors();
		m_brDarkBgAlt.CreateSolidBrush(dc.crBackgroundAlt);
	}

	// Set the flag on all pages (their SetDarkMode will create brushes too)
	m_fi.CResizePage::SetDarkMode(bDark);
	m_fi2.CResizePage::SetDarkMode(bDark);
	m_disass.CResizePage::SetDarkMode(bDark);
	m_manifest.CResizePage::SetDarkMode(bDark);
	m_dll.CResizePage::SetDarkMode(bDark);
	m_export.CResizePage::SetDarkMode(bDark);
	m_ocx.CResizePage::SetDarkMode(bDark);
	m_about.CResizePage::SetDarkMode(bDark);
	m_option.CResizePage::SetDarkMode(bDark);
}

void CListSheet::SetDarkMode(bool bDark)
{
	m_bDarkMode = bDark;

	m_brDarkBgAlt.DeleteObject();
	if (bDark) {
		DarkModeColors dc = GetDarkColors();
		m_brDarkBgAlt.CreateSolidBrush(dc.crBackgroundAlt);
	}

	if (m_hWnd) {
		ApplyDarkMode();
	}
}

void CListSheet::ApplyDarkMode()
{
	DarkModeColors colors = m_bDarkMode ? GetDarkColors() : GetLightColors();

	// 0. Strip borders and set dark background on all ancestor windows
	if (m_bDarkMode) {
		ModifyStyle(WS_BORDER, 0, SWP_FRAMECHANGED);
		ModifyStyleEx(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_DLGMODALFRAME, 0, SWP_FRAMECHANGED);
		SetWindowTheme(m_hWnd, L"", L"");
		// Walk up to parent (CFileinfoListWnd) and grandparent (TC Lister)
		CWnd* pParent = GetParent();  // CFileinfoListWnd
		if (pParent) {
			pParent->ModifyStyle(WS_BORDER, 0, SWP_FRAMECHANGED);
			pParent->ModifyStyleEx(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE, 0, SWP_FRAMECHANGED);
			SetWindowTheme(pParent->m_hWnd, L"", L"");
			::SetClassLongPtr(pParent->m_hWnd, GCLP_HBRBACKGROUND,
				(LONG_PTR)::GetStockObject(BLACK_BRUSH));
			pParent->Invalidate(TRUE);
			CWnd* pGrandParent = pParent->GetParent();  // TC Lister window
			if (pGrandParent) {
				pGrandParent->ModifyStyleEx(WS_EX_CLIENTEDGE, 0, SWP_FRAMECHANGED);
			}
		}
	} else {
		SetWindowTheme(m_hWnd, NULL, NULL);
		CWnd* pParent = GetParent();
		if (pParent) {
			pParent->ModifyStyleEx(0, WS_EX_CLIENTEDGE, SWP_FRAMECHANGED);
			SetWindowTheme(pParent->m_hWnd, NULL, NULL);
			::SetClassLongPtr(pParent->m_hWnd, GCLP_HBRBACKGROUND,
				(LONG_PTR)::GetSysColorBrush(COLOR_WINDOW));
			pParent->Invalidate(TRUE);
			CWnd* pGrandParent = pParent->GetParent();
			if (pGrandParent) {
				pGrandParent->ModifyStyleEx(0, WS_EX_CLIENTEDGE, SWP_FRAMECHANGED);
			}
		}
	}

	// 1. Apply to tab control (owner-draw + subclass for dark mode)
	CTabCtrl* pTab = GetTabControl();
	if (pTab) {
		if (m_bDarkMode) {
			pTab->ModifyStyle(0, TCS_OWNERDRAWFIXED);
			// Subclass the tab control to handle its background painting
			if (!m_pfnOrigTabProc) {
				::SetProp(pTab->m_hWnd, _T("DarkSheet"), (HANDLE)this);
				m_pfnOrigTabProc = (WNDPROC)::SetWindowLongPtr(
					pTab->m_hWnd, GWLP_WNDPROC, (LONG_PTR)DarkTabProc);
			}
		} else {
			pTab->ModifyStyle(TCS_OWNERDRAWFIXED, 0);
			// Remove subclass
			if (m_pfnOrigTabProc) {
				::SetWindowLongPtr(pTab->m_hWnd, GWLP_WNDPROC, (LONG_PTR)m_pfnOrigTabProc);
				::RemoveProp(pTab->m_hWnd, _T("DarkSheet"));
				m_pfnOrigTabProc = NULL;
			}
		}
		SetWindowTheme(pTab->m_hWnd,
			m_bDarkMode ? L"DarkMode_Explorer" : NULL, NULL);

		// Theme the tab scroll arrows (updown spin control)
		HWND hSpin = ::FindWindowEx(pTab->m_hWnd, NULL, _T("msctls_updown32"), NULL);
		if (hSpin) {
			SetWindowTheme(hSpin,
				m_bDarkMode ? L"DarkMode_Explorer" : NULL, NULL);
			::InvalidateRect(hSpin, NULL, TRUE);
		}

		pTab->Invalidate(TRUE);
	}

	// 2. Apply to RichEdit-based pages (CListpagePty)
	m_fi.SetDarkMode(m_bDarkMode);
	m_fi2.SetDarkMode(m_bDarkMode);
	m_disass.SetDarkMode(m_bDarkMode);
	m_manifest.SetDarkMode(m_bDarkMode);

	// 3. Apply to tree page
	m_dll.SetDarkMode(m_bDarkMode);

	// 4. Apply to export page
	m_export.SetDarkMode(m_bDarkMode);

	// 5. Apply to OCX page
	m_ocx.SetDarkMode(m_bDarkMode);

	// 6. Apply to About page
	m_about.SetDarkMode(m_bDarkMode);

	// 7. Apply to Option page
	m_option.SetDarkMode(m_bDarkMode);

	// 8. Update HyperLink colors (static, affects all instances)
	CHyperLink::SetColors(colors.crLink, colors.crActiveLink,
		colors.crVisitedLink, colors.crHoverLink);

	// 9. Redraw
	Invalidate(TRUE);
}

BOOL CListSheet::OnEraseBkgnd(CDC* pDC)
{
	if (m_bDarkMode) {
		CRect rect;
		GetClientRect(&rect);
		DarkModeColors dc = GetDarkColors();
		pDC->FillSolidRect(&rect, dc.crBackgroundAlt);
		return TRUE;
	}
	return CPropertySheetRz::OnEraseBkgnd(pDC);
}

void CListSheet::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (!m_bDarkMode) {
		CPropertySheetRz::OnDrawItem(nIDCtl, lpDrawItemStruct);
		return;
	}

	// Owner-draw the tab control items
	CTabCtrl* pTab = GetTabControl();
	if (pTab && lpDrawItemStruct->CtlType == ODT_TAB)
	{
		DarkModeColors dc = GetDarkColors();
		CDC drawDC;
		drawDC.Attach(lpDrawItemStruct->hDC);

		CRect rect(lpDrawItemStruct->rcItem);
		int nTabIndex = lpDrawItemStruct->itemID;
		BOOL bSelected = (nTabIndex == pTab->GetCurSel());

		// Fill background
		COLORREF crBg = bSelected ? dc.crBackground : dc.crBackgroundAlt;
		drawDC.FillSolidRect(&rect, crBg);

		// Draw a subtle top edge on selected tab
		if (bSelected) {
			CRect edgeRect(rect.left, rect.top, rect.right, rect.top + 2);
			drawDC.FillSolidRect(&edgeRect, dc.crLink);
		}

		// Get tab text
		TCHAR szTabText[256] = {0};
		TC_ITEM tci;
		tci.mask = TCIF_TEXT;
		tci.pszText = szTabText;
		tci.cchTextMax = 255;
		pTab->GetItem(nTabIndex, &tci);

		// Draw text
		drawDC.SetBkMode(TRANSPARENT);
		drawDC.SetTextColor(bSelected ? dc.crText : RGB(160, 160, 160));
		rect.DeflateRect(4, 2);
		drawDC.DrawText(szTabText, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		drawDC.Detach();
	}
	else
	{
		CPropertySheetRz::OnDrawItem(nIDCtl, lpDrawItemStruct);
	}
}

LRESULT CALLBACK CListSheet::DarkTabProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CListSheet* pSheet = (CListSheet*)::GetProp(hWnd, _T("DarkSheet"));
	if (!pSheet || !pSheet->m_pfnOrigTabProc)
		return ::DefWindowProc(hWnd, msg, wParam, lParam);

	switch (msg)
	{
	case WM_ERASEBKGND:
		return TRUE;  // We handle all painting in WM_PAINT

	case WM_PAINT:
		{
			// Fully custom paint - bypass default to avoid themed frame borders
			DarkModeColors dc = GetDarkColors();
			PAINTSTRUCT ps;
			HDC hDC = ::BeginPaint(hWnd, &ps);

			// Fill entire client area with dark background
			RECT rc;
			::GetClientRect(hWnd, &rc);
			HBRUSH hBr = ::CreateSolidBrush(dc.crBackgroundAlt);
			::FillRect(hDC, &rc, hBr);
			::DeleteObject(hBr);

			// Draw each tab item via owner-draw (WM_DRAWITEM to parent)
			int nCount = TabCtrl_GetItemCount(hWnd);
			int nSel = TabCtrl_GetCurSel(hWnd);
			HWND hParent = ::GetParent(hWnd);

			// Select the tab font into DC
			HFONT hFont = (HFONT)::SendMessage(hWnd, WM_GETFONT, 0, 0);
			HFONT hOldFont = hFont ? (HFONT)::SelectObject(hDC, hFont) : NULL;

			for (int i = 0; i < nCount; i++) {
				RECT rcItem;
				TabCtrl_GetItemRect(hWnd, i, &rcItem);

				DRAWITEMSTRUCT dis = {0};
				dis.CtlType = ODT_TAB;
				dis.CtlID = ::GetDlgCtrlID(hWnd);
				dis.itemID = i;
				dis.itemAction = ODA_DRAWENTIRE;
				dis.itemState = (i == nSel) ? ODS_SELECTED : 0;
				dis.hwndItem = hWnd;
				dis.hDC = hDC;
				dis.rcItem = rcItem;

				::SendMessage(hParent, WM_DRAWITEM, dis.CtlID, (LPARAM)&dis);
			}

			if (hOldFont) ::SelectObject(hDC, hOldFont);

			// Draw the scroll buttons area (if tabs overflow)
			// This is handled by the spin control child, not us

			::EndPaint(hWnd, &ps);
			return 0;
		}

	case WM_NCDESTROY:
		{
			WNDPROC origProc = pSheet->m_pfnOrigTabProc;
			::RemoveProp(hWnd, _T("DarkSheet"));
			pSheet->m_pfnOrigTabProc = NULL;
			return ::CallWindowProc(origProc, hWnd, msg, wParam, lParam);
		}
	}

	return ::CallWindowProc(pSheet->m_pfnOrigTabProc, hWnd, msg, wParam, lParam);
}