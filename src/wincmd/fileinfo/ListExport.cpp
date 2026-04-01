// ListExport.cpp : implementation file
//

#include "stdafx.h"
#include "ListExport.h"
#include <imagehlp.h>
#include <afxole.h> // for clipboard
#include <afxadv.h> // shared file
#include <vector>
#include <map>
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
	m_lastSel = -1;
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
// Dark mode listbox subclass — custom WM_PAINT with double-buffering,
// all input/scroll messages wrapped in WM_SETREDRAW to suppress internal drawing

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
		return 0;  // suppress animation/scroll paint-through

	// Suppress the listbox's internal direct-DC drawing for ALL messages
	// that can change selection or scroll position.
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_CHAR:
	case WM_MOUSEWHEEL:
	case WM_VSCROLL:
	case WM_HSCROLL:
		{
			::SendMessage(hWnd, WM_SETREDRAW, FALSE, 0);
			LRESULT result = ::CallWindowProc(pfnOrig, hWnd, msg, wParam, lParam);
			::SendMessage(hWnd, WM_SETREDRAW, TRUE, 0);
			::RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE | RDW_FRAME);
			return result;
		}
	case WM_MOUSEMOVE:
		if (::GetCapture() == hWnd) {
			::SendMessage(hWnd, WM_SETREDRAW, FALSE, 0);
			LRESULT result = ::CallWindowProc(pfnOrig, hWnd, msg, wParam, lParam);
			::SendMessage(hWnd, WM_SETREDRAW, TRUE, 0);
			::RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE | RDW_FRAME);
			return result;
		}
		break;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC = ::BeginPaint(hWnd, &ps);
			DarkModeColors dc = GetDarkColors();

			RECT rcClient;
			::GetClientRect(hWnd, &rcClient);
			int w = rcClient.right - rcClient.left;
			int h = rcClient.bottom - rcClient.top;

			// Double-buffer to offscreen bitmap
			HDC hMemDC = ::CreateCompatibleDC(hDC);
			HBITMAP hBmp = ::CreateCompatibleBitmap(hDC, w, h);
			HBITMAP hOldBmp = (HBITMAP)::SelectObject(hMemDC, hBmp);

			HBRUSH hBr = ::CreateSolidBrush(dc.crBackground);
			::FillRect(hMemDC, &rcClient, hBr);
			::DeleteObject(hBr);

			int topIndex = (int)::SendMessage(hWnd, LB_GETTOPINDEX, 0, 0);
			int count = (int)::SendMessage(hWnd, LB_GETCOUNT, 0, 0);
			int itemHeight = (int)::SendMessage(hWnd, LB_GETITEMHEIGHT, 0, 0);
			if (itemHeight <= 0) itemHeight = 16;

			HFONT hFont = (HFONT)::SendMessage(hWnd, WM_GETFONT, 0, 0);
			HFONT hOldFont = hFont ? (HFONT)::SelectObject(hMemDC, hFont) : NULL;

			int y = 0;
			for (int i = topIndex; i < count && y < rcClient.bottom; i++) {
				RECT rcItem = { rcClient.left, y, rcClient.right, y + itemHeight };

				BOOL bSel = (BOOL)::SendMessage(hWnd, LB_GETSEL, i, 0);
				COLORREF crBg = bSel ? RGB(0, 90, 158) : dc.crBackground;
				COLORREF crText = bSel ? RGB(255, 255, 255) : dc.crText;

				HBRUSH hItemBr = ::CreateSolidBrush(crBg);
				::FillRect(hMemDC, &rcItem, hItemBr);
				::DeleteObject(hItemBr);

				int len = (int)::SendMessage(hWnd, LB_GETTEXTLEN, i, 0);
				if (len > 0 && len != LB_ERR) {
					TCHAR* text = new TCHAR[len + 1];
					::SendMessage(hWnd, LB_GETTEXT, i, (LPARAM)text);
					::SetBkMode(hMemDC, TRANSPARENT);
					::SetTextColor(hMemDC, crText);
					RECT rcText = rcItem;
					rcText.left += 2;
					::DrawText(hMemDC, text, -1, &rcText,
						DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
					delete[] text;
				}
				y += itemHeight;
			}

			if (hOldFont) ::SelectObject(hMemDC, hOldFont);

			::BitBlt(hDC, 0, 0, w, h, hMemDC, 0, 0, SRCCOPY);
			::SelectObject(hMemDC, hOldBmp);
			::DeleteObject(hBmp);
			::DeleteDC(hMemDC);

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
		m_lastSel = 0;
}

