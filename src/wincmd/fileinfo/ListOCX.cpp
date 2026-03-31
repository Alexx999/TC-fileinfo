// ListOCX.cpp : implementation file
//

#include "stdafx.h"
#include "ListOCX.h"
//#include <atlbase.h>
#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")

#include "TypeLib.h"
#include "..\common\wait.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern BOOL b_W95Protect;

/////////////////////////////////////////////////////////////////////////////
// CListOcx dialog
IMPLEMENT_DYNCREATE(CListOcx, CResizePage)

CListOcx::CListOcx()
	: CResizePage(CListOcx::IDD)
{
	m_pPE = NULL;
	m_bwrap = TRUE;
	m_pfnOrigListProc = NULL;
	m_fo.fontsize = 0;
	*m_fo.fontname = '\0';
	m_fo.fontbold = FALSE;
	m_fo.fontitalic = FALSE;

	//{{AFX_DATA_INIT(CListOcx)
	m_REocx = _T("");
	//}}AFX_DATA_INIT
}

void CListOcx::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CListOcx)
	DDX_Control(pDX, IDC_UnRegServer, m_bunreg);
	DDX_Control(pDX, IDC_RegServer, m_breg);
	DDX_Control(pDX, IDC_Listocx, m_listocx);
	DDX_Control(pDX, IDC_REocx, m_ocx);
	//}}AFX_DATA_MAP
}
//	DDX_Text(pDX, IDC_REocx, m_REocx);

BEGIN_MESSAGE_MAP(CListOcx, CResizePage)
	//{{AFX_MSG_MAP(CListOcx)
	ON_BN_CLICKED(IDC_RegServer, OnRegServer)
	ON_BN_CLICKED(IDC_UnRegServer, OnUnRegServer)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_Listocx, OnCustomDrawList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ListOCX message handlers
void CListOcx::Resize(CRect &rectPage)
{
	CResizePage::Resize(rectPage);

	m_listocx.GetWindowRect(&m_rectList);
	ScreenToClient(&m_rectList);
	m_rectList.right -= m_sizeRelChange.cx; // + rectPage.left;
	m_listocx.MoveWindow(&m_rectList);
		
	m_ocx.GetWindowRect(&m_rectOcx);
	ScreenToClient(&m_rectOcx);
	m_rectOcx.right -= m_sizeRelChange.cx; // + rectPage.left;
	m_rectOcx.bottom -= m_sizeRelChange.cy; // + rectPage.top;
	m_ocx.MoveWindow(&m_rectOcx);/**/

/* page haut Variable, bas fixe
	m_listocx.GetWindowRect(&m_rectList);
	ScreenToClient(&m_rectList);
	m_rectList.right -= m_sizeRelChange.cx; // + rectPage.left;
	m_rectList.bottom -= m_sizeRelChange.cy; // + rectPage.top;
	m_listocx.MoveWindow(&m_rectList);
		
	m_ocx.GetWindowRect(&m_rectOcx);
	ScreenToClient(&m_rectOcx);
//	m_rectOcx.left -= m_sizeRelChange.cx; // + rectPage.left;
	m_rectOcx.top -= m_sizeRelChange.cy; // + rectPage.top;
	m_rectOcx.right -= m_sizeRelChange.cx; // + rectPage.left;
	m_rectOcx.bottom -= m_sizeRelChange.cy; // + rectPage.top;
	m_ocx.MoveWindow(&m_rectOcx);/**/
}

CString CStringFromGUID(const GUID *guid)
{
	WCHAR null[128] = {0}, buff[128];
	StringFromGUID2(GUID_NULL, null, 127);
	return CString(StringFromGUID2(*guid, buff, 127) > 0 ? buff : null);
}

inline HRESULT  GetLibrary(CString &name, ITypeLib **pptlib)
{
	BSTR bstrTemp;	
	bstrTemp = name.AllocSysString(); //Get WideChar String
	HRESULT  hr = LoadTypeLib(bstrTemp, pptlib);
	::SysFreeString(bstrTemp);
	return hr;
}

