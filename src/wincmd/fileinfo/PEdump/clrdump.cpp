//==========================================
// clrdump.cpp - .NET / CLR managed-assembly dumper
// Fills m_clr (CLR Header) text and m_clr_deps (AssemblyRef tree).
// See docs/dotnet-reference.md for on-disk format references.
//==========================================
#include "stdafx.h"
#include "clrdump.h"
#include "clrmeta.h"
#include <wincrypt.h>
#pragma comment(lib, "advapi32.lib")

// -------- Small rendering helpers ------------------------------------------
static void AppendHex32(CStringA &s, const char* label, DWORD v)
{
	CStringA line;
	line.Format("%-25s 0x%08X\r\n", label, v);
	s += line;
}

static void AppendDecorated(CStringA &s, const char* label, const char* fmt, ...)
{
	CStringA value;
	va_list ap;
	va_start(ap, fmt);
	value.FormatV(fmt, ap);
	va_end(ap);
	CStringA line;
	line.Format("%-25s %s\r\n", label, (LPCSTR) value);
	s += line;
}

static void AppendDataDir(CStringA &s, const char* label, const IMAGE_DATA_DIRECTORY &dd)
{
	CStringA line;
	if (dd.VirtualAddress == 0 && dd.Size == 0)
		line.Format("%-25s (empty)\r\n", label);
	else
		line.Format("%-25s RVA=0x%08X  Size=%u\r\n",
		            label, (unsigned) dd.VirtualAddress, (unsigned) dd.Size);
	s += line;
}

// -------- COR20 Flags decoding ---------------------------------------------
static CStringA DecodeCor20Flags(DWORD flags)
{
	CStringA s;
	bool first = true;
	#define APPEND(_label) do { if (!first) s += " | "; s += _label; first = false; } while (0)
	if (flags & COMIMAGE_FLAGS_ILONLY)            APPEND("ILONLY");
	if (flags & COMIMAGE_FLAGS_32BITREQUIRED)     APPEND("32BITREQUIRED");
	if (flags & COMIMAGE_FLAGS_IL_LIBRARY)        APPEND("IL_LIBRARY");
	if (flags & COMIMAGE_FLAGS_STRONGNAMESIGNED)  APPEND("STRONGNAMESIGNED");
	if (flags & COMIMAGE_FLAGS_NATIVE_ENTRYPOINT) APPEND("NATIVE_ENTRYPOINT");
	if (flags & COMIMAGE_FLAGS_TRACKDEBUGDATA)    APPEND("TRACKDEBUGDATA");
	if (flags & COMIMAGE_FLAGS_32BITPREFERRED)    APPEND("32BITPREFERRED");
	#undef APPEND
	if (first) s = "0";
	return s;
}

// 32-bit mode quadrant per corhdr.h lines 95-102. The 32BITREQUIRED / 32BITPREFERRED
// pair encodes four combinations — render the named mode, not the raw bits.
static const char* Decode32BitMode(DWORD flags)
{
	bool req = (flags & COMIMAGE_FLAGS_32BITREQUIRED)  != 0;
	bool pre = (flags & COMIMAGE_FLAGS_32BITPREFERRED) != 0;
	if (!req && !pre) return "AnyCPU (or native x86 via MachineType)";
	if (!req &&  pre) return "illegal / reserved combination";
	if ( req && !pre) return "x86 (32-bit required)";
	/*  req &&  pre */ return "AnyCPU (32-bit preferred)";
}

// -------- COR20 header dump ------------------------------------------------
static void DumpCor20Header(CStringA &s, const IMAGE_COR20_HEADER* cor20)
{
	s += "COR20 Header (IMAGE_COR20_HEADER)\r\n";
	s += "---------------------------------\r\n";
	CStringA line;
	AppendDecorated(s, "Header size (cb):",     "%u bytes", (unsigned) cor20->cb);
	AppendDecorated(s, "Runtime version:",      "%u.%u",
	                (unsigned) cor20->MajorRuntimeVersion,
	                (unsigned) cor20->MinorRuntimeVersion);
	AppendDataDir (s, "MetaData:",              cor20->MetaData);

	AppendHex32   (s, "Flags:",                 cor20->Flags);
	AppendDecorated(s, "Flags decoded:",        "%s", (LPCSTR) DecodeCor20Flags(cor20->Flags));
	AppendDecorated(s, "32-bit mode:",          "%s", Decode32BitMode(cor20->Flags));

	if (cor20->Flags & COMIMAGE_FLAGS_NATIVE_ENTRYPOINT)
	{
		AppendDecorated(s, "EntryPoint RVA:",   "0x%08X (native)", (unsigned) cor20->EntryPointRVA);
	}
	else
	{
		DWORD tok = cor20->EntryPointToken;
		if (tok == 0)
			AppendDecorated(s, "EntryPoint token:", "(none)");
		else
			AppendDecorated(s, "EntryPoint token:", "0x%08X  (table 0x%02X, RID %u)",
			                (unsigned) tok, (unsigned)(tok >> 24), (unsigned)(tok & 0x00FFFFFF));
	}

	AppendDataDir(s, "Resources:",              cor20->Resources);
	AppendDataDir(s, "StrongNameSignature:",    cor20->StrongNameSignature);
	AppendDataDir(s, "CodeManagerTable:",       cor20->CodeManagerTable);
	AppendDataDir(s, "VTableFixups:",           cor20->VTableFixups);
	AppendDataDir(s, "ExportAddressTableJumps:",cor20->ExportAddressTableJumps);
	AppendDataDir(s, "ManagedNativeHeader:",    cor20->ManagedNativeHeader);
	s += "\r\n";
}

