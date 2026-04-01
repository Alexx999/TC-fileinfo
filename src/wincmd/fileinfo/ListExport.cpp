// ListExport.cpp : implementation file
//

#include "stdafx.h"
#include "ListExport.h"
#include <imagehlp.h>
#include <afxole.h> // for clipboard
#include <afxadv.h> // shared file
#include <vector>
#include <map>
#include <algorithm>
#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")

#include "..\common\wait.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define FontSize 13
extern BOOL b_W95Protect;

/////////////////////////////////////////////////////////////////////////////
// CListExport property page

IMPLEMENT_DYNCREATE(CListExport, CResizePage)

CListExport::CListExport() : CResizePage(CListExport::IDD)
{
	//{{AFX_DATA_INIT(CListExport)
	m_undecorate = TRUE;
	m_bsort = FALSE;
	m_nbfunc = 0;
	//}}AFX_DATA_INIT
	m_pe = NULL;
	font = NULL;
	m_NbIF = 0;
	m_activeSel = -1;
	m_bsort = FALSE;
	m_crTestStatus = RGB(0, 0, 0);
}

extern TCHAR inifilename[MAX_PATH];
CListExport::~CListExport()
{
	CleanUp();
}

void CListExport::OnDestroy() 
{
	TRACE0("CListExport : OnDestroy \n");
	CleanUp();
}

void CListExport::Renew(PE_EXE *pPE)
{
	m_pe=pPE;
	m_handleCache.Clear();
	m_funcCache.clear();
	m_activeSel = -1;
	if (m_listmodule.m_hWnd) {
		m_listmodule.ResetContent();
		if (m_pe->IsValid())
			Load();
	}
}

void CListExport::CleanUp()
{
	TRACE0("CListExport : CleanUp \n");
	if (font)	delete font;
	font = NULL;
	if (IsWindow(m_hWnd))   
	{
		UpdateData(TRUE);	
		TCHAR temp[50];
		_itot_s(m_bsort, temp, 10); WritePrivateProfileString( _T("Options"), _T("Sort"), temp, inifilename);
	}
}

void CListExport::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CListExport)
	DDX_Control(pDX, IDC_LIST1, m_listmodule);
	DDX_Control(pDX, IDC_LIST2, m_list);
	DDX_Check(pDX, IDC_undecorate, m_undecorate);
	DDX_Check(pDX, IDC_CHECK1, m_bsort);
	DDX_Text(pDX, IDC_nbfunc, m_nbfunc);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CListExport, CResizePage)
	//{{AFX_MSG_MAP(CListExport)
	ON_LBN_SELCHANGE(IDC_LIST1, OnSelchangeModules)
	ON_BN_CLICKED(IDC_undecorate, Onundecorate)
	ON_LBN_SELCHANGE(IDC_LIST2, OnSelchangeFunc)
	ON_BN_CLICKED(IDC_CHECK1, OnSort)
	ON_WM_DESTROY()
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVirtualListBox — owner-draw no-data listbox implementation.
// DrawItem reads item text from CListExport's cache vector.

void CVirtualListBox::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
	// Default height — will be overridden by SetItemHeight after font is set
	lpMIS->itemHeight = 16;
}

