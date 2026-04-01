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
	m_pDllCtx = NULL;
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
	// Clear tree items first — they may hold MODULE_FILE_INFO* pointers
	// into the DllTreeContext that we're about to delete.
	if (m_tree.m_hWnd) m_tree.DeleteAllItems();
	if (m_pDllCtx) DeleteDllTreeContext(m_pDllCtx);
	m_pDllCtx = NULL;
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
	ON_NOTIFY(TVN_ITEMEXPANDING, IDC_TREE1, OnItemExpanding)
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
			m_tree.SetRedraw(FALSE);
			CWait wait(this);
			((pfunc) FillTree) (m_ptr, m_tree, wait);

			// Capture the DllTreeContext from the root item (set by CreateParentTree)
			HTREEITEM hRoot = m_tree.GetRootItem();
			if (hRoot)
			{
				m_pDllCtx = (DllTreeContext*) m_tree.GetItemData(hRoot);
				m_tree.SetItemData(hRoot, 0);  // root item no longer owns it
			}

			VERSION ver = GetSystemVersion();
			if (ver.ver >= WND_XP)
				m_tree.Expand(m_tree.GetRootItem(), TVE_EXPAND);
			m_tree.SetRedraw(TRUE);
			m_tree.Invalidate();
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

	// Enable double-buffering to reduce flicker when hovering over icons
	TreeView_SetExtendedStyle(m_tree.m_hWnd, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);

	// Use a monospace font for aligned display of DLL names and paths
	m_pTreeFont = new CFont();
	m_pTreeFont->CreateFont(-12, 0, 0, 0, FW_NORMAL, 0, 0, 0,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, _T("Consolas"));
	m_tree.SetFont(m_pTreeFont);

	if (FillTree)
	{
		m_tree.SetRedraw(FALSE);
		CWait wait(this);
		((pfunc) FillTree) (m_ptr, m_tree, wait);

		// Capture the DllTreeContext from the root item (set by CreateParentTree)
		HTREEITEM hRoot = m_tree.GetRootItem();
		if (hRoot)
		{
			m_pDllCtx = (DllTreeContext*) m_tree.GetItemData(hRoot);
			m_tree.SetItemData(hRoot, 0);  // root item no longer owns it
		}

		VERSION ver = GetSystemVersion();
		if (ver.ver >= WND_XP)
			m_tree.Expand(m_tree.GetRootItem(), TVE_EXPAND);
		m_tree.SetRedraw(TRUE);
		m_tree.Invalidate();
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

void CPageTree::OnItemExpanding(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_TREEVIEW* pNMTV = (NM_TREEVIEW*)pNMHDR;
	*pResult = 0;

	if (pNMTV->action != TVE_EXPAND || !m_pDllCtx)
		return;

	// Lazily populate children when expanding for the first time.
	// DllTreeContext_ExpandNode checks for a dummy placeholder child,
	// replaces it with real dependency entries (including TestFunction
	// and their own dummy children for further lazy expansion).
	DllTreeContext_ExpandNode(m_pDllCtx, m_tree, pNMTV->itemNew.hItem);
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
			// Dark-theme the tree's built-in tooltip (shown for clipped items)
			HWND hTip = TreeView_GetToolTips(m_tree.m_hWnd);
			if (hTip) {
				DarkMode_AllowForWindow(hTip, TRUE);
				SetWindowTheme(hTip, L"DarkMode_Explorer", NULL);
			}
		} else {
			m_tree.SetBkColor((COLORREF)-1); // reset to default
			m_tree.SetTextColor((COLORREF)-1);
			m_tree.SetLineColor(CLR_DEFAULT);
			m_tree.ModifyStyleEx(0, WS_EX_CLIENTEDGE, SWP_FRAMECHANGED);
			SetWindowTheme(m_tree.m_hWnd, NULL, NULL);
			HWND hTip = TreeView_GetToolTips(m_tree.m_hWnd);
			if (hTip) {
				DarkMode_AllowForWindow(hTip, FALSE);
				SetWindowTheme(hTip, NULL, NULL);
			}
		}
		m_tree.Invalidate(TRUE);
	}
}