// -------- Raw bytes -> UTF-8 normalization ---------------------------------
// Debug-directory strings (CodeView PDB filename, PdbChecksum algorithm name)
// are raw byte sequences whose encoding depends on the toolchain:
//   - Classic MSF PDB: CodeView path is ANSI at build-time (system ACP).
//   - Portable PDB:    CodeView path is UTF-8 per dotnet/runtime PE-COFF.md.
// The main dump pipeline expects UTF-8 bytes end-to-end (so the final
// MultiByteToWideChar(CP_UTF8) doesn't mangle multi-byte ACP sequences).
// Normalize any non-metadata string at the read boundary:
//
//   assumeUtf8 = true  -> validate strict UTF-8; if invalid, treat as ACP
//   assumeUtf8 = false -> always treat as ACP
//
// Fallback of last resort: embed raw bytes so the user sees *something*
// instead of silent loss — mangling in display is preferable to a blank field.
static CStringA BytesToUtf8(const char* bytes, int cb, bool assumeUtf8)
{
	CStringA out;
	if (!bytes || cb <= 0) return out;

	if (assumeUtf8) {
		int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes, cb, NULL, 0);
		if (wlen > 0) {
			// Input bytes already are valid UTF-8 — pass through unchanged.
			out.SetString(bytes, cb);
			return out;
		}
	}

	// Decode as ACP, re-encode as UTF-8.
	int wlen = MultiByteToWideChar(CP_ACP, 0, bytes, cb, NULL, 0);
	if (wlen <= 0) { out.SetString(bytes, cb); return out; }
	CStringW w;
	wchar_t* wbuf = w.GetBuffer(wlen);
	MultiByteToWideChar(CP_ACP, 0, bytes, cb, wbuf, wlen);
	w.ReleaseBuffer(wlen);

	int ulen = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR) w, wlen, NULL, 0, NULL, NULL);
	if (ulen <= 0) { out.SetString(bytes, cb); return out; }
	char* ubuf = out.GetBuffer(ulen);
	WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR) w, wlen, ubuf, ulen, NULL, NULL);
	out.ReleaseBuffer(ulen);
	return out;
}

// -------- UTF-8 bytes -> wide transcode ------------------------------------
// The #Strings heap and all string literals in this dumper are UTF-8 bytes
// (ECMA-335 §II.24.2.3 for metadata; ASCII literals are valid UTF-8 by
// construction). All renderers accumulate UTF-8 into CStringA; the conversion
// to wide happens exactly once at the CLR-tab boundary so the RichEdit can
// display chars outside the system ANSI codepage.
static CStringW Utf8ToWide(LPCSTR utf8, int bytes = -1)
{
	CStringW out;
	if (!utf8 || (bytes == 0)) return out;
	int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, bytes, NULL, 0);
	if (wlen <= 0) return out;
	wchar_t* wbuf = out.GetBuffer(wlen);
	int wr = MultiByteToWideChar(CP_UTF8, 0, utf8, bytes, wbuf, wlen);
	// When bytes == -1, MultiByteToWideChar returns length including terminating NUL;
	// ReleaseBuffer(-1) = strlen the buffer. For explicit length (bytes), wr = written count.
	out.ReleaseBuffer(bytes == -1 ? (wr > 0 ? wr - 1 : 0) : wr);
	return out;
}

