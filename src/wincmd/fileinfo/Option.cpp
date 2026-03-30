// COption.cpp : implementation file
//

#include "stdafx.h"
#include "Option.h"

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
   UpdateData(FALSE);
   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void COption::OnBnClickedEdit()
{
	ShellExecute(0, _T("open"), _T("notepad.exe"), op.p_ini, 0, SW_NORMAL);
}