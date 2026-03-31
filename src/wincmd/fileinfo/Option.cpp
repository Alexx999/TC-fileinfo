// COption.cpp : implementation file
//

#include "stdafx.h"
#include "Option.h"
#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COption property page

IMPLEMENT_DYNCREATE(COption, CResizePage)
extern OPTIONS op;

COption::COption() : CResizePage(COption::IDD)
{
   //{{AFX_DATA_INIT(COption)
   //}}AFX_DATA_INIT
}

COption::~COption()
{
   TRACE0("Delete COption \n");
}

BOOL COption::OnKillActive( )
{
	UpdateData(true);
	return CResizePage::OnKillActive();
}

BOOL COption::Update() {
	if (this->m_hWnd) {
		return UpdateData(true);
	} else return false;
}

BOOL COption::UpdateData(BOOL bSaveAndValidate)
{
	BOOL result;
	if (bSaveAndValidate) {
		result = CResizePage::UpdateData(bSaveAndValidate);
	} else {
//		tcmdfnt = !op.buserfont;
		result = CResizePage::UpdateData(bSaveAndValidate);
	}
	return result;
}

void COption::DoDataExchange(CDataExchange* pDX)
{
	CResizePage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(COption)
	DDX_Check(pDX, IDC_autosave, op.autosave);
	DDX_Check(pDX, IDC_Remember, op.rememberAP);
	DDX_Check(pDX, IDC_undec, op.undec);
	DDX_Check(pDX, IDC_res, op.res);
	DDX_Check(pDX, IDC_debug, op.debug);
	DDX_Text(pDX, IDC_MaxD, op.MaxDepth);
	DDX_Check(pDX, IDC_pdata, op.pdata);
	DDX_Check(pDX, IDC_reloc, op.reloc);
	DDX_Text(pDX, IDC_Path, CString(op.p_ini));
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COption, CResizePage)
   //{{AFX_MSG_MAP(COption)
//   ON_BN_CLICKED(IDC_KeepFocus, OnKeepFocus)
//   ON_BN_CLICKED(IDC_Remember, OnRemember)
//   ON_BN_CLICKED(IDC_autosave, Onautosave)
	ON_BN_CLICKED(IDC_Edit, &COption::OnBnClickedEdit)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAbout message handlers

void COption::Resize(CRect &rectPage)
{
   CResizePage::Resize(rectPage);
}

