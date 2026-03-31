// ListExport.cpp : implementation file
//

#include "stdafx.h"
#include "ListExport.h"
#include "ListDlg.h"
#include <imagehlp.h>
#include <afxole.h> // for clipboard
#include <afxadv.h> // shared file
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
	m_bsort = FALSE;
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
	ON_BN_CLICKED(IDC_BUTTON1, OnTestImport)
	ON_LBN_SELCHANGE(IDC_LIST2, OnSelchangeFunc)
	ON_BN_CLICKED(IDC_CHECK1, OnSort)
	ON_WM_DESTROY()
	ON_WM_DRAWITEM()
	ON_WM_MEASUREITEM()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dark mode helpers for owner-drawn listboxes

// Recreate a listbox with/without LBS_OWNERDRAWFIXED.
// LBS_OWNERDRAWFIXED must be present at creation time; ModifyStyle won't work.
static void RecreateListBoxForDarkMode(CListBox& lb, CWnd* pParent, UINT nID, bool bDark)
{
	if (!lb.m_hWnd) return;

	// Save state
	CRect rc;
	lb.GetWindowRect(&rc);
	pParent->ScreenToClient(&rc);
	CFont* pFont = lb.GetFont();
	int sel = lb.GetCurSel();
	int hExtent = lb.GetHorizontalExtent();
	DWORD style = lb.GetStyle();
	DWORD exStyle = ::GetWindowLong(lb.m_hWnd, GWL_EXSTYLE);

	// Save content
	int count = lb.GetCount();
	CString* items = NULL;
	if (count > 0) {
		items = new CString[count];
		for (int i = 0; i < count; i++)
			lb.GetText(i, items[i]);
	}

	// Destroy
	lb.DestroyWindow();

	// Modify style for dark/light
	if (bDark) {
		style = (style | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS) & ~WS_BORDER;
		exStyle &= ~(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
	} else {
		style = (style & ~LBS_OWNERDRAWFIXED) | WS_BORDER;
		exStyle |= WS_EX_CLIENTEDGE;
	}

	// Recreate
	lb.CreateEx(exStyle, _T("LISTBOX"), NULL,
		style | WS_CHILD | WS_VISIBLE, rc, pParent, nID);
	if (pFont) lb.SetFont(pFont);

	// Restore content
	if (items) {
		for (int i = 0; i < count; i++)
			lb.AddString(items[i]);
		delete[] items;
	}
	if (sel >= 0 && sel < lb.GetCount())
		lb.SetCurSel(sel);
	lb.SetHorizontalExtent(hExtent);
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
		wait.SetStatus(_T("Listing Functions..."));
		AddFunction(0);
		m_listmodule.SetCurSel( 0 );
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
	font->CreateFont( -FontSize, 0, 0, 0, FW_THIN, 0, 0, 0, ANSI_CHARSET, OUT_DEVICE_PRECIS, CLIP_CHARACTER_PRECIS, PROOF_QUALITY, FF_MODERN, _T("Tahoma") ); // modern Courrier New
	m_list.SetFont( font );
	// After font change, re-send WM_MEASUREITEM by recreating if in dark mode
	if (m_bDarkMode)
		RecreateListBoxForDarkMode(m_list, this, IDC_LIST2, true);

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
	if (m_bDarkMode)
		m_ListStyle |= LBS_OWNERDRAWFIXED | LBS_HASSTRINGS;
	m_list.DestroyWindow();
	m_list.Create( m_ListStyle, rc, this, IDC_LIST2 );
	if (!m_bDarkMode)
		m_list.ModifyStyleEx( 0, WS_EX_CLIENTEDGE, SWP_DRAWFRAME );
	m_list.SetFont( fnt );
	UpdateData(FALSE);
}

#define SIZEBUFFER 1024
extern PSTR Undecorate(PSTR Textin, PSTR Textout, int len);
void CListExport::AddFunction(int sel)
{
	m_list.ResetContent( );
	CString strTemp=_T("");
	int i;
	m_Hsize =0;
	if (sel)
	{
// Import Section
		MODULE_DEPENDENCY_LIST *pDep = m_pe->GetDepends();
		PMODULE_FILE_INFO pModInfo;
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
				CStringList *pFlist = pModInfo->GetFList();
				POSITION pos = pFlist->GetHeadPosition();
				if (!pos) return;
				CString func;
				do {
					func = pFlist->GetNext(pos);
					if (_tcsncmp(_T("ordinal "), func, 8)==0)
						strTemp = func;
					else
					{
						if ( m_undecorate ) // Undecorate Name
						{
				 			char Textout[SIZEBUFFER];
							strTemp = CA2T(Undecorate(CT2A(func), Textout, SIZEBUFFER));
						}
						else strTemp.Format(_T("%s"), (LPCTSTR)func);
					}
					if (m_Hsize < strTemp.GetLength()) m_Hsize = strTemp.GetLength();
					m_list.AddString( strTemp );
					m_nbfunc ++;
				} while( pos );
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

				CStringList *pFlist = pModInfo->GetFList();
				POSITION pos = pFlist->GetHeadPosition();
				if (!pos) return;
				CString func;
				do {
					func = pFlist->GetNext(pos);
					if (_tcsncmp(_T("ordinal "), func, 8)==0)
						strTemp = func;
					else
					{
						if ( m_undecorate ) // Undecorate Name
						{
				 			char Textout[SIZEBUFFER];
							strTemp = CA2T(Undecorate(CT2A(func), Textout, SIZEBUFFER));
						}
						else strTemp.Format(_T("%s"), (LPCTSTR)func);
					}
					if (m_Hsize < strTemp.GetLength()) m_Hsize = strTemp.GetLength();
					m_list.AddString( strTemp );
					m_nbfunc ++;
				} while( pos );
			}
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
				for(i=0; i < (int) exportDir->NumberOfFunctions; i++)
				{
					BOOL found = FALSE;
					DWORD entryPointRVA = functions[i];
					if ( entryPointRVA == 0 ) continue; // Skip over gaps in exported function
					// Check if this export is a forwarder
					BOOL isForwarder = (entryPointRVA >= exportsStartRVA) && (entryPointRVA <= exportsEndRVA);
					CString strForward;
					if (isForwarder)
					{
						PSTR pszForward = (PSTR)m_pe->GetReadablePointerFromRVA(entryPointRVA);
						if (pszForward)
							strForward.Format(_T("  ->  %S"), pszForward);
					}
					for ( int j=0; j < (int) exportDir->NumberOfNames; j++ )
						if ( ordinals[j] == i )
						{
							found = TRUE;
							PSTR Name = (PSTR) m_pe->GetReadablePointerFromRVA( name[j] );
							CString displayName;
							if ( m_undecorate ) 	// Undecorate Name
							{
								char Textout[SIZEBUFFER];
								displayName = CString(Undecorate(Name, Textout, SIZEBUFFER));
							}
							else displayName = CString(Name);
							if (!strForward.IsEmpty())
								displayName += strForward;
							m_list.AddString( displayName );
							int size = displayName.GetLength();
							if (m_Hsize < size) m_Hsize = size;
						}
					if (!found)
					{
						CString displayName;
						displayName.Format( _T("ordinal %d"), i );
						if (!strForward.IsEmpty())
							displayName += strForward;
						m_list.AddString( displayName );
					}

				}
			}
		}
	}
	UpdateData(FALSE);
}