char *DllList[]={"DllRegisterServer", "DllUnregisterServer", "DllGetClassObject"};
void CListOcx::Load() 
{
		CWait wait(this);
		wait.SetStatus(_T("Listing IDL..."));

		HINSTANCE hDLL;
		if (b_W95Protect)
			hDLL = LoadLibraryEx(m_pPE->GetName(), NULL, LOAD_LIBRARY_AS_DATAFILE);
		else 
			hDLL = LoadLibraryEx(m_pPE->GetName(), NULL, DONT_RESOLVE_DLL_REFERENCES); 
		bool bproc = FALSE;
		CString str;
		for(int i=0; i< 3; i++)
		{			
			HCRET lpfnDllRegisterServer = (HCRET) GetProcAddress(hDLL, DllList[i]);
			int  itemPos  =  m_listocx.InsertItem( i, CString(DllList[i]));
			if (lpfnDllRegisterServer )
			{
				str.Format(_T("%IXh"), (ULONG_PTR) lpfnDllRegisterServer);
				m_listocx.SetItemText( itemPos, 1, str);
				bproc = TRUE;
			} else m_listocx.SetItemText( itemPos, 1, _T("Entry point not found"));
		}
		FreeLibrary( hDLL );

		CString strTp = m_pPE->GetName();
		LPTYPELIB lpTypeLib = NULL;		
		if ( CTypeLib::IsTypeLib(m_pPE->GetName(), lpTypeLib) == S_OK) // com interface exist
		{
		    TLIBATTR *pTLibAttr;
//			m_REocx += strTp;
			if ( lpTypeLib->GetLibAttr(&pTLibAttr) == S_OK) // get table
			{
				GUID *g = &(pTLibAttr->guid);      // GUID
				if (bproc) 
				{	// IsRegistredClass
					ITypeLib *ptlib2;
					HRESULT  hr = LoadRegTypeLib(pTLibAttr->guid, pTLibAttr->wMajorVerNum, pTLibAttr->wMinorVerNum, pTLibAttr->lcid, &ptlib2);
					if (FAILED(hr)) m_breg.EnableWindow(TRUE);
					else m_bunreg.EnableWindow(TRUE);
				}
				lpTypeLib->ReleaseTLibAttr(pTLibAttr);
			}
			if (CTypeLib::GetTypeLibAsString(lpTypeLib, strTp) == S_OK)
				m_REocx += strTp;
			else m_REocx += _T("Error");
			lpTypeLib->Release();
		} else 
		{ // recuperer le message d'erreur
			strTp.Format(_T("LoadTypeLib( %s ) failed or TypeLib information absent"), m_pPE->GetName());
			m_REocx += strTp;
			if (bproc) 
			{
				m_breg.EnableWindow(TRUE);
				m_bunreg.EnableWindow(TRUE);
				m_REocx += _T("\r\nRegister state is unknown, but you can force to Register or Unregister Server");
			}

		}
	    m_ocx.SetWindowText( m_REocx );
	    m_ocx.SetOptions(ECOOP_OR, ECO_SAVESEL);
	    ApplyDarkTextFormat(m_ocx, m_bDarkMode);
}

void CListOcx::Renew(PE_EXE	*pPE) 
{
	m_pPE = pPE;
	if(m_listocx.m_hWnd) {
		m_listocx.DeleteAllItems();
		Load();
	}
}

