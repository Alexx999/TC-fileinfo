#include "stdafx.h"
#include "winnt.h"
#include "winbase.h"
#include "verwin.h"

/*   --- Changer dans Winbase.h ---

typedef struct _OSVERSIONINFOEXA {
    DWORD dwOSVersionInfoSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;
    CHAR   szCSDVersion[ 128 ];     // Maintenance string for PSS usage
    WORD wServicePackMajor;
    WORD wServicePackMinor;
	WORD wSuiteMask;
	WORD wProductType;
    WORD wReserved;
} OSVERSIONINFOEXA, *POSVERSIONINFOEXA, *LPOSVERSIONINFOEXA;
*/

// RtlGetVersion doesn't lie about the OS version (unlike GetVersionEx on 8.1+)
typedef LONG (WINAPI *pfnRtlGetVersion)(OSVERSIONINFOEXW *);

static BOOL GetRealVersionInfo(OSVERSIONINFOEXW *osvi)
{
	pfnRtlGetVersion pRtlGetVersion = (pfnRtlGetVersion)
		GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion");
	if (pRtlGetVersion)
	{
		ZeroMemory(osvi, sizeof(OSVERSIONINFOEXW));
		osvi->dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
		if (pRtlGetVersion(osvi) == 0) // STATUS_SUCCESS
			return TRUE;
	}
	return FALSE;
}