void CListExport::OnSelchangeModules() 
{
	CWait wait(this);
	wait.SetStatus(_T("Listing Functions..."));
	int sel = m_listmodule.GetCurSel( );
	if (sel == -1) return;
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

void CListExport::OnTestImport() 
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

			if (!pModInfo) return;
			while(_tcsncmp(Module, pModInfo->GetBaseName(), Module.GetLength()-decal))
			{
				if (decal) pModInfo = pDep->GetNextDelayedModule( pModInfo );
				else pModInfo = pDep->GetNextModule( pModInfo );
				if (!pModInfo) return;
			}
			if (decal) Module.SetAt( Module.GetLength()-decal, _T('\0'));
			CStringList *pFlist = pModInfo->GetFList();
			POSITION pos = pFlist->GetHeadPosition();

			int error=0;
			TCHAR szPath[MAX_PATH];
			TCHAR szOriginalPath[MAX_PATH];
			GetCurrentDirectory(MAX_PATH, szOriginalPath);  // Save original dir
			SetCurrentDirectory( m_pe->GetPath() );				 // Switch to app's dir

			LPTSTR pszDontCare;
			if ((pos) && SearchPath(0, Module, 0, MAX_PATH, szPath, &pszDontCare))
			{
				HINSTANCE h;
				if (b_W95Protect)
					h = LoadLibraryEx(szPath, NULL, LOAD_LIBRARY_AS_DATAFILE);
				else
					h = LoadLibraryEx(szPath, NULL, DONT_RESOLVE_DLL_REFERENCES);
				if (h)
				{
					CStringList StrL;
					CString func;
					do {
						func = pFlist->GetNext(pos);
						if (_tcsncmp(_T("ordinal "), func, 8)==0)
						{
							if (!GetProcAddress( h, MAKEINTRESOURCEA(_ttoi((LPCTSTR) func + 8))))
								error ++;
						}
						else if (!GetProcAddress( h, CT2A(func) ))
						{
							error ++;
							StrL.AddTail(func);
						}
					} while( pos );
					if (!error) AfxMessageBox(_T("Module and all imported functions loaded"));
					else {
						ListDlg ldlg(&StrL);
						ldlg.DoModal();
					}
					FreeLibrary( h );
				} else AfxMessageBox(_T("Cannot load Module "));
			} else AfxMessageBox(_T("Cannot find Module "));
			SetCurrentDirectory( szOriginalPath );
		}/**/
	}
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
	RecreateListBoxForDarkMode(m_listmodule, this, IDC_LIST1, bDark);
	RecreateListBoxForDarkMode(m_list, this, IDC_LIST2, bDark);
}

