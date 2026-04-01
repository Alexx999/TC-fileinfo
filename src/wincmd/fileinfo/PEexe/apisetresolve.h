//==========================================
// API Set DLL resolution utilities
// Handles virtual "api-ms-win-*" and "ext-ms-win-*" DLLs
//==========================================
#ifndef __APISETRESOLVE_H__
#define __APISETRESOLVE_H__

#include <windows.h>
#include <tchar.h>
#include <map>

// Returns TRUE if the DLL name is an API Set contract name
// (starts with "api-" or "ext-")
BOOL IsApiSetName(LPCTSTR pszDllName);

// Attempts to resolve an API Set name to its real host DLL path.
// Returns TRUE and fills szResolvedPath if successful.
// Returns FALSE if resolution fails.
BOOL ResolveApiSetDll(LPCTSTR pszApiSetName, LPTSTR szResolvedPath, DWORD cchResolvedPath);

// Unified DLL path resolution: tries API Set resolution first (if applicable),
// then falls back to standard SearchPath.
// Returns TRUE and fills szResolvedPath if the DLL was found/resolved.
// Returns FALSE if the DLL is genuinely not found.
BOOL ResolveDllPath(LPCTSTR pszDllName, LPTSTR szResolvedPath, DWORD cchResolvedPath);

// Case-insensitive comparator for CString map keys
struct CStringNoCaseLess {
	bool operator()(const CString& a, const CString& b) const {
		return a.CompareNoCase(b) < 0;
	}
};

// Caches DLL path resolution results (both hits and misses) to avoid
// redundant SearchPath / LoadLibraryEx calls during a single analysis.
class CDllPathCache {
	std::map<CString, CString, CStringNoCaseLess> m_cache;
public:
	BOOL Resolve(LPCTSTR pszDllName, LPTSTR szResolvedPath, DWORD cchResolvedPath);
};

// Caches HINSTANCE handles from LoadLibraryEx so each unique DLL is loaded
// once and reused for all GetProcAddress validation calls.
class CDllHandleCache {
	std::map<CString, HINSTANCE, CStringNoCaseLess> m_cache;
public:
	~CDllHandleCache();
	// Releases all cached handles and clears the cache.
	void Clear();
	// Returns a cached HINSTANCE, or loads the DLL and caches it.
	// Returns NULL if the DLL cannot be loaded.
	HINSTANCE GetHandle(LPCTSTR pszFullPath, BOOL bAsDataFile);
};

#endif
