// PageTree.cpp : implementation file
//

#include "stdafx.h"
#include "PageTree.h"

#include "..\..\common\verwin.h"
#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPageTree property page
IMPLEMENT_DYNCREATE(CPageTree, CResizePage)

CPageTree::CPageTree() : CResizePage(CPageTree::IDD)
{
//{{AFX_DATA_INIT(CPageTree)
	FillTree = NULL;
	pImageList = NULL;
	m_pTreeFont = NULL;
//}}AFX_DATA_INIT
}

CPageTree::~CPageTree()
{
	TRACE0("Delete TreePage \n");
	CleanUp();
}

void CPageTree::OnDestroy() 
{
	TRACE0("CPageTree : OnDestroy \n");
	CleanUp();
}

void CPageTree::CleanUp()
{
	TRACE0("CPageTree : CleanUp \n");
	if (m_pTreeFont) delete m_pTreeFont;
	m_pTreeFont = NULL;
	if (pImageList) delete pImageList;
	pImageList = NULL;
}

void CPageTree::DoDataExchange(CDataExchange* pDX)
{
	CResizePage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPageTree)
	DDX_Control(pDX, IDC_TREE1, m_tree);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPageTree, CResizePage)
	//{{AFX_MSG_MAP(CPageTree)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPageTree message handlers
void CPageTree::Resize(CRect &rectPage)
{
	CResizePage::Resize(rectPage);

	m_tree.GetWindowRect(&m_rectTree);
	ScreenToClient(&m_rectTree);
	m_rectTree.right -= m_sizeRelChange.cx; // + rectPage.left;
	m_rectTree.bottom -= m_sizeRelChange.cy; // + rectPage.top;
	m_tree.MoveWindow(&m_rectTree);

//	m_Redit.SetSel( 0, 0);
}

void CPageTree::Renew(PVOID pPE) 
{
	CleanUp();
	m_ptr = pPE;
	if (m_tree.m_hWnd) 
		if (FillTree) 
		{	
			CWait wait(this);
			((pfunc) FillTree) (m_ptr, m_tree, wait);

	// Expand the first branch on W95/WNT/W2k  ( à TESTER )
			VERSION ver = GetSystemVersion();
			if (ver.ver >= WND_XP)
				m_tree.Expand(m_tree.GetRootItem(), TVE_EXPAND);
		}
}

BOOL CPageTree::OnInitDialog()
{
	CResizePage::OnInitDialog();

	// Apply dark mode early, before content is loaded and shown
	if (m_bDarkMode)
		SetDarkMode(true);

	CBitmap      bitmap;
	pImageList = new CImageList();
	pImageList->Create(23, 16, ILC_COLOR16 | ILC_MASK, 4, 4);

	bitmap.LoadBitmap(IDB_FOUND);
	pImageList->Add(&bitmap, (COLORREF)0xFFFFFF);
	bitmap.DeleteObject();

	bitmap.LoadBitmap(IDB_NOTFOUND);
	pImageList->Add(&bitmap, (COLORREF)0xFFFFFF);
	bitmap.DeleteObject();

	bitmap.LoadBitmap(IDB_DELAYED);
	pImageList->Add(&bitmap, (COLORREF)0xFFFFFF);
	bitmap.DeleteObject();

	bitmap.LoadBitmap(IDB_NFDELAYED);
	pImageList->Add(&bitmap, (COLORREF)0xFFFFFF);
	bitmap.DeleteObject();

	bitmap.LoadBitmap(IDB_MISSING);
	pImageList->Add(&bitmap, (COLORREF)0xFFFFFF);
	bitmap.DeleteObject();

	bitmap.LoadBitmap(IDB_DELMISSING);
	pImageList->Add(&bitmap, (COLORREF)0xFFFFFF);
	bitmap.DeleteObject();

	m_tree.SetImageList(pImageList, TVSIL_NORMAL);

	// Use a monospace font for aligned display of DLL names and paths
	m_pTreeFont = new CFont();
	m_pTreeFont->CreateFont(-12, 0, 0, 0, FW_NORMAL, 0, 0, 0,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, _T("Consolas"));
	m_tree.SetFont(m_pTreeFont);

	if (FillTree)
	{
		CWait wait(this);
		((pfunc) FillTree) (m_ptr, m_tree, wait);

// Expand the first branch on W95/WNT/W2k  ( à TESTER )
		VERSION ver = GetSystemVersion();
		if (ver.ver >= WND_XP)
			m_tree.Expand(m_tree.GetRootItem(), TVE_EXPAND);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPageTree::PreTranslateMessage(MSG* pMsg) 
{
   if (pMsg->message == WM_KEYDOWN ) 
   {
		if (pMsg->wParam == 0x6a)
		{
			m_tree.Expand(m_tree.GetRootItem(), TVE_TOGGLE);
//			BOOL EnsureVisible( HTREEITEM hItem );
			return 0;
		}
		if (pMsg->wParam == VK_F3)
		{
			SHELLEXECUTEINFO sei  = { sizeof(sei ) };
			sei.fMask = SEE_MASK_DOENVSUBST;
			sei.nShow = SW_SHOWNORMAL;
	//		sei.lpVerb = argv[1];
			sei.lpFile = _T("%commander_exe%");
			CString com = _T("/S=L \" ") + m_tree.GetItemText(m_tree.GetSelectedItem()) + _T("\"");
			sei.lpParameters = com;
			if(ShellExecuteEx(&sei) == FALSE)
			{
				CString mess;
				mess.Format(_T("ShellExecuteEx Error \narg : %s"),(LPCTSTR)com);
				AfxMessageBox(mess,MB_ICONEXCLAMATION);
			}


			//%commander_exe% /S=L "nom du fichier"

//			com.GetBuffer(0);
//			com.ReleaseBuffer();
			return 0;
		}
	}
	return CResizePage::PreTranslateMessage(pMsg);
}

void CPageTree::SetDarkMode(bool bDark)
{
	CResizePage::SetDarkMode(bDark);

	if (m_tree.m_hWnd) {
		if (bDark) {
			DarkModeColors dc = GetDarkColors();
			m_tree.SetBkColor(dc.crBackground);
			m_tree.SetTextColor(dc.crText);
			m_tree.SetLineColor(dc.crText);
			m_tree.ModifyStyle(WS_BORDER, 0, SWP_FRAMECHANGED);
			m_tree.ModifyStyleEx(WS_EX_CLIENTEDGE, 0, SWP_FRAMECHANGED);
			SetWindowTheme(m_tree.m_hWnd, L"DarkMode_Explorer", NULL);
		} else {
			m_tree.SetBkColor((COLORREF)-1); // reset to default
			m_tree.SetTextColor((COLORREF)-1);
			m_tree.SetLineColor(CLR_DEFAULT);
			m_tree.ModifyStyleEx(0, WS_EX_CLIENTEDGE, SWP_FRAMECHANGED);
			SetWindowTheme(m_tree.m_hWnd, NULL, NULL);
		}
		m_tree.Invalidate(TRUE);
	}
}
