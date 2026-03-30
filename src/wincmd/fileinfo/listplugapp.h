#ifndef __listplugapp_H
#define __listplugapp_H

#ifndef __AFXWIN_H__
   #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CListplugApp
//

class CListplugApp : public CWinApp
{
public:
   CListplugApp(LPCTSTR pszAppName);
   virtual BOOL InitInstance();

// Overrides
   //{{AFX_VIRTUAL(CListplugApp)
   //}}AFX_VIRTUAL

   //{{AFX_MSG(CListplugApp)
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()
};

#endif /* __listplugapp_H */