void CListExport::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS)
{
	if (!lpDIS || lpDIS->itemID == (UINT)-1) return;
	if (nIDCtl != IDC_LIST1 && nIDCtl != IDC_LIST2) {
		CResizePage::OnDrawItem(nIDCtl, lpDIS);
		return;
	}

	CListBox* pList = (nIDCtl == IDC_LIST1) ? &m_listmodule : &m_list;
	bool bSelected = (lpDIS->itemState & ODS_SELECTED) != 0;
	DarkModeColors dc = GetDarkColors();

	COLORREF crBg = bSelected ? RGB(0, 90, 158) : dc.crBackground;
	COLORREF crText = bSelected ? RGB(255, 255, 255) : dc.crText;

	// Fill background
	HBRUSH hBr = ::CreateSolidBrush(crBg);
	::FillRect(lpDIS->hDC, &lpDIS->rcItem, hBr);
	::DeleteObject(hBr);

	// Draw text
	CString str;
	if (lpDIS->itemID < (UINT)pList->GetCount())
		pList->GetText(lpDIS->itemID, str);
	::SetBkMode(lpDIS->hDC, TRANSPARENT);
	::SetTextColor(lpDIS->hDC, crText);
	RECT rcText = lpDIS->rcItem;
	rcText.left += 2;
	::DrawText(lpDIS->hDC, str, -1, &rcText,
		DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	// Skip DrawFocusRect in dark mode — XOR drawing looks pink on dark backgrounds
}

void CListExport::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMIS)
{
	if (nIDCtl == IDC_LIST1 || nIDCtl == IDC_LIST2) {
		CListBox* pList = (nIDCtl == IDC_LIST1) ? &m_listmodule : &m_list;
		CDC* pDC = pList->m_hWnd ? pList->GetDC() : GetDC();
		if (pDC) {
			CFont* pFont = (pList->m_hWnd) ? pList->GetFont() : NULL;
			CFont* pOld = pFont ? pDC->SelectObject(pFont) : NULL;
			TEXTMETRIC tm;
			pDC->GetTextMetrics(&tm);
			lpMIS->itemHeight = tm.tmHeight + tm.tmExternalLeading + 2;
			if (pOld) pDC->SelectObject(pOld);
			if (pList->m_hWnd) pList->ReleaseDC(pDC); else ReleaseDC(pDC);
		} else {
			lpMIS->itemHeight = 16;
		}
		return;
	}
	CResizePage::OnMeasureItem(nIDCtl, lpMIS);
}