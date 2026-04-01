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
	// For API Set names, prefer loader-based resolution to get the real
	// host DLL rather than a physical forwarder stub on PATH.
	if (IsApiSetName(pszDllName))
	{
		if (ResolveApiSetDll(pszDllName, szResolvedPath, cchResolvedPath))
			return TRUE;
		// API Set schema has no mapping for this name on this Windows edition.
		// Fall through to SearchPath — some API sets have physical forwarder
		// stubs in System32 that can still be loaded for function testing.
	}

	// Standard DLLs (or API set fallback): use SearchPath
	LPTSTR pszDontCare;
	if (SearchPath(0, pszDllName, 0, cchResolvedPath, szResolvedPath, &pszDontCare))
		return TRUE;

	return FALSE;
}

//------------------------------------------------------------------
// CDllPathCache: caches ResolveDllPath results (hits and misses)
//------------------------------------------------------------------
BOOL CDllPathCache::Resolve(LPCTSTR pszDllName, LPTSTR szResolvedPath, DWORD cchResolvedPath)
{
	CString key(pszDllName);
	auto it = m_cache.find(key);
	if (it != m_cache.end()) {
		if (it->second.IsEmpty())
			return FALSE;  // cached miss
		lstrcpyn(szResolvedPath, it->second, cchResolvedPath);
		return TRUE;
	}
	// Not in cache — do the real resolution
	BOOL found = ResolveDllPath(pszDllName, szResolvedPath, cchResolvedPath);
	m_cache[key] = found ? CString(szResolvedPath) : CString();
	return found;
}

//------------------------------------------------------------------
// CDllHandleCache: caches LoadLibraryEx handles
//------------------------------------------------------------------
CDllHandleCache::~CDllHandleCache()
{
	Clear();
}

void CDllHandleCache::Clear()
{
	for (auto& pair : m_cache)
		if (pair.second)
			FreeLibrary(pair.second);
	m_cache.clear();
}

HINSTANCE CDllHandleCache::GetHandle(LPCTSTR pszFullPath, BOOL bAsDataFile)
{
	CString key(pszFullPath);
	auto it = m_cache.find(key);
	if (it != m_cache.end())
		return it->second;  // may be NULL (cached load failure)

	HINSTANCE h = LoadLibraryEx(pszFullPath, NULL,
		bAsDataFile ? LOAD_LIBRARY_AS_DATAFILE : DONT_RESOLVE_DLL_REFERENCES);
	m_cache[key] = h;
	return h;
}