BOOL CListExport::OnInitDialog()
{
	CResizePage::OnInitDialog();

	// Apply dark mode early, before content is loaded
	if (m_bDarkMode)
		SetDarkMode(true);

	if (m_bsort) OnToggleListStyle();
	if (m_pe->IsValid())
	{
		Load();
	}
	m_list.SetHorizontalExtent( m_Hsize * 10 );
	font = new(CFont);
	font->CreateFont( -FontSize, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, _T("Consolas") );
	m_list.SetFont( font );

    return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CListExport::OnToggleListStyle()
{
	CRect rc;
	CFont *fnt = m_list.GetFont();
	m_list.GetWindowRect(&rc);
	ScreenToClient(&rc);
	m_ListStyle = m_list.GetStyle() ^ LBS_SORT | WS_VSCROLL | WS_HSCROLL;
	m_list.DestroyWindow();
	m_list.Create( m_ListStyle, rc, this, IDC_LIST2 );
	m_list.ModifyStyleEx( 0, WS_EX_CLIENTEDGE, SWP_DRAWFRAME );
	m_list.SetFont( fnt );
	// Re-apply dark theme + subclass to the new HWND
	if (m_bDarkMode) {
		DarkMode_AllowForWindow(m_list.m_hWnd, TRUE);
		SetWindowTheme(m_list.m_hWnd, L"DarkMode_Explorer", NULL);
		SubclassListBoxForDark(m_list, true);
	}
	UpdateData(FALSE);
}

#define SIZEBUFFER 1024
extern PSTR Undecorate(PSTR Textin, PSTR Textout, int len);
void CListExport::AddFunction(int sel)
{
	m_list.SetRedraw(FALSE);
	m_list.ResetContent( );
	CString strTemp=_T("");
	int i;
	m_Hsize =0;
	int missingCount = 0;
	HINSTANCE hTestDll = NULL;
	PMODULE_FILE_INFO pModInfo = NULL;
	if (sel)
	{
// Import Section
		MODULE_DEPENDENCY_LIST *pDep = m_pe->GetDepends();
		m_listmodule.GetText( sel, strTemp );
		m_nbfunc = 0;
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

			CStringList *pFlist = pModInfo->GetFList();
			POSITION pos = pFlist->GetHeadPosition();
			if (!pos) return;
			CString func;
			do {
				func = pFlist->GetNext(pos);
				BOOL bMissing = FALSE;

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

				if (m_Hsize < strTemp.GetLength()) m_Hsize = strTemp.GetLength();
				m_list.AddString( strTemp );
				m_nbfunc ++;
			} while( pos );
		}
	}
	else
	{
// Export Section
		PIMAGE_EXPORT_DIRECTORY exportDir = m_pe->GetExportsDesc();
		m_nbfunc = 0;
		if ( exportDir  && m_pe->IsValidPtr(( ULONG_PTR) exportDir ))
		{
			PSTR	filename = (PSTR)m_pe->GetReadablePointerFromRVA( exportDir->Name );
			if (!(((ULONG_PTR) filename > (ULONG_PTR) exportDir + m_pe->GetFileSize()) || ((ULONG_PTR) filename < (ULONG_PTR) exportDir)))
			{
				PDWORD functions = ( PDWORD ) m_pe->GetReadablePointerFromRVA( exportDir->AddressOfFunctions );
				PWORD ordinals = (PWORD) m_pe->GetReadablePointerFromRVA( exportDir->AddressOfNameOrdinals );
				PDWORD name = ( PDWORD ) m_pe->GetReadablePointerFromRVA( exportDir->AddressOfNames);
				m_nbfunc =exportDir->NumberOfNames;
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

				// Display pass: iterate functions with O(1) name lookup
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
						m_list.AddString( displayName );
						int size = displayName.GetLength();
						if (m_Hsize < size) m_Hsize = size;
					}
					else
					{
						CString displayName;
						displayName.Format( _T("ordinal %d"), i );
						if (!strForwardTarget.IsEmpty())
						{
							int pad = padTo - displayName.GetLength();
							if (pad < 2) pad = 2;
							CString entry;
							entry.Format(_T("%s%*s-> %s"), (LPCTSTR)displayName, pad, _T(""), (LPCTSTR)strForwardTarget);
							displayName = entry;
						}
						m_list.AddString( displayName );
					}
				}
			}
		}
	}
	m_list.SetRedraw(TRUE);
	m_list.Invalidate();
	UpdateData(FALSE);

	// Update the test status label
	CWnd* pStatus = GetDlgItem(IDC_TESTSTATUS);
	if (pStatus)
	{
		if (sel == 0)
		{
			// Export section: no testing
			pStatus->SetWindowText(_T(""));
		}
		else if (!pModInfo || !pModInfo->IsModuleFound())
		{
			m_crTestStatus = m_bDarkMode ? RGB(255, 80, 80) : RGB(200, 0, 0);
			pStatus->SetWindowText(_T("DLL not found"));
		}
		else if (hTestDll == NULL)
		{
			m_crTestStatus = m_bDarkMode ? RGB(255, 80, 80) : RGB(200, 0, 0);
			pStatus->SetWindowText(_T("DLL load failed"));
		}
		else if (missingCount == 0)
		{
			m_crTestStatus = m_bDarkMode ? RGB(80, 200, 80) : RGB(0, 140, 0);
			pStatus->SetWindowText(_T("All OK"));
		}
		else
		{
			m_crTestStatus = m_bDarkMode ? RGB(255, 80, 80) : RGB(200, 0, 0);
			CString status;
			status.Format(_T("%d Missing"), missingCount);
			pStatus->SetWindowText(status);
		}
		pStatus->Invalidate();
	}
}

void CListExport::OnSelchangeModules()
{
	int sel = m_listmodule.GetCurSel( );
	if (sel == -1 || sel == m_lastSel) return;
	m_lastSel = sel;
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
	m_list.GetText( sel, func);
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
	SubclassListBoxForDark(m_list, bDark);
}

