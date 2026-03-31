//==========================================
// API Set DLL resolution utilities
// Handles virtual "api-ms-win-*" and "ext-ms-win-*" DLLs
//==========================================
#ifndef __APISETRESOLVE_H__
#define __APISETRESOLVE_H__

#include <windows.h>
#include <tchar.h>

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

#endif