// -------- PublicKeyToken derivation ----------------------------------------
// Computes SHA1(publicKey) and returns the last 8 bytes REVERSED (storage form
// for AssemblyRef.PublicKeyOrToken when !afPublicKey), which is also what the
// display form ("b03f5f7f11d50a3a") encodes. See reference doc §9.
// Returns true on success.
static bool ComputePublicKeyToken(const BYTE* publicKey, DWORD publicKeySize, BYTE outToken[8])
{
	if (!publicKey || publicKeySize == 0) return false;

	HCRYPTPROV hProv = 0;
	if (!CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		return false;

	HCRYPTHASH hHash = 0;
	bool ok = false;
	if (CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
		if (CryptHashData(hHash, publicKey, publicKeySize, 0)) {
			BYTE digest[20];
			DWORD cb = sizeof(digest);
			if (CryptGetHashParam(hHash, HP_HASHVAL, digest, &cb, 0) && cb == 20) {
				// Take last 8 bytes. Convention: public-key-token bytes in storage order
				// are digest[19]..digest[12] — i.e. the tail, reversed.
				for (int i = 0; i < 8; ++i)
					outToken[i] = digest[19 - i];
				ok = true;
			}
		}
		CryptDestroyHash(hHash);
	}
	CryptReleaseContext(hProv, 0);
	return ok;
}

// Render the 8-byte token as lowercase hex in display order (e.g. "b03f5f7f11d50a3a").
// Display order = storage order reversed, per §9.
static CStringA FormatPublicKeyToken(const BYTE token[8])
{
	CStringA s;
	for (int i = 7; i >= 0; --i) {
		CStringA byteStr;
		byteStr.Format("%02x", token[i]);
		s += byteStr;
	}
	return s;
}

// -------- CorAssemblyFlags decoding (for Assembly.Flags / AssemblyRef.Flags) --
// Source: corhdr.h:L745-L772 and reference doc §9 table.
#ifndef afPublicKey
	#define afPublicKey                         0x0001
	#define afPA_Mask                           0x0070
	#define afPA_Specified                      0x0080
	#define afRetargetable                      0x0100
	#define afContentType_Mask                  0x0E00
	#define afContentType_Default               0x0000
	#define afContentType_WindowsRuntime        0x0200
#endif

static const char* DecodeProcessorArch(DWORD flags)
{
	switch (flags & afPA_Mask) {
		case 0x0000: return "None";
		case 0x0010: return "MSIL";
		case 0x0020: return "x86";
		case 0x0030: return "IA64";
		case 0x0040: return "AMD64";
		case 0x0050: return "ARM";
		case 0x0060: return "ARM64";
		case 0x0070: return "NoPlatform";
	}
	return "?";
}

// -------- HashAlgorithm decoding (Assembly.HashAlgId) ----------------------
// Complete list per reference doc §9. Renders unknown values as raw hex.
static CStringA DecodeHashAlgId(DWORD alg)
{
	CStringA s;
	switch (alg) {
		case 0x00000000: s = "None";   return s;
		case 0x00008003: s = "MD5";    return s;
		case 0x00008004: s = "SHA1";   return s;
		case 0x0000800C: s = "SHA256"; return s;
		case 0x0000800D: s = "SHA384"; return s;
		case 0x0000800E: s = "SHA512"; return s;
	}
	s.Format("0x%08X", alg);
	return s;
}

// -------- ManagedNativeHeader summary (§11) --------------------------------
// Classifies the ManagedNativeHeader data directory: R2R (plus composite flag),
// NGen, or unknown. No deep decoding — one-liner only.
static void DumpManagedNativeHeader(CStringA &s, PE_EXE &pe, const IMAGE_COR20_HEADER* cor20)
{
	DWORD rva = cor20->ManagedNativeHeader.VirtualAddress;
	DWORD sz  = cor20->ManagedNativeHeader.Size;
	if (rva == 0 || sz == 0)
		return;  // Pure IL — silent.

	s += "Native Image\r\n";
	s += "------------\r\n";

	// Validate the FULL declared range, not just the starting offset —
	// ManagedNativeHeader is an attacker-controlled (RVA, Size) pair.
	const BYTE* p = ClrSafeRvaRange(pe, rva, sz);
	if (!p || sz < 4) {
		AppendDecorated(s, "ManagedNativeHeader:", "%u bytes at RVA 0x%08X (payload not readable or out of bounds)",
		                (unsigned) sz, (unsigned) rva);
		s += "\r\n";
		return;
	}

	DWORD sig = (DWORD) p[0] | ((DWORD) p[1] << 8) | ((DWORD) p[2] << 16) | ((DWORD) p[3] << 24);

	// 'RTR\0' = 0x00525452 → ReadyToRun (composite and non-composite share this magic).
	if (sig == 0x00525452) {
		CStringA flagsStr;
		if (sz >= 12) {
			DWORD flags = (DWORD) p[8] | ((DWORD) p[9] << 8) | ((DWORD) p[10] << 16) | ((DWORD) p[11] << 24);
			if (flags & 0x02) flagsStr += ", composite";
			if (flags & 0x20) flagsStr += ", component";
		}
		AppendDecorated(s, "ReadyToRun:", "%u bytes at RVA 0x%08X%s",
		                (unsigned) sz, (unsigned) rva, (LPCSTR) flagsStr);
		s += "\r\n";
		return;
	}

	// Legacy NGen — signature varies; just report size + signature hex.
	AppendDecorated(s, "ManagedNativeHeader:", "%u bytes at RVA 0x%08X  signature=0x%08X",
	                (unsigned) sz, (unsigned) rva, (unsigned) sig);
	s += "\r\n";
}

// -------- Debug-directory summary (§10) ------------------------------------
// Older SDKs don't define these — fall back to literal integers per the
// dotnet/runtime PE-COFF.md spec.
#ifndef IMAGE_DEBUG_TYPE_EMBEDDED_PORTABLE_PDB
	#define IMAGE_DEBUG_TYPE_EMBEDDED_PORTABLE_PDB 17
#endif
#ifndef IMAGE_DEBUG_TYPE_PDBCHECKSUM
	#define IMAGE_DEBUG_TYPE_PDBCHECKSUM 19
#endif

static void FormatGuid(CStringA &s, const GUID &g)
{
	s.Format("%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
	         g.Data1, g.Data2, g.Data3,
	         g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3],
	         g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);
}

