#pragma once

// Dark mode color palette and helpers for TC lister plugins
// Used when Total Commander signals lcp_darkmode / lcp_darkmodenative

struct DarkModeColors {
	COLORREF crBackground;
	COLORREF crBackgroundAlt;
	COLORREF crText;
	COLORREF crLink;
	COLORREF crActiveLink;
	COLORREF crVisitedLink;
	COLORREF crHoverLink;
};

inline DarkModeColors GetDarkColors() {
	DarkModeColors dc;
	dc.crBackground    = RGB(30, 30, 30);
	dc.crBackgroundAlt = RGB(43, 43, 43);
	dc.crText          = RGB(220, 220, 220);
	dc.crLink          = RGB(100, 149, 237);   // Cornflower blue
	dc.crActiveLink    = RGB(0, 200, 200);
	dc.crVisitedLink   = RGB(180, 130, 255);
	dc.crHoverLink     = RGB(255, 100, 100);
	return dc;
}

inline DarkModeColors GetLightColors() {
	DarkModeColors dc;
	dc.crBackground    = ::GetSysColor(COLOR_WINDOW);
	dc.crBackgroundAlt = ::GetSysColor(COLOR_BTNFACE);
	dc.crText          = ::GetSysColor(COLOR_WINDOWTEXT);
	dc.crLink          = RGB(0, 0, 255);
	dc.crActiveLink    = RGB(0, 128, 128);
	dc.crVisitedLink   = RGB(128, 0, 128);
	dc.crHoverLink     = RGB(255, 0, 0);
	return dc;
}

// Undocumented uxtheme.dll ordinal 133: per-window dark mode opt-in
// Available on Windows 10 1809+ (build 17763+)
typedef BOOL (WINAPI *fnAllowDarkModeForWindow)(HWND hWnd, BOOL allow);

inline BOOL DarkMode_AllowForWindow(HWND hWnd, BOOL bAllow)
{
	static fnAllowDarkModeForWindow fn = nullptr;
	static bool bLoaded = false;
	if (!bLoaded) {
		bLoaded = true;
		HMODULE hUxtheme = ::GetModuleHandle(_T("uxtheme.dll"));
		if (hUxtheme)
			fn = (fnAllowDarkModeForWindow)::GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133));
	}
	if (fn) return fn(hWnd, bAllow);
	return FALSE;
}