BOOL COption::OnInitDialog()
{
   CResizePage::OnInitDialog();

   // Apply dark mode early, before content is shown
   if (m_bDarkMode)
		SetDarkMode(true);

   UpdateData(FALSE);

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void COption::OnBnClickedEdit()
{
	ShellExecute(0, _T("open"), _T("notepad.exe"), op.p_ini, 0, SW_NORMAL);
}

// Create a copy of a bitmap with its background color replaced.
// Auto-detects the background by sampling the bottom-left corner pixel.
static HBITMAP CreateRemappedBitmap(HBITMAP hSrc, COLORREF crTo)
{
	BITMAP bm;
	if (!::GetObject(hSrc, sizeof(bm), &bm) || bm.bmWidth < 1 || bm.bmHeight == 0)
		return NULL;

	HDC hScreenDC = ::GetDC(NULL);
	HDC hSrcDC = ::CreateCompatibleDC(hScreenDC);
	HDC hDstDC = ::CreateCompatibleDC(hScreenDC);

	BITMAPINFO bi = {};
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = bm.bmWidth;
	bi.bmiHeader.biHeight = bm.bmHeight;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;

	void* pBits = NULL;
	HBITMAP hDst = ::CreateDIBSection(hScreenDC, &bi, DIB_RGB_COLORS, &pBits, NULL, 0);
	if (!hDst || !pBits) {
		::DeleteDC(hSrcDC); ::DeleteDC(hDstDC);
		::ReleaseDC(NULL, hScreenDC);
		return NULL;
	}

	// Copy source bitmap pixels into the 32-bit DIB
	HBITMAP hOldSrc = (HBITMAP)::SelectObject(hSrcDC, hSrc);
	HBITMAP hOldDst = (HBITMAP)::SelectObject(hDstDC, hDst);
	::BitBlt(hDstDC, 0, 0, bm.bmWidth, bm.bmHeight, hSrcDC, 0, 0, SRCCOPY);
	::SelectObject(hSrcDC, hOldSrc);
	::SelectObject(hDstDC, hOldDst);
	::DeleteDC(hSrcDC); ::DeleteDC(hDstDC);
	::ReleaseDC(NULL, hScreenDC);

	// Auto-detect background: sample the first pixel (bottom-left corner of the image)
	DWORD* pixels = (DWORD*)pBits;
	int total = bm.bmWidth * abs(bm.bmHeight);
	DWORD dwFrom = pixels[0] & 0x00FFFFFF;
	DWORD dwTo = GetRValue(crTo) | (GetGValue(crTo) << 8) | (GetBValue(crTo) << 16);

	if (dwFrom == dwTo) {
		// Already the target color, nothing to do
		::DeleteObject(hDst);
		return NULL;
	}

	for (int i = 0; i < total; i++) {
		if ((pixels[i] & 0x00FFFFFF) == dwFrom)
			pixels[i] = (pixels[i] & 0xFF000000) | dwTo;
	}
	return hDst;
}

// Remap a single SS_BITMAP static control's background for dark/light mode.
// Stores/restores original bitmap handle using window properties.
static void RemapStaticBitmapDarkMode(HWND hStatic, bool bDark)
{
	if (!hStatic) return;

	if (bDark) {
		HBITMAP hCurrent = (HBITMAP)::SendMessage(hStatic, STM_GETIMAGE, IMAGE_BITMAP, 0);
		if (!hCurrent) return;

		// Save original if not yet saved
		if (!::GetProp(hStatic, _T("OrigBmp"))) {
			::SetProp(hStatic, _T("OrigBmp"), (HANDLE)hCurrent);
		}

		DarkModeColors dc = GetDarkColors();
		HBITMAP hDark = CreateRemappedBitmap(hCurrent, dc.crBackground);
		if (hDark) {
			::SendMessage(hStatic, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hDark);
			HBITMAP hPrevDark = (HBITMAP)::GetProp(hStatic, _T("DarkBmp"));
			if (hPrevDark) ::DeleteObject(hPrevDark);
			::SetProp(hStatic, _T("DarkBmp"), (HANDLE)hDark);
		}
	} else {
		// Restore original bitmap
		HBITMAP hOrig = (HBITMAP)::GetProp(hStatic, _T("OrigBmp"));
		if (hOrig) {
			::SendMessage(hStatic, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hOrig);
			::RemoveProp(hStatic, _T("OrigBmp"));
		}
		HBITMAP hDark = (HBITMAP)::GetProp(hStatic, _T("DarkBmp"));
		if (hDark) { ::DeleteObject(hDark); ::RemoveProp(hStatic, _T("DarkBmp")); }
	}
}

void COption::SetDarkMode(bool bDark)
{
	CResizePage::SetDarkMode(bDark);

	if (!m_hWnd) return;

	// 1. Combobox needs special dark theme for proper dropdown rendering
	CWnd* pCombo = GetDlgItem(IDC_MaxD);
	if (pCombo && pCombo->m_hWnd) {
		DarkMode_AllowForWindow(pCombo->m_hWnd, bDark);
		SetWindowTheme(pCombo->m_hWnd,
			bDark ? L"DarkMode_CFD" : NULL, NULL);
		pCombo->Invalidate(TRUE);
	}

	// 2. Fix group box and bitmap statics
	CWnd* pChild = GetWindow(GW_CHILD);
	while (pChild) {
		TCHAR cls[64];
		::GetClassName(pChild->m_hWnd, cls, _countof(cls));

		// Group box: remove theming so classic rendering uses our WM_CTLCOLOR brush
		if (_tcsicmp(cls, _T("Button")) == 0) {
			LONG style = ::GetWindowLong(pChild->m_hWnd, GWL_STYLE);
			if ((style & 0x0FL) == BS_GROUPBOX) {
				SetWindowTheme(pChild->m_hWnd, bDark ? L"" : NULL, bDark ? L"" : NULL);
			}
		}

		// Bitmap statics: remap background color for dark/light
		if (_tcsicmp(cls, _T("Static")) == 0) {
			LONG style = ::GetWindowLong(pChild->m_hWnd, GWL_STYLE);
			if ((style & SS_TYPEMASK) == SS_BITMAP) {
				RemapStaticBitmapDarkMode(pChild->m_hWnd, bDark);
			}
		}

		pChild = pChild->GetNextWindow();
	}
}