static void DumpDebugDirectorySummary(CStringA &s, PE_EXE &pe)
{
	// Locate IMAGE_DIRECTORY_ENTRY_DEBUG (index 6). The directory Size is
	// attacker-controlled — validate the entire declared range fits in the file.
	DWORD dirRVA  = pe.GetDataDirectoryEntryRVA (IMAGE_DIRECTORY_ENTRY_DEBUG);
	DWORD dirSize = pe.GetDataDirectoryEntrySize(IMAGE_DIRECTORY_ENTRY_DEBUG);
	if (dirRVA == 0 || dirSize == 0)
		return;  // No debug directory — silently skip (not an error for managed PEs).

	const BYTE* dirBytes = ClrSafeRvaRange(pe, dirRVA, dirSize);
	if (!dirBytes) {
		s += "Debug Info (PDB advertisement)\r\n";
		s += "------------------------------\r\n";
		CStringA line;
		line.Format("(directory at RVA 0x%08X, Size=%u is out of bounds)\r\n\r\n",
		            (unsigned) dirRVA, (unsigned) dirSize);
		s += line;
		return;
	}

	s += "Debug Info (PDB advertisement)\r\n";
	s += "------------------------------\r\n";

	const IMAGE_DEBUG_DIRECTORY* entries = (const IMAGE_DEBUG_DIRECTORY*) dirBytes;
	DWORD nEntries = dirSize / sizeof(IMAGE_DEBUG_DIRECTORY);
	if (nEntries == 0) {
		s += "(empty)\r\n\r\n";
		return;
	}

	bool recognized = false;
	for (DWORD i = 0; i < nEntries; ++i) {
		const IMAGE_DEBUG_DIRECTORY &d = entries[i];

		// Every payload read below routes through ClrSafeRvaRange so the declared
		// SizeOfData must fit entirely inside the mapped file — not just the start.
		switch (d.Type)
		{
		case IMAGE_DEBUG_TYPE_CODEVIEW:
		{
			// CV_INFO_PDB70: RSDS + GUID(16) + Age(u32) + NUL-term filename
			const BYTE* p = ClrSafeRvaRange(pe, d.AddressOfRawData, d.SizeOfData);
			if (!p || d.SizeOfData < 4 + 16 + 4 + 1) {
				AppendDecorated(s, "CodeView:", "(payload not readable or out of bounds)");
				recognized = true;
				break;
			}
			// Signature check: 'RSDS'
			if (p[0] != 'R' || p[1] != 'S' || p[2] != 'D' || p[3] != 'S') {
				AppendDecorated(s, "CodeView:", "(unrecognized signature '%c%c%c%c')",
				                p[0] ? p[0] : '?', p[1] ? p[1] : '?',
				                p[2] ? p[2] : '?', p[3] ? p[3] : '?');
				recognized = true;
				break;
			}
			const GUID* guid = (const GUID*) (p + 4);
			DWORD age = *(const DWORD*) (p + 20);
			const char* pdbName = (const char*) (p + 24);
			DWORD maxName = d.SizeOfData - 24;
			// Ensure NUL inside payload.
			DWORD nameLen = 0;
			while (nameLen < maxName && pdbName[nameLen] != 0) nameLen++;
			if (nameLen >= maxName) {
				AppendDecorated(s, "CodeView:", "(PDB name not NUL-terminated)");
				recognized = true;
				break;
			}

			bool isPortable = (d.MinorVersion == 0x504D);
			CStringA guidStr;
			FormatGuid(guidStr, *guid);

			// PDB filename: UTF-8 per spec for portable, ACP historically for classic.
			CStringA pdbUtf8 = BytesToUtf8(pdbName, (int) nameLen, isPortable);

			if (isPortable) {
				// Per §10: portable PDB ID = (GUID, TimeDateStamp) — Age is always 1.
				AppendDecorated(s, "CodeView:", "Portable PDB  ID=%s-%08X  Pdb=\"%s\"",
				                (LPCSTR) guidStr,
				                (unsigned) d.TimeDateStamp,
				                (LPCSTR) pdbUtf8);
			} else {
				// Classic PDB: (GUID, Age) is the match key.
				AppendDecorated(s, "CodeView:", "Classic PDB   GUID=%s  Age=%u  Pdb=\"%s\"",
				                (LPCSTR) guidStr, (unsigned) age,
				                (LPCSTR) pdbUtf8);
			}
			recognized = true;
			break;
		}

		case IMAGE_DEBUG_TYPE_REPRO:
			AppendDecorated(s, "Reproducible:", "yes (deterministic build; TimeDateStamp is a content hash)");
			recognized = true;
			break;

		case IMAGE_DEBUG_TYPE_EMBEDDED_PORTABLE_PDB:
		{
			// 'MPDB' + UncompressedSize(u32) + deflate-compressed PDB
			const BYTE* p = ClrSafeRvaRange(pe, d.AddressOfRawData, d.SizeOfData);
			if (p && d.SizeOfData >= 8) {
				if (p[0] == 'M' && p[1] == 'P' && p[2] == 'D' && p[3] == 'B') {
					DWORD uncompressed = *(const DWORD*) (p + 4);
					AppendDecorated(s, "Embedded PDB:", "%u bytes compressed -> %u uncompressed (portable)",
					                (unsigned) d.SizeOfData - 8, (unsigned) uncompressed);
				} else {
					AppendDecorated(s, "Embedded PDB:", "%u bytes (unrecognized signature)",
					                (unsigned) d.SizeOfData);
				}
			} else {
				AppendDecorated(s, "Embedded PDB:", "%u bytes (payload not readable or out of bounds)",
				                (unsigned) d.SizeOfData);
			}
			recognized = true;
			break;
		}

		case IMAGE_DEBUG_TYPE_PDBCHECKSUM:
		{
			// NUL-term algorithm name + checksum bytes
			const BYTE* p = ClrSafeRvaRange(pe, d.AddressOfRawData, d.SizeOfData);
			if (p && d.SizeOfData > 1) {
				const char* alg = (const char*) p;
				DWORD maxLen = d.SizeOfData;
				DWORD algLen = 0;
				while (algLen < maxLen && alg[algLen] != 0) algLen++;
				if (algLen < maxLen) {
					DWORD hashLen = d.SizeOfData - (algLen + 1);
					const BYTE* hash = p + algLen + 1;
					CStringA hashHex;
					for (DWORD j = 0; j < hashLen && j < 64; ++j) {
						CStringA byteStr;
						byteStr.Format("%02X", hash[j]);
						hashHex += byteStr;
					}
					if (hashLen > 64) hashHex += "...";
					// Algorithm name is ASCII in practice ("SHA256", "SHA1"); treat as ACP.
					CStringA algUtf8 = BytesToUtf8(alg, (int) algLen, false);
					AppendDecorated(s, "PdbChecksum:", "%s = %s",
					                (LPCSTR) algUtf8, (LPCSTR) hashHex);
				} else {
					AppendDecorated(s, "PdbChecksum:", "(algorithm name not NUL-terminated)");
				}
			} else {
				AppendDecorated(s, "PdbChecksum:", "(payload not readable or out of bounds)");
			}
			recognized = true;
			break;
		}

		default:
			// Silently skip unrecognized entry types (covers types 1/4/8/etc. that aren't CLR-relevant).
			break;
		}
	}

	if (!recognized)
		s += "(present, but no CLR-relevant entries recognized)\r\n";
	s += "\r\n";
}

