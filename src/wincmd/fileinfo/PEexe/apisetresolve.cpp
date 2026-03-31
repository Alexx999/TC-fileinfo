//==========================================
// API Set DLL resolution utilities
// Handles virtual "api-ms-win-*" and "ext-ms-win-*" DLLs
//==========================================
#include "stdafx.h"
#include "apisetresolve.h"

BOOL IsApiSetName(LPCTSTR pszDllName)
{
	if (!pszDllName)
		return FALSE;
	// API Set contract names start with "api-" or "ext-"
	return (_tcsnicmp(pszDllName, _T("api-"), 4) == 0) ||
	       (_tcsnicmp(pszDllName, _T("ext-"), 4) == 0);
}

BOOL ResolveApiSetDll(LPCTSTR pszApiSetName, LPTSTR szResolvedPath, DWORD cchResolvedPath)
{
	// Let the Windows loader resolve the API Set name internally.
	// DONT_RESOLVE_DLL_REFERENCES avoids running DllMain but still
	// triggers the loader's API Set resolution.
	HMODULE hMod = LoadLibraryEx(pszApiSetName, NULL, DONT_RESOLVE_DLL_REFERENCES);
	if (!hMod)
		return FALSE;

	DWORD len = GetModuleFileName(hMod, szResolvedPath, cchResolvedPath);
	FreeLibrary(hMod);

	return (len > 0 && len < cchResolvedPath);
}

BOOL ResolveDllPath(LPCTSTR pszDllName, LPTSTR szResolvedPath, DWORD cchResolvedPath)
{
	// For API Set names, always use loader-based resolution to get
	// the real host DLL. SearchPath would find physical forwarder stubs
	// sitting on PATH, which is not what we want.
	if (IsApiSetName(pszDllName))
		return ResolveApiSetDll(pszDllName, szResolvedPath, cchResolvedPath);

	// Standard DLLs: use SearchPath
	LPTSTR pszDontCare;
	if (SearchPath(0, pszDllName, 0, cchResolvedPath, szResolvedPath, &pszDontCare))
		return TRUE;

	return FALSE;
}