VERSION GetSystemVersion()
{
#define BUFSIZE 256
	VERSION ver={0,0,0,0};
   OSVERSIONINFOEXW osvi;

   // Use RtlGetVersion for accurate version on Windows 8.1+
   // Falls back to GetVersionEx if RtlGetVersion is unavailable
   if (!GetRealVersionInfo(&osvi))
   {
      OSVERSIONINFOEX osviLegacy;
      ZeroMemory(&osviLegacy, sizeof(OSVERSIONINFOEX));
      osviLegacy.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
      if (!GetVersionEx((OSVERSIONINFO *)&osviLegacy))
      {
         osviLegacy.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
         if (!GetVersionEx((OSVERSIONINFO *)&osviLegacy))
            return ver;
      }
      // Copy to wide struct
      ZeroMemory(&osvi, sizeof(osvi));
      osvi.dwMajorVersion = osviLegacy.dwMajorVersion;
      osvi.dwMinorVersion = osviLegacy.dwMinorVersion;
      osvi.dwBuildNumber = osviLegacy.dwBuildNumber;
      osvi.dwPlatformId = osviLegacy.dwPlatformId;
      osvi.wServicePackMajor = osviLegacy.wServicePackMajor;
      osvi.wServicePackMinor = osviLegacy.wServicePackMinor;
      osvi.wSuiteMask = osviLegacy.wSuiteMask;
      osvi.wProductType = osviLegacy.wProductType;
   }

   ver.major = osvi.dwMajorVersion;
   ver.minor = osvi.dwMinorVersion;
   ver.sp = osvi.wServicePackMajor;
   ver.build = osvi.dwBuildNumber;

	switch (osvi.dwPlatformId)
   {// Test for the Windows NT product family.
      case VER_PLATFORM_WIN32_NT:

		 // Windows 10 / 11 / Server 2016+
		 if ( osvi.dwMajorVersion == 10)
		 {
			if ( osvi.wProductType == VER_NT_WORKSTATION )
			{
				if ( osvi.dwBuildNumber >= 22000 )
					ver.ver = WND_ELEVEN;
				else
					ver.ver = WND_TEN;
			}
			else
			{
				if ( osvi.dwBuildNumber >= 26100 )
					ver.ver = WND_SERVER2025;
				else if ( osvi.dwBuildNumber >= 20348 )
					ver.ver = WND_SERVER2022;
				else if ( osvi.dwBuildNumber >= 17763 )
					ver.ver = WND_SERVER2019;
				else
					ver.ver = WND_SERVER2016;
			}

			// Modern edition detection via GetProductInfo
			typedef BOOL (WINAPI *pfnGetProductInfo)(DWORD, DWORD, DWORD, DWORD, PDWORD);
			pfnGetProductInfo pGetProductInfo = (pfnGetProductInfo)
				GetProcAddress(GetModuleHandle(_T("kernel32.dll")), "GetProductInfo");
			if (pGetProductInfo)
			{
				DWORD dwProductType = 0;
				pGetProductInfo(osvi.dwMajorVersion, osvi.dwMinorVersion, 0, 0, &dwProductType);
				switch (dwProductType)
				{
				case 0x00000065: // PRODUCT_CORE (Home)
				case 0x00000063: // PRODUCT_CORE_SINGLELANGUAGE
				case 0x00000062: // PRODUCT_CORE_N
				case 0x00000064: // PRODUCT_CORE_COUNTRYSPECIFIC
					ver.type = WND_HOME; break;
				case 0x00000030: // PRODUCT_PROFESSIONAL
				case 0x00000031: // PRODUCT_PROFESSIONAL_N
					ver.type = WND_PRO; break;
				case 0x000000A1: // PRODUCT_PRO_WORKSTATION
				case 0x000000A2: // PRODUCT_PRO_WORKSTATION_N
					ver.type = WND_PROWKS; break;
				case 0x00000004: // PRODUCT_ENTERPRISE
				case 0x0000001B: // PRODUCT_ENTERPRISE_N
				case 0x00000046: // PRODUCT_ENTERPRISE_E
					ver.type = WND_ENT; break;
				case 0x00000079: // PRODUCT_EDUCATION
				case 0x0000007A: // PRODUCT_EDUCATION_N
					ver.type = WND_EDU; break;
				case 0x00000007: // PRODUCT_STANDARD_SERVER
				case 0x0000000D: // PRODUCT_STANDARD_SERVER_CORE
					ver.type = WND_SE; break;
				case 0x00000008: // PRODUCT_DATACENTER_SERVER
				case 0x0000000C: // PRODUCT_DATACENTER_SERVER_CORE
					ver.type = WND_DE; break;
				}
			}
		 }
		 // Windows 8.1 / Server 2012 R2
		 else if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3 )
		 {
			if ( osvi.wProductType == VER_NT_WORKSTATION )
				ver.ver = WND_81;
			else ver.ver = WND_SERVER2012R2;
		 }
		 // Windows 8 / Server 2012
		 else if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2 )
		 {
			if ( osvi.wProductType == VER_NT_WORKSTATION )
				ver.ver = WND_8;
			else ver.ver = WND_SERVER2012;
		 }
		 // Windows 7 / Vista / Server 2008
		 else if ( osvi.dwMajorVersion == 6)
		 {
			 if ( osvi.dwMinorVersion == 0 )
				if ( osvi.wProductType == VER_NT_WORKSTATION )
					ver.ver = WND_VISTA;
				else ver.ver = WND_SERVER2008;
			if ( osvi.dwMinorVersion == 1 )
				if ( osvi.wProductType == VER_NT_WORKSTATION )
					ver.ver = WND_SEVEN;
				else ver.ver = WND_SERVER2008R2;
		 }
         if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
			ver.ver = WND_SERVER2003;

         if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
            ver.ver = WND_XP;

         if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
            ver.ver = WND_2K;

         if ( osvi.dwMajorVersion <= 4 )
			 ver.ver = WND_NT;

         // Legacy edition detection via wSuiteMask (for pre-Win10)
		 if ( osvi.dwMajorVersion < 10 )
         {
            // Test for the workstation type.
			if ( osvi.wProductType == VER_NT_WORKSTATION )
            {
               if( osvi.dwMajorVersion == 4 )
				   ver.type = WND_WKS4;
               else if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
					ver.type = WND_HE;
               else
				   ver.type = WND_PE;
            }
// Test for the server type.
            else if ( osvi.wProductType == VER_NT_SERVER || osvi.wProductType == VER_NT_DOMAIN_CONTROLLER )
            {
               if( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
               {
                  if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
					  ver.type = WND_DE;
                  else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
					  ver.type = WND_EE;
                  else if ( osvi.wSuiteMask == VER_SUITE_BLADE )
                     ver.type = WND_WE;
                  else
					  ver.type = WND_SE;
				}
				else if( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
				{
					if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
					  ver.type = WND_DS;
					else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
					  ver.type = WND_AS;
					else
					  ver.type = WND_S;
				}
				else  // Windows NT 4.0
				{
					if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
						ver.type = WND_4EE;
					else
						ver.type = WND_4S;
				}
			}
		 }
// Display service pack (if any) and build number.
        if( osvi.dwMajorVersion == 4 &&
             lstrcmpi( osvi.szCSDVersion, L"Service Pack 6" ) == 0 )
        {
            HKEY hKey;
            LONG lRet;

            // Test for SP6 versus SP6a.
            lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009"), 0, KEY_QUERY_VALUE, &hKey );
            if( lRet == ERROR_SUCCESS )
			{
				ver.sp = 7; // sp6a
				ver.build = osvi.dwBuildNumber & 0xFFFF;
			}
            else // Windows NT 4.0 prior to SP6a
				ver.build = osvi.dwBuildNumber & 0xFFFF;

            RegCloseKey( hKey );
         }
         else // Windows NT 3.51 and earlier or Windows 2000 and later
			ver.build = osvi.dwBuildNumber & 0xFFFF;
         break;

      // Test for the Windows 95 product family.
      case VER_PLATFORM_WIN32_WINDOWS:
         if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
         {
			ver.ver = WND_95;
            if ( osvi.szCSDVersion[1] == L'C' || osvi.szCSDVersion[1] == L'B' )
			      ver.type = WND_OSR2;
         }

         if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
         {
			ver.ver = WND_98;
             if ( osvi.szCSDVersion[1] == L'A' )
				 ver.type = WND_OSR1;
         }

         if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
			 ver.type = WND_ME;
         break;

      case VER_PLATFORM_WIN32s: ver.ver = WND_32S;
         break;
   }
   return ver;
}

typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
CString GetSystemVersionName(VERSION &ver)
{
	SYSTEM_INFO si; PGNSI pGNSI;
	CString str;
	// Use GetProcAddress to avoid load issues on Windows 2000
	pGNSI = (PGNSI) GetProcAddress(GetModuleHandle(_T("kernel32.dll")),
		"GetNativeSystemInfo");
	if(NULL != pGNSI)
	pGNSI(&si);
	switch (ver.ver)
	{
		case 0 : break;
		case WND_SERVER2025 : str = _T("Microsoft Windows Server 2025 "); break;
		case WND_SERVER2022 : str = _T("Microsoft Windows Server 2022 "); break;
		case WND_SERVER2019 : str = _T("Microsoft Windows Server 2019 "); break;
		case WND_SERVER2016 : str = _T("Microsoft Windows Server 2016 "); break;
		case WND_ELEVEN : str = _T("Microsoft Windows 11 "); break;
		case WND_TEN : str = _T("Microsoft Windows 10 "); break;
		case WND_SERVER2012R2 : str = _T("Microsoft Windows Server 2012 R2 "); break;
		case WND_SERVER2012 : str = _T("Microsoft Windows Server 2012 "); break;
		case WND_SERVER2008R2 : str = _T("Microsoft Windows Server 2008 R2 "); break;
		case WND_SERVER2008 : str = _T("Microsoft Windows Server 2008 "); break;
		case WND_SEVEN : str = _T("Microsoft Windows 7 "); break;
		case WND_81 : str = _T("Microsoft Windows 8.1 "); break;
		case WND_8 : str = _T("Microsoft Windows 8 "); break;

		case WND_VISTA: str = _T("Microsoft Windows Vista "); break;
		case WND_SERVER2003: str = _T("Microsoft Windows Server 2003 "); break;
		case WND_XP: str = _T("Microsoft Windows XP ");break;
		case WND_2K: str = _T("Microsoft Windows 2000 ");break;
		case WND_NT: str = _T("Microsoft Windows NT ");break;
		case WND_95: str = _T("Microsoft Windows 95 ");break;
		case WND_98: str = _T("Microsoft Windows 98 ");break;
		case WND_ME: str = _T("Microsoft Windows Millennium Edition\n");break;
		case WND_32S: str = _T("Microsoft Win32s\n");break;
	}
#ifndef PROCESSOR_ARCHITECTURE_ARM
#define PROCESSOR_ARCHITECTURE_ARM 5
#endif
#ifndef PROCESSOR_ARCHITECTURE_ARM64
#define PROCESSOR_ARCHITECTURE_ARM64 12
#endif
	if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA64 )
		str += _T("Itanium-based Systems ");
	else if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
		str += _T("x64 ");
	else if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_ARM64 )
		str += _T("ARM64 ");
	else if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_ARM )
		str += _T("ARM ");

	switch (ver.type)
	{
		case WND_WKS4: str += _T("Workstation 4.0 "); break;
		case WND_HE:  str += _T("Home Edition ");break;
		case WND_PE: str += _T("Professional Edition ");break;
		case WND_PRO: str += _T("Pro ");break;
		case WND_PROWKS: str += _T("Pro for Workstations ");break;
		case WND_HOME: str += _T("Home ");break;
		case WND_ENT: str += _T("Enterprise ");break;
		case WND_EDU: str += _T("Education ");break;
		case WND_DE: str += _T("Datacenter Edition ");break;
		case WND_EE: str += _T("Enterprise Edition ");break;
		case WND_WE: str += _T("Web Edition ");break;
		case WND_SE: str += _T("Standard Edition ");break;
		case WND_DS: str += _T("Datacenter Server ");break;
		case WND_AS: str += _T("Advanced Server");break;
		case WND_S:  str += _T("Server ");break;
		case WND_4EE: str += _T("Server 4.0, Enterprise Edition ");break;
		case WND_4S: str += _T("Server 4.0 ");break;
		case WND_OSR2: str += _T("OSR2 ");break;
		case WND_OSR1: str += _T("SE ");break;
		case WND_WKS3: str += _T("Workstation "); break;

	}