// -------- Identity block ---------------------------------------------------
// Renders "<Name>, Version=M.m.B.R, Culture=neutral, PublicKeyToken=...".
// `table` = 0x20 (Assembly) or 0x23 (AssemblyRef). `rid` is 1-based.
// Culture interpretation: schema column is `Locale`; render as "Culture".
// Empty Locale -> "neutral".
//
// For PublicKey rendering:
//   - Assembly.PublicKey is always the full key (display a token derived from it).
//   - AssemblyRef.PublicKeyOrToken is a full key when afPublicKey is set, else the 8-byte token.
static CStringA RenderAssemblyIdentity(const ClrView &v, BYTE table, DWORD rid)
{
	CStringA s;
	const BYTE* row;
	if (!ClrGetRow(v, table, rid, &row)) {
		s = "(malformed row)";
		return s;
	}

	// Column layouts (see schema arrays in clrmeta.cpp):
	//   Assembly:    HashAlg, Major, Minor, Build, Rev, Flags, PublicKey(blob), Name(str), Locale(str)
	//   AssemblyRef: Major, Minor, Build, Rev, Flags, PublicKeyOrToken(blob), Name(str), Locale(str), HashValue(blob)
	// AssemblyRef columns 0..7 match Assembly columns 1..8, so use a per-table offset.
	int base;
	if      (table == 0x20) base = 1;    // Assembly: skip col 0 (HashAlgId)
	else if (table == 0x23) base = 0;    // AssemblyRef: starts at MajorVersion
	else {
		s = "(wrong table for identity)";
		return s;
	}

	#define COL(_i) ClrReadCol(row, v.colOffset[table][base + (_i)], v.colWidth[table][base + (_i)])
	DWORD major     = COL(0);
	DWORD minor     = COL(1);
	DWORD build     = COL(2);
	DWORD rev       = COL(3);
	DWORD flags     = COL(4);
	DWORD pkBlobIdx = COL(5);
	DWORD nameIdx   = COL(6);
	DWORD localeIdx = COL(7);
	#undef COL

	// #Strings is UTF-8. Embed UTF-8 bytes inline in CStringA — the caller
	// transcodes to wide once at the display boundary (CreateClrHeader /
	// BuildAssemblyRefTree).
	const char* name   = ClrGetString(v, nameIdx);
	const char* locale = ClrGetString(v, localeIdx);

	s.Format("%s, Version=%u.%u.%u.%u, Culture=%s",
	         (name && *name)     ? name   : "(unnamed)",
	         (unsigned) major, (unsigned) minor, (unsigned) build, (unsigned) rev,
	         (locale && *locale) ? locale : "neutral");

	// PublicKeyToken rendering.
	const BYTE* blob = NULL;
	DWORD       blobSize = 0;
	BYTE        token[8] = {0};
	bool        haveToken = false;
	bool        hasKey = (table == 0x20) || ((flags & afPublicKey) != 0);

	if (pkBlobIdx && ClrGetBlob(v, pkBlobIdx, &blob, &blobSize) && blobSize > 0) {
		if (hasKey) {
			haveToken = ComputePublicKeyToken(blob, blobSize, token);
		} else if (blobSize == 8) {
			// AssemblyRef with token stored directly (afPublicKey clear).
			// Storage bytes are already in the conventional order — just reverse for display.
			memcpy(token, blob, 8);
			haveToken = true;
		}
	}

	s += ", PublicKeyToken=";
	if (haveToken) s += FormatPublicKeyToken(token);
	else           s += "null";

	// Append ProcessorArchitecture + Retargetable + ContentType tags if interesting.
	CStringA extra;
	if (flags & afPA_Specified) {
		extra += ", ProcessorArchitecture=";
		extra += DecodeProcessorArch(flags);
	}
	if (flags & afRetargetable)
		extra += ", Retargetable=Yes";
	if ((flags & afContentType_Mask) == afContentType_WindowsRuntime)
		extra += ", ContentType=WindowsRuntime";
	s += extra;

	return s;
}