BOOL CListOcx::OnInitDialog()
{
	CResizePage::OnInitDialog();

	// Apply dark mode early, before content is loaded
	if (m_bDarkMode)
		SetDarkMode(true);

	UpdateFont();
	m_listocx.InsertColumn(0,_T("Name"));
	m_listocx.InsertColumn(1,_T("Address"));
	m_listocx.SetColumnWidth( 0, 250 );
	m_listocx.SetColumnWidth( 1, 150);

	if (m_pPE)
	{
		Load();
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CListOcx::OnRegServer() 
{
	HINSTANCE hDLL = LoadLibrary(m_pPE->GetName());
	if (!hDLL) return;
	HCRET lpfnDllRegisterServer = (HCRET) GetProcAddress(hDLL, DllList[0]);
	if(FAILED((*lpfnDllRegisterServer)()))
		return;
	m_breg.EnableWindow(FALSE);
	m_bunreg.EnableWindow(TRUE);
	FreeLibrary( hDLL );
}

void CListOcx::OnUnRegServer() 
{
	HINSTANCE hDLL = LoadLibrary(m_pPE->GetName());
	if (!hDLL) return;
	HCRET lpfnDllRegisterServer = (HCRET) GetProcAddress(hDLL, DllList[1]);
	if(FAILED((*lpfnDllRegisterServer)()))
		return;
	m_breg.EnableWindow(TRUE);
	m_bunreg.EnableWindow(FALSE);
	FreeLibrary( hDLL );
}

BOOL CListOcx::PreTranslateMessage(MSG* pMsg) 
{
//	if ((0x8000 & GetKeyState(VK_CONTROL))) // if CTRL
   if (pMsg->message == WM_KEYDOWN ) 
   {

		if (pMsg->wParam == 'W')
			{
				m_bwrap=!m_bwrap;
				SetWrap(m_bwrap);
				return 0;
			}
	}
// EN_MSGFILTER 
   if (pMsg->message == EM_SETSEL)
   {
      m_ocx.SetSel( 0, -1 );
      return 1;
   }
   if (pMsg->message == WM_COPY)
   {
      m_ocx.Copy();
      return 1;
   }
   return CResizePage::PreTranslateMessage(pMsg);
}

void CListOcx::UpdateFont( void )
{
   CHARFORMAT cf; BOOL modif = FALSE;
   m_ocx.GetDefaultCharFormat( cf );
   if (m_fo.fontsize)
   {
      cf.dwMask = CFM_SIZE;
      cf.yHeight = m_fo.fontsize;
      modif = TRUE;
   }
   if (*m_fo.fontname)
   {
      cf.dwMask = cf.dwMask | CFM_FACE;
      _tcscpy_s(cf.szFaceName, m_fo.fontname);
      modif = TRUE;
   }

   if (m_fo.fontbold)
   {
      cf.dwMask |= CFM_BOLD;
      cf.dwEffects = CFE_BOLD;
      modif = TRUE;
   }
   
   if (m_fo.fontitalic)
   {
      cf.dwMask |= CFM_ITALIC;
      cf.dwEffects = CFE_ITALIC;
      modif = TRUE;
   }
   if (modif)
      m_ocx.SetDefaultCharFormat( cf );
}

void CListOcx::SetDarkMode(bool bDark)
{
	CResizePage::SetDarkMode(bDark);

	ApplyDarkRichEdit(m_ocx, bDark);

	if (m_listocx.m_hWnd) {
		if (bDark) {
			DarkModeColors dc = GetDarkColors();
			m_listocx.SetBkColor(dc.crBackground);
			m_listocx.SetTextColor(dc.crText);
			m_listocx.SetTextBkColor(dc.crBackground);
			m_listocx.ModifyStyleEx(WS_EX_CLIENTEDGE, 0, SWP_FRAMECHANGED);
			// Enable double-buffering to reduce flicker
			DWORD exStyle = ListView_GetExtendedListViewStyle(m_listocx.m_hWnd);
			ListView_SetExtendedListViewStyle(m_listocx.m_hWnd, exStyle | LVS_EX_DOUBLEBUFFER);
		} else {
			m_listocx.SetBkColor(::GetSysColor(COLOR_WINDOW));
			m_listocx.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
			m_listocx.SetTextBkColor(::GetSysColor(COLOR_WINDOW));
			m_listocx.ModifyStyleEx(0, WS_EX_CLIENTEDGE, SWP_FRAMECHANGED);
		}

		// Use AllowDarkModeForWindow + "ItemsView" theme for native dark header
		// (Notepad++/win32-darkmode approach: ordinal 133 + NM_CUSTOMDRAW text fix)
		DarkMode_AllowForWindow(m_listocx.m_hWnd, bDark);
		HWND hHeader = ListView_GetHeader(m_listocx.m_hWnd);
		if (hHeader) {
			DarkMode_AllowForWindow(hHeader, bDark);
			SetWindowTheme(hHeader, bDark ? L"ItemsView" : NULL, NULL);

			// Subclass ListView to intercept header's NM_CUSTOMDRAW and fix text color
			if (bDark) {
				if (!m_pfnOrigListProc) {
					::SetProp(m_listocx.m_hWnd, _T("DarkOcx"), (HANDLE)this);
					m_pfnOrigListProc = (WNDPROC)::SetWindowLongPtr(
						m_listocx.m_hWnd, GWLP_WNDPROC, (LONG_PTR)DarkListProc);
				}
			} else {
				if (m_pfnOrigListProc) {
					::SetWindowLongPtr(m_listocx.m_hWnd, GWLP_WNDPROC, (LONG_PTR)m_pfnOrigListProc);
					::RemoveProp(m_listocx.m_hWnd, _T("DarkOcx"));
					m_pfnOrigListProc = NULL;
				}
			}
			::InvalidateRect(hHeader, NULL, TRUE);
		}
		SetWindowTheme(m_listocx.m_hWnd, bDark ? L"Explorer" : NULL, NULL);
		m_listocx.Invalidate(TRUE);
	}
}

LRESULT CALLBACK CListOcx::DarkListProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CListOcx* pOcx = (CListOcx*)::GetProp(hWnd, _T("DarkOcx"));
	if (!pOcx || !pOcx->m_pfnOrigListProc)
		return ::DefWindowProc(hWnd, msg, wParam, lParam);

	// Intercept header's NM_CUSTOMDRAW to fix text color
	// ("ItemsView" theme gives dark bg but also dark text -- we override text to light)
	if (msg == WM_NOTIFY)
	{
		LPNMHDR pNMHdr = (LPNMHDR)lParam;
		if (pNMHdr && pNMHdr->code == NM_CUSTOMDRAW)
		{
			HWND hHeader = ListView_GetHeader(hWnd);
			if (hHeader && pNMHdr->hwndFrom == hHeader)
			{
				LPNMCUSTOMDRAW nmcd = (LPNMCUSTOMDRAW)lParam;
				switch (nmcd->dwDrawStage)
				{
				case CDDS_PREPAINT:
					return CDRF_NOTIFYITEMDRAW;
				case CDDS_ITEMPREPAINT:
					{
						DarkModeColors dc = GetDarkColors();
						::SetTextColor(nmcd->hdc, dc.crText);
						return CDRF_DODEFAULT;
					}
				}
			}
		}
	}

	if (msg == WM_NCDESTROY)
	{
		WNDPROC origProc = pOcx->m_pfnOrigListProc;
		::RemoveProp(hWnd, _T("DarkOcx"));
		pOcx->m_pfnOrigListProc = NULL;
		return ::CallWindowProc(origProc, hWnd, msg, wParam, lParam);
	}

	return ::CallWindowProc(pOcx->m_pfnOrigListProc, hWnd, msg, wParam, lParam);
}

void CListOcx::OnCustomDrawList(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVCUSTOMDRAW* pLVCD = (NMLVCUSTOMDRAW*)pNMHDR;
	*pResult = CDRF_DODEFAULT;

	if (!m_bDarkMode) return;

	DarkModeColors dc = GetDarkColors();

	switch (pLVCD->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		*pResult = CDRF_NOTIFYITEMDRAW;
		break;

	case CDDS_ITEMPREPAINT:
		pLVCD->clrText = dc.crText;
		pLVCD->clrTextBk = dc.crBackground;
		*pResult = CDRF_NEWFONT;
		break;
	}
}

/*
		    TLIBATTR *pTLibAttr;

			BSTR BstrHelpFile[3];
			ptlib->GetDocumentation( -1, &(BstrHelpFile[0]), &(BstrHelpFile[1]), NULL, &(BstrHelpFile[2]));
			strTp.Format("%S\r\n\r\nName\t: %S\r\nHelp file\t: %S\r\n", BstrHelpFile[1], BstrHelpFile[0], BstrHelpFile[2]);
			m_REocx += strTp;
			SysFreeString(BstrHelpFile[0]);
			SysFreeString(BstrHelpFile[1]);
			SysFreeString(BstrHelpFile[2]);

			if ( ptlib->GetLibAttr(&pTLibAttr) == S_OK) // get table
			{
				GUID *g = &(pTLibAttr->guid);      // GUID
				
				{	// IsRegistredClass
					ITypeLib *ptlib2;
					HRESULT  hr = LoadRegTypeLib(pTLibAttr->guid, pTLibAttr->wMajorVerNum, pTLibAttr->wMinorVerNum, pTLibAttr->lcid, &ptlib2);					
					if (FAILED(hr)) m_bunreg.EnableWindow(FALSE);
					else m_breg.EnableWindow(FALSE);
				}

				m_REocx += "GUID \t: ";
				m_REocx += CStringFromGUID( g );

				strTp.Format("\r\nLCID \t: %08X \r\n", pTLibAttr->lcid);
				m_REocx += strTp;
				strTp.Format("SysKind \t: %s \r\n", 
					pTLibAttr->syskind==0?"SYS-WIN16":pTLibAttr->syskind==1?"SYS_WIN32":"SYS_MAC");
				m_REocx += strTp;
				strTp.Format("LibFlags \t: %04X \r\n", pTLibAttr->wLibFlags);
				m_REocx += strTp;
				strTp.Format("Version \t: %d.%d \r\n", pTLibAttr->wMajorVerNum, pTLibAttr->wMinorVerNum);
				m_REocx += strTp;

				ptlib->ReleaseTLibAttr(pTLibAttr);
			}
			ptlib->Release();

*/