CString strtp;
	if ( ver.ver == WND_NT)
	{
		if (ver.sp == 7)
			strtp.Format( _T("Service Pack 6a (Build %d)\n"), ver.build );
		else strtp.Format( _T("Service Pack %d (Build %d)\n"), ver.sp, ver.build );
	}
	else if ( ver.major >= 10 )
	{
		// Read DisplayVersion (e.g. "25H2") and UBR (e.g. 8106) from registry
		CString szDisplayVer;
		DWORD dwUBR = 0;
		{
			CRegKey key;
			if ( key.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), KEY_READ) == ERROR_SUCCESS )
			{
				TCHAR szVal[64] = {0};
				DWORD dwCount = 64;
				if ( key.QueryValue(szVal, _T("DisplayVersion"), &dwCount) == ERROR_SUCCESS && szVal[0] )
					szDisplayVer = szVal;
				DWORD dwSize = sizeof(DWORD);
				RegQueryValueEx(key, _T("UBR"), NULL, NULL, (LPBYTE)&dwUBR, &dwSize);
				key.Close();
			}
		}
		if ( !szDisplayVer.IsEmpty() )
		{
			if (dwUBR > 0)
				strtp.Format( _T("Version %s (build %d.%d)"), (LPCTSTR)szDisplayVer, ver.build, dwUBR );
			else
				strtp.Format( _T("Version %s (build %d)"), (LPCTSTR)szDisplayVer, ver.build );
		}
		else
		{
			if (dwUBR > 0)
				strtp.Format( _T("Version %d.%d (build %d.%d)"), ver.major, ver.minor, ver.build, dwUBR );
			else
				strtp.Format( _T("Version %d.%d (build %d)"), ver.major, ver.minor, ver.build );
		}
	}
	else {
		if (ver.sp > 0)
			strtp.Format( _T("Version %d.%d (build %d) Service pack %d \n"), ver.major, ver.minor, ver.build, ver.sp);
		else
			strtp.Format( _T("Version %d.%d (build %d)"), ver.major, ver.minor, ver.build);

	}
	str += strtp;
   return str;
}