// Render the Module row's identity (fallback for manifestless .netmodule files).
// Module schema: Generation(u16), Name(str), Mvid(guid), EncId(guid), EncBaseId(guid)
static CStringA RenderModuleIdentity(const ClrView &v, DWORD rid)
{
	CStringA s;
	const BYTE* row;
	if (!ClrGetRow(v, 0x00, rid, &row)) {
		s = "(malformed Module row)";
		return s;
	}

	DWORD nameIdx = ClrReadCol(row, v.colOffset[0x00][1], v.colWidth[0x00][1]);
	DWORD mvidIdx = ClrReadCol(row, v.colOffset[0x00][2], v.colWidth[0x00][2]);
	const char* name = ClrGetString(v, nameIdx);
	const GUID* mvid = ClrGetGuid(v, mvidIdx);

	CStringA mvidStr = "(none)";
	if (mvid) {
		mvidStr.Format("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
		               mvid->Data1, mvid->Data2, mvid->Data3,
		               mvid->Data4[0], mvid->Data4[1],
		               mvid->Data4[2], mvid->Data4[3],
		               mvid->Data4[4], mvid->Data4[5],
		               mvid->Data4[6], mvid->Data4[7]);
	}

	s.Format("%s  Mvid=%s",
	         (name && *name) ? name : "(unnamed)",
	         (LPCSTR) mvidStr);
	return s;
}

