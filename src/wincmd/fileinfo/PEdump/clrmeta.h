//==========================================
// clrmeta.h - ECMA-335 metadata-root parser with full bounds checking.
//
// The file-mapping is attacker-controlled — every (RVA, Size) or (offset, size)
// tuple consumed here is validated before any dereference. See
// docs/dotnet-reference.md §12 bullet 7 for the seven-layer cascade.
//==========================================
#ifndef __CLRMETA_H__
#define __CLRMETA_H__

#include "..\PEexe\PEEXE.H"
#include <vector>

// ECMA-335 §II.22 table IDs. Only enumerated up to COUNT; unused IDs in the
// 0x00..0x2C range are intentionally reserved. 0x30..0x35 are portable-PDB
// tables (see §3 of reference doc); MVP does not parse portable PDBs, but
// they may still appear in the `Valid` bitmask when the file has an embedded
// #Pdb stream — row counts for those IDs are stored so the tables-stream
// header dump can round-trip them.
enum ClrTableId
{
	CLR_TBL_Module                 = 0x00,
	CLR_TBL_TypeRef                = 0x01,
	CLR_TBL_TypeDef                = 0x02,
	CLR_TBL_Field                  = 0x04,
	CLR_TBL_MethodDef              = 0x06,
	CLR_TBL_Param                  = 0x08,
	CLR_TBL_InterfaceImpl          = 0x09,
	CLR_TBL_MemberRef              = 0x0A,
	CLR_TBL_Constant               = 0x0B,
	CLR_TBL_CustomAttribute        = 0x0C,
	CLR_TBL_FieldMarshal           = 0x0D,
	CLR_TBL_DeclSecurity           = 0x0E,
	CLR_TBL_ClassLayout            = 0x0F,
	CLR_TBL_FieldLayout            = 0x10,
	CLR_TBL_StandAloneSig          = 0x11,
	CLR_TBL_EventMap               = 0x12,
	CLR_TBL_Event                  = 0x14,
	CLR_TBL_PropertyMap            = 0x15,
	CLR_TBL_Property               = 0x17,
	CLR_TBL_MethodSemantics        = 0x18,
	CLR_TBL_MethodImpl             = 0x19,
	CLR_TBL_ModuleRef              = 0x1A,
	CLR_TBL_TypeSpec               = 0x1B,
	CLR_TBL_ImplMap                = 0x1C,
	CLR_TBL_FieldRVA               = 0x1D,
	CLR_TBL_Assembly               = 0x20,
	CLR_TBL_AssemblyRef            = 0x23,
	CLR_TBL_File                   = 0x26,
	CLR_TBL_ExportedType           = 0x27,
	CLR_TBL_ManifestResource       = 0x28,
	CLR_TBL_NestedClass            = 0x29,
	CLR_TBL_GenericParam           = 0x2A,
	CLR_TBL_MethodSpec             = 0x2B,
	CLR_TBL_GenericParamConstraint = 0x2C,
	CLR_TBL_COUNT                  = 64
};

struct ClrStreamEntry
{
	DWORD offset;               // relative to mdRoot
	DWORD size;
	char  name[32];             // NUL-terminated
};

struct ClrView
{
	PE_EXE*                   pe;
	const BYTE*               fileBase;
	DWORD                     fileSize;

	// COR20 header (pointer into mapped file)
	const IMAGE_COR20_HEADER* cor20;
	DWORD                     cor20Size;         // dir-14 Size, clamped against cor20->cb

	// Metadata root
	const BYTE*               mdRoot;
	DWORD                     mdSize;
	char                      mdVersion[256];    // NUL-terminated version string

	// Stream directory (raw)
	std::vector<ClrStreamEntry> streams;

	// Resolved well-known streams (NULL + 0 if absent)
	const BYTE*               pTables;     DWORD tablesSize;
	bool                      tablesIsEnC;           // true if stream is "#-" not "#~"
	const BYTE*               pStrings;    DWORD stringsSize;
	const BYTE*               pUS;         DWORD usSize;
	const BYTE*               pBlob;       DWORD blobSize;
	const BYTE*               pGuid;       DWORD guidSize;
	const BYTE*               pPdb;        DWORD pdbSize;

	// Tables-stream header (present iff pTables != NULL)
	BYTE                      tablesMajor;
	BYTE                      tablesMinor;
	BYTE                      heapSizes;             // bits: 0=Strings wide, 1=GUID wide, 2=Blob wide
	UINT64                    valid;
	UINT64                    sorted;
	DWORD                     rowCount[CLR_TBL_COUNT];

	// Computed in Step 6 (zero-initialized until that step is implemented).
	BYTE                      stringIdxBytes;        // 2 or 4
	BYTE                      guidIdxBytes;          // 2 or 4
	BYTE                      blobIdxBytes;          // 2 or 4
	BYTE                      colCount[CLR_TBL_COUNT];
	BYTE                      colWidth[CLR_TBL_COUNT][16];
	BYTE                      colOffset[CLR_TBL_COUNT][16];
	WORD                      rowSize[CLR_TBL_COUNT];
	const BYTE*               rowBase[CLR_TBL_COUNT];

	ClrView();
};

// Run the full §12 bullet-7 validation cascade. Returns true on success.
// On failure, errOut (if non-null) gets a single-line human-readable description.
bool InitClrView(PE_EXE &pe, ClrView &out, CStringA *errOut = NULL);

// Returns a readable pointer for [rva, rva+size) that is proven to lie entirely
// within the mapped file, or NULL on any bounds failure. Unlike
// PE_EXE::GetReadablePointerFromRVA (which only translates the start), this
// also validates that size bytes fit inside the file — required for
// attacker-controlled RVA+Size pairs from the debug directory,
// ManagedNativeHeader, etc.
const BYTE* ClrSafeRvaRange(PE_EXE &pe, DWORD rva, DWORD size);

// Decoded name of a known table (e.g. "Module", "Assembly"), or NULL if reserved.
const char* ClrTableName(BYTE id);

// Heap accessors (Step 6). Return safe defaults on out-of-range.
const char* ClrGetString(const ClrView &v, DWORD offset);
const GUID* ClrGetGuid  (const ClrView &v, DWORD index1);
bool        ClrGetBlob  (const ClrView &v, DWORD offset, const BYTE** outPtr, DWORD* outSize);

// Row accessor (1-based rid). Writes row pointer into *outRow on success.
bool ClrGetRow(const ClrView &v, BYTE tableId, DWORD rid1, const BYTE** outRow);

// Little-endian 1/2/4-byte read from a row column.
DWORD ClrReadCol(const BYTE* row, DWORD offset, DWORD width);

#endif