void CVirtualListBox::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	if ((int)lpDIS->itemID < 0 || !m_pOwner)
		return;

	// Get the text from the cache vector
	CString text;
	auto it = m_pOwner->m_funcCache.find(m_pOwner->m_activeSel);
	if (it != m_pOwner->m_funcCache.end() && (int)lpDIS->itemID < (int)it->second.items.size())
		text = it->second.items[lpDIS->itemID];

	CDC dc;
	dc.Attach(lpDIS->hDC);

	// Determine colors based on selection state and dark mode
	COLORREF crBg, crText;
	if (lpDIS->itemState & ODS_SELECTED)
	{
		if (m_pOwner->IsDarkMode()) {
			crBg = RGB(0, 90, 158);
			crText = RGB(255, 255, 255);
		} else {
			crBg = ::GetSysColor(COLOR_HIGHLIGHT);
			crText = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		}
	}
	else
	{
		if (m_pOwner->IsDarkMode()) {
			DarkModeColors dmc = GetDarkColors();
			crBg = dmc.crBackground;
			crText = dmc.crText;
		} else {
			crBg = ::GetSysColor(COLOR_WINDOW);
			crText = ::GetSysColor(COLOR_WINDOWTEXT);
		}
	}

	// Draw background
	dc.FillSolidRect(&lpDIS->rcItem, crBg);
	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(crText);

	// Draw text
	CRect rcText = lpDIS->rcItem;
	rcText.left += 2;
	dc.DrawText(text, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	// Focus rectangle
	if (lpDIS->itemState & ODS_FOCUS)
		dc.DrawFocusRect(&lpDIS->rcItem);

	dc.Detach();
}

// Dark-mode paint proc for normal (string-based) listboxes like the module list.
// Reads text via LB_GETTEXT (not from cache).
static LRESULT CALLBACK DarkListBoxProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC pfnOrig = (WNDPROC)::GetProp(hWnd, _T("DarkLBOrig"));
	if (!pfnOrig)
		return ::DefWindowProc(hWnd, msg, wParam, lParam);

	switch (msg)
	{
	case WM_ERASEBKGND:
		return TRUE;
	case WM_PRINTCLIENT:
		return 0;
	case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
	case WM_KEYDOWN: case WM_KEYUP: case WM_CHAR:
	case WM_MOUSEWHEEL: case WM_VSCROLL: case WM_HSCROLL:
		{
			::SendMessage(hWnd, WM_SETREDRAW, FALSE, 0);
			LRESULT r = ::CallWindowProc(pfnOrig, hWnd, msg, wParam, lParam);
			::SendMessage(hWnd, WM_SETREDRAW, TRUE, 0);
			::RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE | RDW_FRAME);
			return r;
		}
	case WM_MOUSEMOVE:
		if (::GetCapture() == hWnd) {
			::SendMessage(hWnd, WM_SETREDRAW, FALSE, 0);
			LRESULT r = ::CallWindowProc(pfnOrig, hWnd, msg, wParam, lParam);
			::SendMessage(hWnd, WM_SETREDRAW, TRUE, 0);
			::RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE | RDW_FRAME);
			return r;
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC = ::BeginPaint(hWnd, &ps);
			DarkModeColors dc = GetDarkColors();
			RECT rcClient; ::GetClientRect(hWnd, &rcClient);
			int w = rcClient.right, h = rcClient.bottom;
			HDC hMemDC = ::CreateCompatibleDC(hDC);
			HBITMAP hBmp = ::CreateCompatibleBitmap(hDC, w, h);
			HBITMAP hOldBmp = (HBITMAP)::SelectObject(hMemDC, hBmp);
			HBRUSH hBr = ::CreateSolidBrush(dc.crBackground);
			::FillRect(hMemDC, &rcClient, hBr); ::DeleteObject(hBr);
			int topIndex = (int)::SendMessage(hWnd, LB_GETTOPINDEX, 0, 0);
			int count = (int)::SendMessage(hWnd, LB_GETCOUNT, 0, 0);
			int itemH = (int)::SendMessage(hWnd, LB_GETITEMHEIGHT, 0, 0);
			if (itemH <= 0) itemH = 16;
			HFONT hFont = (HFONT)::SendMessage(hWnd, WM_GETFONT, 0, 0);
			HFONT hOldFont = hFont ? (HFONT)::SelectObject(hMemDC, hFont) : NULL;
			::SetBkMode(hMemDC, TRANSPARENT);
			int y = 0;
			for (int i = topIndex; i < count && y < h; i++) {
				RECT rcItem = { 0, y, rcClient.right, y + itemH };
				BOOL bSel = (BOOL)::SendMessage(hWnd, LB_GETSEL, i, 0);
				HBRUSH hIBr = ::CreateSolidBrush(bSel ? RGB(0,90,158) : dc.crBackground);
				::FillRect(hMemDC, &rcItem, hIBr); ::DeleteObject(hIBr);
				int len = (int)::SendMessage(hWnd, LB_GETTEXTLEN, i, 0);
				if (len > 0 && len != LB_ERR) {
					TCHAR* text = new TCHAR[len + 1];
					::SendMessage(hWnd, LB_GETTEXT, i, (LPARAM)text);
					::SetTextColor(hMemDC, bSel ? RGB(255,255,255) : dc.crText);
					RECT rcT = rcItem; rcT.left += 2;
					::DrawText(hMemDC, text, -1, &rcT, DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
					delete[] text;
				}
				y += itemH;
			}
			if (hOldFont) ::SelectObject(hMemDC, hOldFont);
			::BitBlt(hDC, 0, 0, w, h, hMemDC, 0, 0, SRCCOPY);
			::SelectObject(hMemDC, hOldBmp); ::DeleteObject(hBmp); ::DeleteDC(hMemDC);
			::EndPaint(hWnd, &ps);
			return 0;
		}
	case WM_NCDESTROY:
		{
			WNDPROC orig = pfnOrig;
			::RemoveProp(hWnd, _T("DarkLBOrig"));
			return ::CallWindowProc(orig, hWnd, msg, wParam, lParam);
		}
	}
	return ::CallWindowProc(pfnOrig, hWnd, msg, wParam, lParam);
}