// Top-level identity block — three-way classification per §9 / §12.
static void DumpIdentityBlock(CStringA &s, const ClrView &v)
{
	s += "Assembly Identity\r\n";
	s += "-----------------\r\n";

	DWORD cAssembly = v.rowCount[0x20];
	DWORD cModule   = v.rowCount[0x00];

	if (cAssembly == 1) {
		AppendDecorated(s, "Type:",        "Assembly (manifest-bearing)");
		CStringA identity = RenderAssemblyIdentity(v, 0x20, 1);
		AppendDecorated(s, "Identity:",    "%s", (LPCSTR) identity);

		// Also show HashAlgId (Assembly only).
		const BYTE* row;
		if (ClrGetRow(v, 0x20, 1, &row)) {
			DWORD hashAlg = ClrReadCol(row, v.colOffset[0x20][0], v.colWidth[0x20][0]);
			CStringA algStr = DecodeHashAlgId(hashAlg);
			AppendDecorated(s, "HashAlgId:", "%s", (LPCSTR) algStr);
		}
	} else if (cAssembly == 0) {
		AppendDecorated(s, "Type:",        "Manifestless module (no assembly manifest)");
		if (cModule == 1) {
			CStringA identity = RenderModuleIdentity(v, 1);
			AppendDecorated(s, "Module:",  "%s", (LPCSTR) identity);
		} else {
			AppendDecorated(s, "Module:",  "(invalid - Module table has %u rows, expected exactly 1 per ECMA-335)",
			                (unsigned) cModule);
		}
	} else {
		// > 1 Assembly rows = malformed. Don't silently pick row 1.
		AppendDecorated(s, "Type:",        "INVALID - Assembly table has %u rows (expected 0 or 1 per ECMA-335)",
		                (unsigned) cAssembly);
	}

	// Module row count sanity (independent of Assembly).
	if (cModule != 1) {
		AppendDecorated(s, "Module row:",  "INVALID - Module table has %u rows (expected exactly 1)",
		                (unsigned) cModule);
	}

	AppendDecorated(s, "AssemblyRefs:",    "%u", (unsigned) v.rowCount[0x23]);
	s += "\r\n";
}

// -------- Metadata root dump (§2) ------------------------------------------
static void DumpMetadataRoot(CStringA &s, const ClrView &v)
{
	s += "Metadata Root\r\n";
	s += "-------------\r\n";
	CStringA line;
	AppendDecorated(s, "Signature:",   "'BSJB' (0x424A5342)");
	AppendDecorated(s, "Version:",     "%s", v.mdVersion[0] ? v.mdVersion : "(empty)");
	AppendDecorated(s, "Streams:",     "%u", (unsigned) v.streams.size());

	if (!v.streams.empty())
	{
		s += "\r\n";
		line.Format("  %-10s %-12s %s\r\n", "Name", "Size", "Offset");
		s += line;
		line.Format("  %-10s %-12s %s\r\n", "----", "----", "------");
		s += line;
		for (size_t i = 0; i < v.streams.size(); ++i)
		{
			const ClrStreamEntry &e = v.streams[i];
			line.Format("  %-10s %-12u 0x%08X\r\n",
			            e.name, (unsigned) e.size, (unsigned) e.offset);
			s += line;
		}
	}
	s += "\r\n";
}

// -------- Tables-stream header dump (§3) -----------------------------------
static void DumpTablesStreamHeader(CStringA &s, const ClrView &v)
{
	if (!v.pTables)
	{
		s += "Tables Stream: (none)\r\n\r\n";
		return;
	}

	s += v.tablesIsEnC
	     ? "Tables Stream ('#-', uncompressed / Edit-and-Continue)\r\n"
	     : "Tables Stream ('#~', compressed)\r\n";
	s += "------------------------------------------------------\r\n";

	CStringA line;
	AppendDecorated(s, "Schema version:",  "%u.%u",
	                (unsigned) v.tablesMajor, (unsigned) v.tablesMinor);

	// HeapSizes: bit 0 = #Strings wide, bit 1 = #GUID wide, bit 2 = #Blob wide.
	// Bits 5-7 mark EnC / deleted-rows / extra-data variants (rarely set on #~).
	CStringA hs;
	{
		bool first = true;
		#define APPEND(_label) do { if (!first) hs += " | "; hs += _label; first = false; } while (0)
		if (v.heapSizes & 0x01) APPEND("Strings=4B");
		if (v.heapSizes & 0x02) APPEND("GUID=4B");
		if (v.heapSizes & 0x04) APPEND("Blob=4B");
		if (v.heapSizes & 0x20) APPEND("EnC");
		if (v.heapSizes & 0x40) APPEND("DeletedRows");
		if (v.heapSizes & 0x80) APPEND("ExtraData");
		#undef APPEND
		if (first) hs = "(default — all heaps 2-byte indices)";
	}
	AppendDecorated(s, "HeapSizes:",       "0x%02X  %s", (unsigned) v.heapSizes, (LPCSTR) hs);

	// Table presence / row counts.
	int cPresent = 0;
	for (int i = 0; i < 64; ++i)
		if (v.valid & ((UINT64) 1 << i))
			cPresent++;

	AppendDecorated(s, "Tables present:",  "%d", cPresent);
	s += "\r\n";
	line.Format("  %-4s %-28s %-10s %s\r\n", "ID", "Name", "Rows", "Sorted");
	s += line;
	line.Format("  %-4s %-28s %-10s %s\r\n", "--", "----", "----", "------");
	s += line;
	for (int i = 0; i < 64; ++i)
	{
		if (!(v.valid & ((UINT64) 1 << i)))
			continue;
		const char* name = ClrTableName((BYTE) i);
		CStringA nm;
		if (name) nm = name;
		else      nm.Format("(reserved 0x%02X)", i);
		bool isSorted = (v.sorted & ((UINT64) 1 << i)) != 0;
		line.Format("  0x%02X %-28s %-10u %s\r\n",
		            i, (LPCSTR) nm,
		            (unsigned) v.rowCount[i],
		            isSorted ? "yes" : "");
		s += line;
	}
	s += "\r\n";
}

