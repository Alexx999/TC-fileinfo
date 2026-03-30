#ifndef __DECPLUG_H__
#define __DECPLUG_H__

#include "..\..\unaspack\unpack.h"

class PE_EXE;
class DecPlugin
{
public:
   DecPlugin( PE_EXE *pe );
    ~DecPlugin( );
   static CStringList pluglist;
   CString GetDecStr() { if (m_bComp) return m_sDecStr; else return _T(""); }
   DWORD GetSize() { return m_dwMemSize; }
   LPVOID GetNewHeader() { return m_pNewHeader; }
   BOOL IsComp() { return m_bComp; }
protected:
   BOOL PluginDec( int id, PE_EXE *pe );

   BOOL m_bComp;
   CString m_sPlugpath;
   CString m_sDecStr;
   DWORD m_dwMemSize;
   LPVOID m_pNewHeader;
};

#endif