static void SubclassListBoxForDark(CListBox& lb, bool bDark)
{
	if (!lb.m_hWnd) return;
	WNDPROC pfnOrig = (WNDPROC)::GetProp(lb.m_hWnd, _T("DarkLBOrig"));
	if (bDark && !pfnOrig) {
		pfnOrig = (WNDPROC)::SetWindowLongPtr(
			lb.m_hWnd, GWLP_WNDPROC, (LONG_PTR)DarkListBoxProc);
		::SetProp(lb.m_hWnd, _T("DarkLBOrig"), (HANDLE)pfnOrig);
	} else if (!bDark && pfnOrig) {
		::SetWindowLongPtr(lb.m_hWnd, GWLP_WNDPROC, (LONG_PTR)pfnOrig);
		::RemoveProp(lb.m_hWnd, _T("DarkLBOrig"));
	}
	if (bDark) {
		lb.ModifyStyleEx(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE, 0, SWP_FRAMECHANGED);
		lb.ModifyStyle(WS_BORDER, 0, SWP_FRAMECHANGED);
	} else {
		lb.ModifyStyleEx(0, WS_EX_CLIENTEDGE, SWP_FRAMECHANGED);
	}
	lb.Invalidate(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CListExport message handlers
void CListExport::Resize(CRect &rectPage)
{
   CResizePage::Resize(rectPage);

   m_list.GetWindowRect(&m_rectList);
   ScreenToClient(&m_rectList);
   m_rectList.right -= m_sizeRelChange.cx; 
   m_rectList.bottom -= m_sizeRelChange.cy;
   m_list.MoveWindow(&m_rectList);

   m_listmodule.GetWindowRect(&m_rectModule);
   ScreenToClient(&m_rectModule);
   m_rectModule.right -= m_sizeRelChange.cx;   
   m_listmodule.MoveWindow(&m_rectModule);
}

void CListExport::Load()
{
		CString str;
		CWait wait(this);
		wait.SetStatus(_T("Listing Modules..."));

		m_pe->IsCoded();		//Test compressed and decompress
		m_listmodule.SetRedraw(FALSE);
		str.Format(_T("%s  ( Exported functions )"), m_pe->GetBaseName() );
		m_listmodule.AddString( str );
		MODULE_DEPENDENCY_LIST *pDep = m_pe->GetDepends();
		if (pDep)
		{
			PMODULE_FILE_INFO pModInfo = pDep->GetNextModule( (PMODULE_FILE_INFO) 0 );
			if (pModInfo )
				while ( pModInfo = pDep->GetNextModule( pModInfo ) )
				{
					m_listmodule.AddString( pModInfo->GetBaseName() );
					m_NbIF++;
				}
			pModInfo = pDep->GetNextDelayedModule( (PMODULE_FILE_INFO) 0 );
			if (pModInfo )
				while ( pModInfo = pDep->GetNextDelayedModule( pModInfo ) )
				{
					str.Format(_T("%s  ( Delayed Import )"), pModInfo->GetBaseName() );
					m_listmodule.AddString( str );
				}
		}
		m_listmodule.SetRedraw(TRUE);
		m_listmodule.Invalidate();
		wait.SetStatus(_T("Listing Functions..."));
		AddFunction(0);
		m_listmodule.SetCurSel( 0 );
}

BOOL CListExport::OnInitDialog()
{
	CResizePage::OnInitDialog();

	// Create the font first — needed for MeasureItem during listbox recreation
	font = new(CFont);
	font->CreateFont( -FontSize, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, _T("Consolas") );

	// Recreate the function listbox as virtual (owner-data).
	// LBS_NODATA + LBS_OWNERDRAWFIXED must be present at creation time.
	// CVirtualListBox::DrawItem() handles all painting via MFC message reflection.
	{
		CRect rc;
		m_list.GetWindowRect(&rc);
		ScreenToClient(&rc);
		DWORD dwExStyle = m_list.GetExStyle();
		HWND hOld = m_list.UnsubclassWindow();
		::DestroyWindow(hOld);
		m_list.m_pOwner = this;
		m_list.CreateEx(dwExStyle, _T("LISTBOX"), NULL,
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | WS_TABSTOP |
			LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED | LBS_NODATA | LBS_NOTIFY,
			rc, this, IDC_LIST2);
		m_list.SetFont(font);
		// Set item height explicitly for scrollbar calculation
		CDC* pDC = m_list.GetDC();
		if (pDC) {
			CFont* pOld = pDC->SelectObject(font);
			TEXTMETRIC tm;
			pDC->GetTextMetrics(&tm);
			m_list.SetItemHeight(0, tm.tmHeight + tm.tmExternalLeading + 1);
			pDC->SelectObject(pOld);
			m_list.ReleaseDC(pDC);
		}
	}

	m_listmodule.SetFont(font);

	// Apply dark mode early, before content is loaded
	if (m_bDarkMode)
		SetDarkMode(true);

	if (m_pe->IsValid())
	{
		Load();
	}
	m_list.SetHorizontalExtent( m_Hsize * 10 );

    return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CListExport::OnToggleListStyle()
{
	m_funcCache.clear();
}

#define SIZEBUFFER 1024
extern PSTR Undecorate(PSTR Textin, PSTR Textout, int len);

// Restore a cached function list — virtual listbox, just set the count
void CListExport::RestoreFromCache(int sel)
{
	m_activeSel = sel;
	const FuncListCache& c = m_funcCache[sel];
	::SendMessage(m_list.m_hWnd, LB_SETCOUNT, (WPARAM)c.items.size(), 0);
	m_list.Invalidate();
	m_Hsize = c.hsize;
	m_list.SetHorizontalExtent( m_Hsize * 10 );
	m_nbfunc = c.nbfunc;
	m_crTestStatus = c.statusColor;
	UpdateData(FALSE);
	CWnd* pStatus = GetDlgItem(IDC_TESTSTATUS);
	if (pStatus) {
		pStatus->SetWindowText(c.statusText);
		pStatus->Invalidate();
	}
}

void CListExport::AddFunction(int sel)
{
	// Check cache first
	if (m_funcCache.count(sel)) {
		RestoreFromCache(sel);
		return;
	}

	// Build results into cache vector (no listbox interaction yet)
	FuncListCache cache;
	CString strTemp=_T("");
	int i;
	int hsize = 0;
	int missingCount = 0;
	HINSTANCE hTestDll = NULL;
	PMODULE_FILE_INFO pModInfo = NULL;
	DWORD nbfunc = 0;
	if (sel)
	{
// Import Section
		MODULE_DEPENDENCY_LIST *pDep = m_pe->GetDepends();
		m_listmodule.GetText( sel, strTemp );
		if (pDep)
		{
			if ( sel <= m_NbIF )
			{
				pModInfo = pDep->GetNextModule( (PMODULE_FILE_INFO) 0 );
				if (!pModInfo) return;
				while(strTemp != pModInfo->GetBaseName())
				{
					pModInfo = pDep->GetNextModule( pModInfo );
					if (!pModInfo) return;
				}
			}
			else
			{
				pModInfo = pDep->GetNextDelayedModule( (PMODULE_FILE_INFO) 0 );
				if (!pModInfo) return;
				int nb = strTemp.GetLength();
				while(_tcsncmp(strTemp, pModInfo->GetBaseName(), nb-20))
				{
					pModInfo = pDep->GetNextDelayedModule( pModInfo );
					if (!pModInfo) return;
				}
			}

			// Load the DLL for testing imported functions
			if (pModInfo->IsModuleFound())
				hTestDll = m_handleCache.GetHandle(pModInfo->GetFullName(), b_W95Protect);

			// Build ordinal→name map from the target DLL's export table
			std::map<int, CString> ordinalNames;
			if (pModInfo->IsModuleFound())
			{
				PE_EXE peTarget(pModInfo->GetFullName());
				if (peTarget.IsValid())
				{
					PIMAGE_EXPORT_DIRECTORY expDir = peTarget.GetExportsDesc();
					if (expDir && peTarget.IsValidPtr((ULONG_PTR)expDir))
					{
						PDWORD expFunctions = (PDWORD)peTarget.GetReadablePointerFromRVA(expDir->AddressOfFunctions);
						PWORD  expOrdinals  = (PWORD)peTarget.GetReadablePointerFromRVA(expDir->AddressOfNameOrdinals);
						PDWORD expNames     = (PDWORD)peTarget.GetReadablePointerFromRVA(expDir->AddressOfNames);
						if (expFunctions && expOrdinals && expNames)
						{
							for (int j = 0; j < (int)expDir->NumberOfNames; j++)
							{
								int ord = expOrdinals[j] + expDir->Base;
								PSTR eName = (PSTR)peTarget.GetReadablePointerFromRVA(expNames[j]);
								if (eName)
									ordinalNames[ord] = CString(eName);
							}
						}
					}
				}
			}

			const auto& flist = pModInfo->GetFList();
			if (flist.empty()) return;
			cache.items.reserve(flist.size());
			BOOL bMissing;
			for (const auto& func : flist)
			{
				bMissing = FALSE;

				// Test this function against the loaded DLL
				if (hTestDll)
				{
					if (_tcsncmp(_T("ordinal "), func, 8)==0)
					{
						if (!GetProcAddress( hTestDll, MAKEINTRESOURCEA(_ttoi((LPCTSTR) func + 8))))
							bMissing = TRUE;
					}
					else if (_tcsncmp(func, _T("<invalid name>"), 14) != 0)
					{
						if (!GetProcAddress( hTestDll, CT2A(func) ))
							bMissing = TRUE;
					}
				}

				// Format display name
				if (_tcsncmp(_T("ordinal "), func, 8)==0)
				{
					int ord = _ttoi((LPCTSTR) func + 8);
					auto it = ordinalNames.find(ord);
					if (it != ordinalNames.end())
						strTemp.Format(_T("ordinal %d  (%s)"), ord, (LPCTSTR)it->second);
					else
						strTemp = func;
				}
				else
				{
					if ( m_undecorate ) // Undecorate Name
					{
			 			char Textout[SIZEBUFFER];
						strTemp = CA2T(Undecorate(CT2A(func), Textout, SIZEBUFFER));
					}
					else strTemp.Format(_T("%s"), (LPCTSTR)func);
				}

				// Prepend "(Missing) " if function was not found
				if (bMissing)
				{
					missingCount++;
					strTemp = _T("(Missing) ") + strTemp;
				}

				if (hsize < strTemp.GetLength()) hsize = strTemp.GetLength();
				cache.items.push_back(strTemp);
				nbfunc++;
			}
		}
	}
	else
	{
// Export Section
		PIMAGE_EXPORT_DIRECTORY exportDir = m_pe->GetExportsDesc();
		if ( exportDir  && m_pe->IsValidPtr(( ULONG_PTR) exportDir ))
		{
			PSTR	filename = (PSTR)m_pe->GetReadablePointerFromRVA( exportDir->Name );
			if (!(((ULONG_PTR) filename > (ULONG_PTR) exportDir + m_pe->GetFileSize()) || ((ULONG_PTR) filename < (ULONG_PTR) exportDir)))
			{
				PDWORD functions = ( PDWORD ) m_pe->GetReadablePointerFromRVA( exportDir->AddressOfFunctions );
				PWORD ordinals = (PWORD) m_pe->GetReadablePointerFromRVA( exportDir->AddressOfNameOrdinals );
				PDWORD name = ( PDWORD ) m_pe->GetReadablePointerFromRVA( exportDir->AddressOfNames);
				// Compute export directory bounds for forwarder detection
				DWORD exportsStartRVA = m_pe->GetDataDirectoryEntryRVA(IMAGE_DIRECTORY_ENTRY_EXPORT);
				DWORD exportsEndRVA = exportsStartRVA + m_pe->GetDataDirectoryEntrySize(IMAGE_DIRECTORY_ENTRY_EXPORT);

				// Single pass over names: resolve display names and compute max length
				struct ExportName { CString displayName; int funcIndex; };
				std::vector<ExportName> namedExports;
				namedExports.reserve(exportDir->NumberOfNames);
				int maxExportLen = 0;

				for(i=0; i < (int) exportDir->NumberOfNames; i++)
				{
					PSTR Name = (PSTR) m_pe->GetReadablePointerFromRVA( name[i] );
					if (!Name) continue;
					CString dn;
					if ( m_undecorate )
					{
						char Textout[SIZEBUFFER];
						dn = CA2T(Undecorate(Name, Textout, SIZEBUFFER));
					}
					else dn = CString(Name);
					if (dn.GetLength() > maxExportLen) maxExportLen = dn.GetLength();
					ExportName en = { dn, (int)ordinals[i] };
					namedExports.push_back(en);
				}
				int padTo = maxExportLen + 2;

				// Build reverse map: function index → namedExports index (O(n) instead of O(n*m))
				std::vector<int> nameForFunc(exportDir->NumberOfFunctions, -1);
				for (int j = 0; j < (int)namedExports.size(); j++)
					nameForFunc[namedExports[j].funcIndex] = j;

				// Build items into cache vector
				cache.items.reserve(exportDir->NumberOfFunctions);
				for(i=0; i < (int) exportDir->NumberOfFunctions; i++)
				{
					DWORD entryPointRVA = functions[i];
					if ( entryPointRVA == 0 ) continue; // Skip over gaps in exported function
					// Check if this export is a forwarder
					BOOL isForwarder = (entryPointRVA >= exportsStartRVA) && (entryPointRVA <= exportsEndRVA);
					CString strForwardTarget;
					if (isForwarder)
					{
						PSTR pszForward = (PSTR)m_pe->GetReadablePointerFromRVA(entryPointRVA);
						if (pszForward)
							strForwardTarget = CString(pszForward);
					}
					int nameIdx = nameForFunc[i];
					if (nameIdx >= 0)
					{
						CString displayName = namedExports[nameIdx].displayName;
						if (!strForwardTarget.IsEmpty())
						{
							int pad = padTo - displayName.GetLength();
							if (pad < 2) pad = 2;
							CString entry;
							entry.Format(_T("%s%*s-> %s"), (LPCTSTR)displayName, pad, _T(""), (LPCTSTR)strForwardTarget);
							displayName = entry;
						}
						int size = displayName.GetLength();
						if (hsize < size) hsize = size;
						cache.items.push_back(displayName);
						nbfunc++;
					}
					else
					{
						CString displayName;
						displayName.Format( _T("ordinal %d"), i + exportDir->Base );
						if (!strForwardTarget.IsEmpty())
						{
							int pad = padTo - displayName.GetLength();
							if (pad < 2) pad = 2;
							CString entry;
							entry.Format(_T("%s%*s-> %s"), (LPCTSTR)displayName, pad, _T(""), (LPCTSTR)strForwardTarget);
							displayName = entry;
						}
						cache.items.push_back(displayName);
						nbfunc++;
					}
				}
			}
		}
	}

	// Compute status text and color
	cache.hsize = hsize;
	cache.nbfunc = nbfunc;
	if (sel == 0)
	{
		cache.statusText = _T("");
		cache.statusColor = RGB(0, 0, 0);
	}
	else if (!pModInfo || !pModInfo->IsModuleFound())
	{
		cache.statusColor = m_bDarkMode ? RGB(255, 80, 80) : RGB(200, 0, 0);
		cache.statusText = _T("DLL not found");
	}
	else if (hTestDll == NULL)
	{
		cache.statusColor = m_bDarkMode ? RGB(255, 80, 80) : RGB(200, 0, 0);
		cache.statusText = _T("DLL load failed");
	}
	else if (missingCount == 0)
	{
		cache.statusColor = m_bDarkMode ? RGB(80, 200, 80) : RGB(0, 140, 0);
		cache.statusText = _T("All OK");
	}
	else
	{
		cache.statusColor = m_bDarkMode ? RGB(255, 80, 80) : RGB(200, 0, 0);
		cache.statusText.Format(_T("%d Missing"), missingCount);
	}
	// Sort the items if sort mode is active
	if (m_bsort)
		std::sort(cache.items.begin(), cache.items.end(),
			[](const CString& a, const CString& b) { return a.CompareNoCase(b) < 0; });

	m_funcCache[sel] = cache;

	// Populate listbox from cache using the optimized path
	RestoreFromCache(sel);
}

void CListExport::OnSelchangeModules()
{
	int sel = m_listmodule.GetCurSel( );
	if (sel == -1) return;
	// Skip if cached (instant restore, no wait dialog needed)
	if (m_funcCache.count(sel)) {
		RestoreFromCache(sel);
		m_list.SetHorizontalExtent( m_Hsize * 10 );
		m_listmodule.SetFocus();
		return;
	}
	CWait wait(this);
	wait.SetStatus(_T("Listing Functions..."));
	AddFunction(sel);
	m_list.SetHorizontalExtent( m_Hsize *10 );
	m_listmodule.SetFocus();
}

void CListExport::Onundecorate()
{
	// UpdateData(TRUE);
	m_undecorate = !m_undecorate;
	m_funcCache.clear();
	int sel = m_list.GetCurSel( );
	AddFunction(m_listmodule.GetCurSel( ));
	m_list.SetHorizontalExtent( m_Hsize * 10 );
	m_list.SetCurSel( sel );
	UpdateData(TRUE);
}

HBRUSH CListExport::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CResizePage::OnCtlColor(pDC, pWnd, nCtlColor);
	if (pWnd->GetDlgCtrlID() == IDC_TESTSTATUS)
		pDC->SetTextColor(m_crTestStatus);
	return hbr;
}

void CListExport::OnSelchangeFunc()
{
	int sel = m_list.GetCurSel( );
	if (sel == -1) return;
	COleDataSource* pData = new COleDataSource;

	CSharedFile sf(GMEM_MOVEABLE|GMEM_DDESHARE|GMEM_ZEROINIT);
	CString	func;
	// Read from cache vector (LBS_NODATA listbox has no string storage)
	auto it = m_funcCache.find(m_activeSel);
	if (it != m_funcCache.end() && sel < (int)it->second.items.size())
		func = it->second.items[sel];
	else
		return;
	sf.Write(func, func.GetLength()); // you can write to the clipboard as you would to any cfile

	HGLOBAL  hmem = sf.Detach();
	if (!hmem) return;
	
	pData->CacheGlobalData( CF_TEXT, hmem );
	pData->SetClipboard();

}

void CListExport::OnSort() 
{
	// UpdateData(TRUE);
	m_bsort = !m_bsort;
	OnToggleListStyle();
	int sel = m_listmodule.GetCurSel( );
	if (sel == -1) return;
	AddFunction(sel);
	m_list.SetHorizontalExtent( m_Hsize * 10 );
	UpdateData(TRUE);
}

BOOL CListExport::PreTranslateMessage(MSG* pMsg) 
{
   if (pMsg->message == WM_KEYDOWN ) 
   {
		if (pMsg->wParam == 85)
		{
			Onundecorate();
			return 0;
		}
		if (pMsg->wParam == 83)
		{
			OnSort();
			return 0;
		}
		if (pMsg->wParam == VK_F3)
		{
			int sel = m_listmodule.GetCurSel( );
			if (sel)
			{
				MODULE_DEPENDENCY_LIST *pDep = m_pe->GetDepends();
				if (pDep)
				{
					PMODULE_FILE_INFO pModInfo;
					CString Module; int decal = 0; //BOOL bdelay=FALSE;

					m_listmodule.GetText( sel, Module );
					if ( sel <= m_NbIF ) // import
						pModInfo = pDep->GetNextModule( (PMODULE_FILE_INFO) 0 );
					else 
					{
						decal = 20;
						pModInfo = pDep->GetNextDelayedModule( (PMODULE_FILE_INFO) 0 );
					}

					if (!pModInfo) return 0;
					while(_tcsncmp(Module, pModInfo->GetBaseName(), Module.GetLength()-decal))
					{
						if (decal) pModInfo = pDep->GetNextDelayedModule( pModInfo );
						else pModInfo = pDep->GetNextModule( pModInfo );
						if (!pModInfo) return 0;
					}
					if (pModInfo->IsModuleFound())
					{
						SHELLEXECUTEINFO sei  = { sizeof(sei ) };
						sei.fMask = SEE_MASK_DOENVSUBST;
						sei.nShow = SW_SHOWNORMAL;
				//		sei.lpVerb = argv[1];
						sei.lpFile = _T("%commander_exe%");
						CString com = _T("/S=L \" ") + (CString) pModInfo->GetFullName() + _T("\"");
						sei.lpParameters = com;
						if(ShellExecuteEx(&sei) == FALSE)
						{
							CString mess;
							mess.Format(_T("ShellExecuteEx Error \narg : %s"),(LPCTSTR)com);
							AfxMessageBox(mess,MB_ICONEXCLAMATION);
						}
					}
				}
			}
			return 0; 
		}
	}
	return CResizePage::PreTranslateMessage(pMsg);
}

void CListExport::SetDarkMode(bool bDark)
{
	CResizePage::SetDarkMode(bDark);
	SubclassListBoxForDark(m_listmodule, bDark);
	// m_list (CVirtualListBox) paints itself via DrawItem — just invalidate
	if (m_list.m_hWnd) {
		if (bDark) {
			DarkMode_AllowForWindow(m_list.m_hWnd, TRUE);
			SetWindowTheme(m_list.m_hWnd, L"DarkMode_Explorer", NULL);
		} else {
			DarkMode_AllowForWindow(m_list.m_hWnd, FALSE);
			SetWindowTheme(m_list.m_hWnd, NULL, NULL);
		}
		m_list.Invalidate();
	}
}