// ---------------------------------------------------------------------------
CStringA DumpClrHeader(PE_EXE &pe)
{
	CStringA s;
	ClrView v;
	CStringA err;
	if (!InitClrView(pe, v, &err))
	{
		s += "CLR Header\r\n";
		s += "==========\r\n\r\n";
		s += err.IsEmpty() ? CStringA("CLR: unknown parse error") : err;
		s += "\r\n";
		return s;
	}

	// Section 1: Assembly identity (§9 / §12) — goes first since it's the
	// most useful info for a user opening a managed PE.
	DumpIdentityBlock(s, v);

	// Section 2: Debug-info / PDB advertisement (§10) — identity-category data,
	// unconditional (not gated on ShowDebug).
	DumpDebugDirectorySummary(s, pe);

	// Section 3: Native image summary (§11) — if present.
	DumpManagedNativeHeader(s, pe, v.cor20);

	// Section 4: COR20 header (§1)
	DumpCor20Header(s, v.cor20);

	// Section 5: Metadata root (§2)
	DumpMetadataRoot(s, v);

	// Section 6: Tables-stream header (§3)
	DumpTablesStreamHeader(s, v);

	return s;
}

// Insert a tree node with a wide-string label, bypassing the MBCS wrapper so
// Unicode identifiers render correctly (the TreeView control is Unicode-native
// regardless of the app's character set).
static HTREEITEM InsertTreeItemW(CTreeCtrl &tree, HTREEITEM parent, const CStringW &label)
{
	TVINSERTSTRUCTW tvis;
	memset(&tvis, 0, sizeof(tvis));
	tvis.hParent = parent;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_TEXT;
	tvis.item.pszText = (LPWSTR) (LPCWSTR) label;
	tvis.item.cchTextMax = label.GetLength();
	return (HTREEITEM) ::SendMessageW(tree.m_hWnd, TVM_INSERTITEMW, 0, (LPARAM) &tvis);
}

void BuildAssemblyRefTree(PE_EXE &pe, CTreeCtrl &tree)
{
	ClrView v;
	CStringA err;
	if (!InitClrView(pe, v, &err))
	{
		InsertTreeItemW(tree, TVI_ROOT, L"(CLR metadata is malformed - see CLR Header tab)");
		return;
	}

	// Root node: self-identity, same three-way classification as §9/§12.
	DWORD cAssembly = v.rowCount[0x20];
	DWORD cModule   = v.rowCount[0x00];

	CStringA rootA;
	if (cAssembly == 1) {
		rootA = RenderAssemblyIdentity(v, 0x20, 1);
	} else if (cAssembly == 0) {
		if (cModule == 1) {
			CStringA modId = RenderModuleIdentity(v, 1);
			rootA.Format("[Manifestless module] %s", (LPCSTR) modId);
		} else {
			rootA.Format("[Invalid - Module table has %u rows]", (unsigned) cModule);
		}
	} else {
		rootA.Format("[Invalid - Assembly table has %u rows]", (unsigned) cAssembly);
	}

	HTREEITEM root = InsertTreeItemW(tree, TVI_ROOT, Utf8ToWide(rootA));

	// Children: one per AssemblyRef row (table 0x23), in rid order.
	DWORD cRefs = v.rowCount[0x23];
	if (cRefs == 0) {
		InsertTreeItemW(tree, root, L"(no AssemblyRef entries)");
	} else {
		for (DWORD rid = 1; rid <= cRefs; ++rid) {
			CStringA childA = RenderAssemblyIdentity(v, 0x23, rid);
			InsertTreeItemW(tree, root, Utf8ToWide(childA));
		}
	}
	tree.Expand(root, TVE_EXPAND);
}
