#include "stdafx.h"
#include "peexe.h"
#include "decplug.h"

//	pluglist must be initialize before using this class with the complete path of all plugins
//
//	for testing you can set 
//	m_sPlugpath = "C:\\Program Files\\Heaventools\\PE Explorer\\PLUGINS\\unupx.dll" ;
CStringList DecPlugin::pluglist;
CString *pDecStr = NULL;

void * __stdcall fnMemoryAllocator(DWORD len)
{
   return new BYTE[len];
}

void __stdcall fnPGICallBack(DWORD src, DWORD id, LPCTSTR s)
{
   CString str;
   str.Format(_T("Plugin %d  :\t%s \n"), src, s);
   if (pDecStr) *pDecStr += str;
}

DecPlugin::DecPlugin( PE_EXE *pPE )
{
   m_bComp = FALSE;
   m_pNewHeader = NULL;
   m_dwMemSize = 0; 
   if (!pluglist.IsEmpty())
   {
      int id=0;
      POSITION pos = pluglist.GetHeadPosition();
	  if (!pos) return;
      do 
      {
         m_sPlugpath = pluglist.GetNext(pos); 
         pDecStr = &m_sDecStr;
         if (m_bComp = PluginDec( id, pPE ))
             return;
         id++;
      } while(pos);
   }
}

DecPlugin::~DecPlugin( )
{
   if (m_pNewHeader) delete m_pNewHeader;
   m_pNewHeader = NULL;
   pDecStr = NULL;
}

BOOL DecPlugin::PluginDec(int id, PE_EXE *pPE)
{
   HINSTANCE h = LoadLibrary( m_sPlugpath );
   bool ret = FALSE;
   if (h)
   {
      LPCTSTR pStr;
      LPCTSTR *ppStr = &pStr;

      pRegisterPlugIn RPI = (pRegisterPlugIn) GetProcAddress( h, "PexRegisterPlugIn");
      if ( RPI )
         RPI(  ppStr );
//      pAboutPlugIn API = (pAboutPlugIn) GetProcAddress( h, "PexAboutPlugIn");
//      if (API)   API();
      pPreloadImage PLI = (pPreloadImage) GetProcAddress( h, "PexPreloadImage");
      if (PLI)
      {
         m_sDecStr.Empty();

         PGIParamsBlock PGIPB;
         PGIPB.pMemAllocator = &fnMemoryAllocator;
         PGIPB.pCallBack = &fnPGICallBack;
         PGIPB.pOutBuff = NULL;
         PGIPB.dwOutSize = 0;
         PGIPB.pInBuff = (PVOID) pPE->GetBase();
         PGIPB.dwInSize = pPE->GetFileSize();
         PGIPB.dwIndex = 0;
         PGIPB.dwInterface = id;
         if (PLI(&PGIPB))
         {
            fnPGICallBack(PGIPB.dwInterface, 0, _T("End of Work"));
            if (PGIPB.pOutBuff)
            {
               m_pNewHeader = PGIPB.pOutBuff;
               m_dwMemSize = PGIPB.dwOutSize;
            }
            ret = TRUE;
         }
      } 
      FreeLibrary( h );
   }
   return ret;
}

