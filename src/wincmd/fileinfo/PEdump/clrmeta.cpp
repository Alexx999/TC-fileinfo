//==========================================
// clrmeta.cpp - ECMA-335 metadata-root parser with bounds checking.
// See docs/dotnet-reference.md §2-§3 for on-disk layout.
//==========================================
#include "stdafx.h"
#include "clrmeta.h"
#include <string.h>

// ---------------------------------------------------------------------------
// Bounds-check helper. Returns true iff [p, p+n) fits inside [base, base+size).
// ---------------------------------------------------------------------------
static inline bool RangeIn(const BYTE* p, DWORD n, const BYTE* base, DWORD size)
{
	if (!p || !base) return false;
	if (p < base) return false;
	size_t off = (size_t)(p - base);
	if (off > (size_t)size) return false;
	if ((size_t)n > (size_t)size - off) return false;
	return true;
}

// Little-endian reads (metadata is always little-endian on disk per ECMA-335 §II.24.1).
static inline UINT32 ReadU32LE(const BYTE* p) { return (UINT32)p[0] | ((UINT32)p[1] << 8) | ((UINT32)p[2] << 16) | ((UINT32)p[3] << 24); }
static inline UINT16 ReadU16LE(const BYTE* p) { return (UINT16)((UINT16)p[0] | ((UINT16)p[1] << 8)); }
static inline UINT64 ReadU64LE(const BYTE* p) {
	UINT64 lo = ReadU32LE(p);
	UINT64 hi = ReadU32LE(p + 4);
	return lo | (hi << 32);
}

// ---------------------------------------------------------------------------
// ClrView: default-construct all fields to safe zero.
// ---------------------------------------------------------------------------
ClrView::ClrView()
{
	pe = NULL;
	fileBase = NULL;
	fileSize = 0;
	cor20 = NULL;
	cor20Size = 0;
	mdRoot = NULL;
	mdSize = 0;
	mdVersion[0] = 0;
	pTables = NULL;  tablesSize = 0;
	tablesIsEnC = false;
	pStrings = NULL; stringsSize = 0;
	pUS = NULL;      usSize = 0;
	pBlob = NULL;    blobSize = 0;
	pGuid = NULL;    guidSize = 0;
	pPdb = NULL;     pdbSize = 0;
	tablesMajor = 0; tablesMinor = 0;
	heapSizes = 0;
	valid = 0;
	sorted = 0;
	memset(rowCount, 0, sizeof(rowCount));
	stringIdxBytes = 2; guidIdxBytes = 2; blobIdxBytes = 2;
	memset(colCount, 0, sizeof(colCount));
	memset(colWidth, 0, sizeof(colWidth));
	memset(colOffset, 0, sizeof(colOffset));
	memset(rowSize, 0, sizeof(rowSize));
	memset(rowBase, 0, sizeof(rowBase));
}

// ---------------------------------------------------------------------------
// Schema model — 38 standard ECMA-335 §II.22 tables.
// Each table's row layout is an array of (kind, param) entries terminated by
// (0,0). See docs/dotnet-reference.md §3 / §5 and docs/dotnet-headers/
// metamodelcolumndefs.h for the source. Column widths depend on runtime state
// (heapSizes bits + per-table row counts) and are resolved in BuildColumnLayout.
// ---------------------------------------------------------------------------
enum ClrColKind
{
	CK_END   = 0,
	CK_U8    = 1,    // 1-byte fixed
	CK_U16   = 2,    // 2-byte fixed
	CK_U32   = 3,    // 4-byte fixed
	CK_STR,          // #Strings heap-index
	CK_GUID,         // #GUID heap-index
	CK_BLOB,         // #Blob heap-index
	CK_RID,          // RID into table `param`
	CK_CDTKN         // coded-token with type `param` (ClrCodedIdx)
};

enum ClrCodedIdx
{
	CDI_TypeDefOrRef        = 0,
	CDI_HasConstant,
	CDI_HasCustomAttribute,
	CDI_HasFieldMarshal,
	CDI_HasDeclSecurity,
	CDI_MemberRefParent,
	CDI_HasSemantic,
	CDI_MethodDefOrRef,
	CDI_MemberForwarded,
	CDI_Implementation,
	CDI_CustomAttributeType,
	CDI_ResolutionScope,
	CDI_TypeOrMethodDef,
	CDI_COUNT
};

struct ColumnDef { BYTE kind; BYTE param; };

#define C(k, p) { (BYTE) (k), (BYTE) (p) }

static const ColumnDef kSchema_Module[]          = { C(CK_U16,0), C(CK_STR,0), C(CK_GUID,0), C(CK_GUID,0), C(CK_GUID,0), C(CK_END,0) };
static const ColumnDef kSchema_TypeRef[]         = { C(CK_CDTKN,CDI_ResolutionScope), C(CK_STR,0), C(CK_STR,0), C(CK_END,0) };
static const ColumnDef kSchema_TypeDef[]         = { C(CK_U32,0), C(CK_STR,0), C(CK_STR,0), C(CK_CDTKN,CDI_TypeDefOrRef), C(CK_RID,0x04), C(CK_RID,0x06), C(CK_END,0) };
static const ColumnDef kSchema_FieldPtr[]        = { C(CK_RID,0x04), C(CK_END,0) };
static const ColumnDef kSchema_Field[]           = { C(CK_U16,0), C(CK_STR,0), C(CK_BLOB,0), C(CK_END,0) };
static const ColumnDef kSchema_MethodPtr[]       = { C(CK_RID,0x06), C(CK_END,0) };
static const ColumnDef kSchema_MethodDef[]       = { C(CK_U32,0), C(CK_U16,0), C(CK_U16,0), C(CK_STR,0), C(CK_BLOB,0), C(CK_RID,0x08), C(CK_END,0) };
static const ColumnDef kSchema_ParamPtr[]        = { C(CK_RID,0x08), C(CK_END,0) };
static const ColumnDef kSchema_Param[]           = { C(CK_U16,0), C(CK_U16,0), C(CK_STR,0), C(CK_END,0) };
static const ColumnDef kSchema_InterfaceImpl[]   = { C(CK_RID,0x02), C(CK_CDTKN,CDI_TypeDefOrRef), C(CK_END,0) };
static const ColumnDef kSchema_MemberRef[]       = { C(CK_CDTKN,CDI_MemberRefParent), C(CK_STR,0), C(CK_BLOB,0), C(CK_END,0) };
static const ColumnDef kSchema_Constant[]        = { C(CK_U8,0), C(CK_U8,0), C(CK_CDTKN,CDI_HasConstant), C(CK_BLOB,0), C(CK_END,0) };
static const ColumnDef kSchema_CustomAttribute[] = { C(CK_CDTKN,CDI_HasCustomAttribute), C(CK_CDTKN,CDI_CustomAttributeType), C(CK_BLOB,0), C(CK_END,0) };
static const ColumnDef kSchema_FieldMarshal[]    = { C(CK_CDTKN,CDI_HasFieldMarshal), C(CK_BLOB,0), C(CK_END,0) };
static const ColumnDef kSchema_DeclSecurity[]    = { C(CK_U16,0), C(CK_CDTKN,CDI_HasDeclSecurity), C(CK_BLOB,0), C(CK_END,0) };
static const ColumnDef kSchema_ClassLayout[]     = { C(CK_U16,0), C(CK_U32,0), C(CK_RID,0x02), C(CK_END,0) };
static const ColumnDef kSchema_FieldLayout[]     = { C(CK_U32,0), C(CK_RID,0x04), C(CK_END,0) };
static const ColumnDef kSchema_StandAloneSig[]   = { C(CK_BLOB,0), C(CK_END,0) };
static const ColumnDef kSchema_EventMap[]        = { C(CK_RID,0x02), C(CK_RID,0x14), C(CK_END,0) };
static const ColumnDef kSchema_EventPtr[]        = { C(CK_RID,0x14), C(CK_END,0) };
static const ColumnDef kSchema_Event[]           = { C(CK_U16,0), C(CK_STR,0), C(CK_CDTKN,CDI_TypeDefOrRef), C(CK_END,0) };
static const ColumnDef kSchema_PropertyMap[]     = { C(CK_RID,0x02), C(CK_RID,0x17), C(CK_END,0) };
static const ColumnDef kSchema_PropertyPtr[]     = { C(CK_RID,0x17), C(CK_END,0) };
static const ColumnDef kSchema_Property[]        = { C(CK_U16,0), C(CK_STR,0), C(CK_BLOB,0), C(CK_END,0) };
static const ColumnDef kSchema_MethodSemantics[] = { C(CK_U16,0), C(CK_RID,0x06), C(CK_CDTKN,CDI_HasSemantic), C(CK_END,0) };
static const ColumnDef kSchema_MethodImpl[]      = { C(CK_RID,0x02), C(CK_CDTKN,CDI_MethodDefOrRef), C(CK_CDTKN,CDI_MethodDefOrRef), C(CK_END,0) };
static const ColumnDef kSchema_ModuleRef[]       = { C(CK_STR,0), C(CK_END,0) };
static const ColumnDef kSchema_TypeSpec[]        = { C(CK_BLOB,0), C(CK_END,0) };
static const ColumnDef kSchema_ImplMap[]         = { C(CK_U16,0), C(CK_CDTKN,CDI_MemberForwarded), C(CK_STR,0), C(CK_RID,0x1A), C(CK_END,0) };
static const ColumnDef kSchema_FieldRVA[]        = { C(CK_U32,0), C(CK_RID,0x04), C(CK_END,0) };
static const ColumnDef kSchema_ENCLog[]          = { C(CK_U32,0), C(CK_U32,0), C(CK_END,0) };
static const ColumnDef kSchema_ENCMap[]          = { C(CK_U32,0), C(CK_END,0) };
static const ColumnDef kSchema_Assembly[]        = { C(CK_U32,0), C(CK_U16,0), C(CK_U16,0), C(CK_U16,0), C(CK_U16,0), C(CK_U32,0), C(CK_BLOB,0), C(CK_STR,0), C(CK_STR,0), C(CK_END,0) };
static const ColumnDef kSchema_AssemblyProcessor[] = { C(CK_U32,0), C(CK_END,0) };
static const ColumnDef kSchema_AssemblyOS[]      = { C(CK_U32,0), C(CK_U32,0), C(CK_U32,0), C(CK_END,0) };
static const ColumnDef kSchema_AssemblyRef[]     = { C(CK_U16,0), C(CK_U16,0), C(CK_U16,0), C(CK_U16,0), C(CK_U32,0), C(CK_BLOB,0), C(CK_STR,0), C(CK_STR,0), C(CK_BLOB,0), C(CK_END,0) };
static const ColumnDef kSchema_AssemblyRefProcessor[] = { C(CK_U32,0), C(CK_RID,0x23), C(CK_END,0) };
static const ColumnDef kSchema_AssemblyRefOS[]   = { C(CK_U32,0), C(CK_U32,0), C(CK_U32,0), C(CK_RID,0x23), C(CK_END,0) };
static const ColumnDef kSchema_File[]            = { C(CK_U32,0), C(CK_STR,0), C(CK_BLOB,0), C(CK_END,0) };
static const ColumnDef kSchema_ExportedType[]    = { C(CK_U32,0), C(CK_U32,0), C(CK_STR,0), C(CK_STR,0), C(CK_CDTKN,CDI_Implementation), C(CK_END,0) };
static const ColumnDef kSchema_ManifestResource[] = { C(CK_U32,0), C(CK_U32,0), C(CK_STR,0), C(CK_CDTKN,CDI_Implementation), C(CK_END,0) };
static const ColumnDef kSchema_NestedClass[]     = { C(CK_RID,0x02), C(CK_RID,0x02), C(CK_END,0) };
static const ColumnDef kSchema_GenericParam[]    = { C(CK_U16,0), C(CK_U16,0), C(CK_CDTKN,CDI_TypeOrMethodDef), C(CK_STR,0), C(CK_END,0) };
static const ColumnDef kSchema_MethodSpec[]      = { C(CK_CDTKN,CDI_MethodDefOrRef), C(CK_BLOB,0), C(CK_END,0) };
static const ColumnDef kSchema_GenericParamConstraint[] = { C(CK_RID,0x2A), C(CK_CDTKN,CDI_TypeDefOrRef), C(CK_END,0) };

#undef C

// Coded-token definitions: list of constituent table IDs (sentinel 0xFF) and
// tag-bit count (ceil(log2(N)) where N is number of slots incl. reserved).
// Source: metamodel.cpp g_CodedTokens[] + reference doc §5 table.
struct CodedIdxDef { BYTE tagBits; BYTE tables[32]; };

// 0xFF is an "invalid/reserved" slot — still counts toward tagBits.
static const CodedIdxDef kCoded[CDI_COUNT] = {
	/* CDI_TypeDefOrRef        */ { 2, { 0x02, 0x01, 0x1B, 0xFF } },
	/* CDI_HasConstant         */ { 2, { 0x04, 0x08, 0x17, 0xFF } },
	/* CDI_HasCustomAttribute  */ { 5, { 0x06, 0x04, 0x01, 0x02, 0x08, 0x09, 0x0A, 0x00,
	                                      0x0E, 0x17, 0x14, 0x11, 0x1A, 0x1B, 0x20, 0x23,
	                                      0x26, 0x27, 0x28, 0x2A, 0x2C, 0x2B, 0xFF, 0xFF,
	                                      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } },
	/* CDI_HasFieldMarshal     */ { 1, { 0x04, 0x08, 0xFF } },
	/* CDI_HasDeclSecurity     */ { 2, { 0x02, 0x06, 0x20, 0xFF } },
	/* CDI_MemberRefParent     */ { 3, { 0x02, 0x01, 0x1A, 0x06, 0x1B, 0xFF } },
	/* CDI_HasSemantic         */ { 1, { 0x14, 0x17, 0xFF } },
	/* CDI_MethodDefOrRef      */ { 1, { 0x06, 0x0A, 0xFF } },
	/* CDI_MemberForwarded     */ { 1, { 0x04, 0x06, 0xFF } },
	/* CDI_Implementation      */ { 2, { 0x26, 0x23, 0x27, 0xFF } },
	/* CDI_CustomAttributeType */ { 3, { 0xFF, 0xFF, 0x06, 0x0A, 0xFF } },   // 2 reserved low slots
	/* CDI_ResolutionScope     */ { 2, { 0x00, 0x1A, 0x23, 0x01, 0xFF } },
	/* CDI_TypeOrMethodDef     */ { 1, { 0x02, 0x06, 0xFF } },
};

static const ColumnDef* GetSchema(BYTE tableId)
{
	switch (tableId)
	{
		case 0x00: return kSchema_Module;
		case 0x01: return kSchema_TypeRef;
		case 0x02: return kSchema_TypeDef;
		case 0x03: return kSchema_FieldPtr;
		case 0x04: return kSchema_Field;
		case 0x05: return kSchema_MethodPtr;
		case 0x06: return kSchema_MethodDef;
		case 0x07: return kSchema_ParamPtr;
		case 0x08: return kSchema_Param;
		case 0x09: return kSchema_InterfaceImpl;
		case 0x0A: return kSchema_MemberRef;
		case 0x0B: return kSchema_Constant;
		case 0x0C: return kSchema_CustomAttribute;
		case 0x0D: return kSchema_FieldMarshal;
		case 0x0E: return kSchema_DeclSecurity;
		case 0x0F: return kSchema_ClassLayout;
		case 0x10: return kSchema_FieldLayout;
		case 0x11: return kSchema_StandAloneSig;
		case 0x12: return kSchema_EventMap;
		case 0x13: return kSchema_EventPtr;
		case 0x14: return kSchema_Event;
		case 0x15: return kSchema_PropertyMap;
		case 0x16: return kSchema_PropertyPtr;
		case 0x17: return kSchema_Property;
		case 0x18: return kSchema_MethodSemantics;
		case 0x19: return kSchema_MethodImpl;
		case 0x1A: return kSchema_ModuleRef;
		case 0x1B: return kSchema_TypeSpec;
		case 0x1C: return kSchema_ImplMap;
		case 0x1D: return kSchema_FieldRVA;
		case 0x1E: return kSchema_ENCLog;
		case 0x1F: return kSchema_ENCMap;
		case 0x20: return kSchema_Assembly;
		case 0x21: return kSchema_AssemblyProcessor;
		case 0x22: return kSchema_AssemblyOS;
		case 0x23: return kSchema_AssemblyRef;
		case 0x24: return kSchema_AssemblyRefProcessor;
		case 0x25: return kSchema_AssemblyRefOS;
		case 0x26: return kSchema_File;
		case 0x27: return kSchema_ExportedType;
		case 0x28: return kSchema_ManifestResource;
		case 0x29: return kSchema_NestedClass;
		case 0x2A: return kSchema_GenericParam;
		case 0x2B: return kSchema_MethodSpec;
		case 0x2C: return kSchema_GenericParamConstraint;
	}
	return NULL;
}

// Width of a coded-token column for this file's row-count profile.
// Rule (§5): 2 bytes if max constituent row count fits in (16 - tagBits) bits, else 4.
static BYTE CodedIdxWidth(const ClrView &v, BYTE codedIdx)
{
	if (codedIdx >= CDI_COUNT) return 2;
	const CodedIdxDef &c = kCoded[codedIdx];
	DWORD limit = (DWORD) 1 << (16 - c.tagBits);
	for (int i = 0; i < (int) sizeof(c.tables); ++i) {
		BYTE t = c.tables[i];
		if (t == 0xFF) continue;
		if (t >= CLR_TBL_COUNT) continue;
		if (v.rowCount[t] >= limit) return 4;
	}
	return 2;
}

// Width of a RID column pointing at `targetTable`.
static BYTE RidWidth(const ClrView &v, BYTE targetTable)
{
	if (targetTable >= CLR_TBL_COUNT) return 2;
	return (v.rowCount[targetTable] >= 0x10000) ? 4 : 2;
}

// Width of a heap-index column.
static BYTE HeapIdxWidth(BYTE kind, const ClrView &v)
{
	switch (kind) {
		case CK_STR:  return v.stringIdxBytes;
		case CK_GUID: return v.guidIdxBytes;
		case CK_BLOB: return v.blobIdxBytes;
	}
	return 0;
}

// ---------------------------------------------------------------------------
// Populate per-table column widths, row sizes, and rowBase pointers.
// Returns false if the declared row data would run off the end of the tables stream.
// ---------------------------------------------------------------------------
static bool BuildColumnLayout(ClrView &v, CStringA *errOut)
{
	v.stringIdxBytes = (v.heapSizes & 0x01) ? 4 : 2;
	v.guidIdxBytes   = (v.heapSizes & 0x02) ? 4 : 2;
	v.blobIdxBytes   = (v.heapSizes & 0x04) ? 4 : 2;

	// 1. Compute per-table column widths / offsets / row sizes.
	for (int t = 0; t < CLR_TBL_COUNT; ++t)
	{
		if (!(v.valid & ((UINT64) 1 << t))) continue;
		const ColumnDef* schema = GetSchema((BYTE) t);
		if (!schema) {
			// Reserved / portable-PDB table whose schema is not in our model.
			// We cannot compute its row size, so leave rowSize[t] == 0;
			// BuildColumnLayout's pass-2 check will fail if the table has any rows.
			continue;
		}
		DWORD offset = 0;
		BYTE  n = 0;
		for (const ColumnDef* c = schema; c->kind != CK_END; ++c, ++n)
		{
			if (n >= 16) {
				if (errOut) errOut->Format("CLR: table 0x%02X has >16 columns (unsupported)", t);
				return false;
			}
			BYTE w = 0;
			switch (c->kind) {
				case CK_U8:    w = 1; break;
				case CK_U16:   w = 2; break;
				case CK_U32:   w = 4; break;
				case CK_STR:   w = v.stringIdxBytes; break;
				case CK_GUID:  w = v.guidIdxBytes;   break;
				case CK_BLOB:  w = v.blobIdxBytes;   break;
				case CK_RID:   w = RidWidth(v, c->param);    break;
				case CK_CDTKN: w = CodedIdxWidth(v, c->param); break;
				default:
					if (errOut) errOut->Format("CLR: internal: unknown column kind %u", c->kind);
					return false;
			}
			v.colWidth[t][n]  = w;
			v.colOffset[t][n] = (BYTE) offset;
			offset += w;
			if (offset > 0xFFFF) {
				if (errOut) errOut->Format("CLR: table 0x%02X row size exceeds 65535 bytes", t);
				return false;
			}
		}
		v.colCount[t] = n;
		v.rowSize[t]  = (WORD) offset;
	}

	// 2. Walk tables in ID order, assigning rowBase pointers into the stream.
	// Header: 24 bytes + 4 × popcount(Valid) Rows entries.
	int cPresent = 0;
	for (int i = 0; i < 64; ++i)
		if (v.valid & ((UINT64) 1 << i))
			cPresent++;

	DWORD headerBytes = 24 + (DWORD) cPresent * 4;
	if (headerBytes > v.tablesSize) {
		if (errOut) *errOut = "CLR: tables-stream header exceeds stream size (should not happen)";
		return false;
	}

	DWORD runningOffset = headerBytes;
	for (int t = 0; t < CLR_TBL_COUNT; ++t)
	{
		if (!(v.valid & ((UINT64) 1 << t))) continue;

		v.rowBase[t] = v.pTables + runningOffset;

		// If rowSize is 0 for a present table, either we don't know the schema
		// (portable-PDB table, etc.) or the table is a sentinel — if it has any
		// rows, we can't safely advance. Fall back to leaving rowBase[] set but
		// stopping the cumulative walk.
		if (v.rowCount[t] == 0)
			continue;
		if (v.rowSize[t] == 0) {
			// Unknown schema but non-zero rows — further rowBase[] values will
			// be stale. Clear them and report a diagnostic.
			for (int u = t + 1; u < CLR_TBL_COUNT; ++u) v.rowBase[u] = NULL;
			if (errOut) errOut->Format("CLR: table 0x%02X has %u rows but no schema; downstream tables unresolved",
			                           t, (unsigned) v.rowCount[t]);
			// Return true — partial layout is useful for MVP inspectors since
			// Module / Assembly / AssemblyRef all have known schemas.
			return true;
		}

		UINT64 rowsBytes = (UINT64) v.rowCount[t] * v.rowSize[t];
		if (rowsBytes > (UINT64) (v.tablesSize - runningOffset)) {
			if (errOut) errOut->Format("CLR: table 0x%02X rows (%u x %u) extend past tables-stream end",
			                           t, (unsigned) v.rowCount[t], (unsigned) v.rowSize[t]);
			return false;
		}
		runningOffset += (DWORD) rowsBytes;
	}

	return true;
}

// ---------------------------------------------------------------------------
// Table-name lookup (partial — only MVP-interesting tables).
// ---------------------------------------------------------------------------
const char* ClrTableName(BYTE id)
{
	switch (id) {
		case 0x00: return "Module";
		case 0x01: return "TypeRef";
		case 0x02: return "TypeDef";
		case 0x03: return "FieldPtr";
		case 0x04: return "Field";
		case 0x05: return "MethodPtr";
		case 0x06: return "MethodDef";
		case 0x07: return "ParamPtr";
		case 0x08: return "Param";
		case 0x09: return "InterfaceImpl";
		case 0x0A: return "MemberRef";
		case 0x0B: return "Constant";
		case 0x0C: return "CustomAttribute";
		case 0x0D: return "FieldMarshal";
		case 0x0E: return "DeclSecurity";
		case 0x0F: return "ClassLayout";
		case 0x10: return "FieldLayout";
		case 0x11: return "StandAloneSig";
		case 0x12: return "EventMap";
		case 0x13: return "EventPtr";
		case 0x14: return "Event";
		case 0x15: return "PropertyMap";
		case 0x16: return "PropertyPtr";
		case 0x17: return "Property";
		case 0x18: return "MethodSemantics";
		case 0x19: return "MethodImpl";
		case 0x1A: return "ModuleRef";
		case 0x1B: return "TypeSpec";
		case 0x1C: return "ImplMap";
		case 0x1D: return "FieldRVA";
		case 0x1E: return "ENCLog";
		case 0x1F: return "ENCMap";
		case 0x20: return "Assembly";
		case 0x21: return "AssemblyProcessor";
		case 0x22: return "AssemblyOS";
		case 0x23: return "AssemblyRef";
		case 0x24: return "AssemblyRefProcessor";
		case 0x25: return "AssemblyRefOS";
		case 0x26: return "File";
		case 0x27: return "ExportedType";
		case 0x28: return "ManifestResource";
		case 0x29: return "NestedClass";
		case 0x2A: return "GenericParam";
		case 0x2B: return "MethodSpec";
		case 0x2C: return "GenericParamConstraint";
		case 0x30: return "Document";
		case 0x31: return "MethodDebugInformation";
		case 0x32: return "LocalScope";
		case 0x33: return "LocalVariable";
		case 0x34: return "LocalConstant";
		case 0x35: return "ImportScope";
		case 0x36: return "StateMachineMethod";
		case 0x37: return "CustomDebugInformation";
		default:   return NULL;
	}
}

// ---------------------------------------------------------------------------
// Validation cascade (seven layers).
// Each stage returns early with an error string on bounds failure.
// ---------------------------------------------------------------------------
#define FAIL(_msg) do { if (errOut) *errOut = _msg; return false; } while (0)
#define FAILF(_fmt, ...) do { if (errOut) errOut->Format(_fmt, __VA_ARGS__); return false; } while (0)

bool InitClrView(PE_EXE &pe, ClrView &out, CStringA *errOut)
{
	out = ClrView();
	out.pe = &pe;
	out.fileBase = (const BYTE*) pe.GetBase();
	out.fileSize = pe.GetFileSize();
	if (errOut) *errOut = "";

	if (!out.fileBase || out.fileSize < sizeof(IMAGE_COR20_HEADER))
		FAIL("CLR: mapped file is too small");

	// --- Layer 1: Data directory 14 ------------------------------------------
	DWORD cor20RVA  = pe.GetDataDirectoryEntryRVA (IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);
	DWORD cor20Size = pe.GetDataDirectoryEntrySize(IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);
	if (!cor20RVA || cor20Size < sizeof(IMAGE_COR20_HEADER))
		FAIL("CLR: data directory 14 (COM_DESCRIPTOR) is missing or too small");
	const BYTE* cor20Bytes = ClrSafeRvaRange(pe, cor20RVA, cor20Size);
	if (!cor20Bytes)
		FAIL("CLR: COR20 directory is not contained within a single section");

	const IMAGE_COR20_HEADER* cor20 = (const IMAGE_COR20_HEADER*) cor20Bytes;

	// --- Layer 2: COR20 self-declared size (cb) ------------------------------
	// Minimum: everything through MetaData directory (cb >= offsetof(ManagedNativeHeader) + sizeof(DataDir)).
	// Spec clamps effective reads to min(cb, directory Size).
	if (cor20->cb < offsetof(IMAGE_COR20_HEADER, ManagedNativeHeader) + sizeof(IMAGE_DATA_DIRECTORY))
		FAILF("CLR: IMAGE_COR20_HEADER.cb (%u) too small to contain MetaData directory", (unsigned) cor20->cb);
	if (cor20->cb > cor20Size)
		FAILF("CLR: IMAGE_COR20_HEADER.cb (%u) exceeds directory Size (%u)", (unsigned) cor20->cb, (unsigned) cor20Size);

	out.cor20     = cor20;
	out.cor20Size = cor20->cb;   // clamped

	// --- Layer 3: COR20 sub-directories: MetaData --------------------------------
	DWORD mdRVA  = cor20->MetaData.VirtualAddress;
	DWORD mdSize = cor20->MetaData.Size;
	if (!mdRVA || mdSize < 16)
		FAIL("CLR: MetaData directory is missing or too small to contain STORAGESIGNATURE");
	const BYTE* mdBytes = ClrSafeRvaRange(pe, mdRVA, mdSize);
	if (!mdBytes)
		FAIL("CLR: MetaData directory is not contained within a single section");

	out.mdRoot = mdBytes;
	out.mdSize = mdSize;

	// --- Layer 4: Metadata root (STORAGESIGNATURE + version + STORAGEHEADER) -
	// Layout:
	//   +0  DWORD lSignature          ('BSJB')
	//   +4  WORD  iMajorVer
	//   +6  WORD  iMinorVer
	//   +8  DWORD iExtraData          (reserved)
	//  +12  DWORD iVersionString      (length of version[]: padded to 4, max 255)
	//  +16  BYTE  pVersion[iVersionString]
	//  ++   STORAGEHEADER             (fFlags, pad, iStreams)
	//  ++   STORAGESTREAM[iStreams]
	const BYTE* p   = out.mdRoot;
	const BYTE* pe0 = out.mdRoot + out.mdSize;

	DWORD sig = ReadU32LE(p);
	if (sig != 0x424A5342)   // 'BSJB' little-endian
		FAILF("CLR: metadata-root signature is 0x%08X, expected 'BSJB' (0x424A5342)", sig);

	DWORD verStrLen = ReadU32LE(p + 12);
	if (verStrLen > 255)
		FAILF("CLR: metadata-root iVersionString (%u) exceeds 255", verStrLen);
	if (16 + verStrLen + 4 > out.mdSize)  // 4 = min STORAGEHEADER
		FAIL("CLR: metadata-root header extends past end of MetaData stream");

	// Copy version string (already NUL-terminated within verStrLen bytes)
	DWORD copyLen = verStrLen;
	if (copyLen >= sizeof(out.mdVersion))
		copyLen = sizeof(out.mdVersion) - 1;
	memcpy(out.mdVersion, p + 16, copyLen);
	out.mdVersion[copyLen] = 0;
	// Trim at first NUL if any (the ECMA spec pads with NULs within verStrLen)
	for (DWORD i = 0; i < copyLen; ++i) {
		if (out.mdVersion[i] == 0) { /* already NUL-terminated */ break; }
	}

	const BYTE* pStorageHdr = p + 16 + verStrLen;
	// STORAGEHEADER: fFlags(1) pad(1) iStreams(2)
	if (pStorageHdr + 4 > pe0)
		FAIL("CLR: STORAGEHEADER extends past end of MetaData stream");
	UINT16 iStreams = ReadU16LE(pStorageHdr + 2);
	if (iStreams == 0 || iStreams > 16)
		FAILF("CLR: STORAGEHEADER.iStreams = %u (expected 1..16)", (unsigned) iStreams);

	// --- Layer 5: Stream directory ---------------------------------------
	// STORAGESTREAM: offset(4) size(4) name(NUL-terminated, padded to 4, max 32 bytes).
	const BYTE* pStream = pStorageHdr + 4;
	for (UINT16 i = 0; i < iStreams; ++i) {
		if (pStream + 8 > pe0)
			FAIL("CLR: stream directory truncated mid-header");

		ClrStreamEntry e;
		e.offset = ReadU32LE(pStream + 0);
		e.size   = ReadU32LE(pStream + 4);
		const BYTE* pName = pStream + 8;

		// Name: NUL-terminated within 32 bytes, total aligned to 4.
		size_t maxName = (size_t)(pe0 - pName);
		if (maxName > 32) maxName = 32;
		size_t nameLen = 0;
		while (nameLen < maxName && pName[nameLen] != 0) nameLen++;
		if (nameLen >= maxName)
			FAIL("CLR: stream name not NUL-terminated within 32-byte budget");
		memcpy(e.name, pName, nameLen);
		e.name[nameLen] = 0;
		// Validate offset/size fit in mdSize
		if (e.offset > out.mdSize || e.size > out.mdSize - e.offset)
			FAILF("CLR: stream '%s' offset/size (%u/%u) extends past MetaData (%u)",
			      e.name, (unsigned) e.offset, (unsigned) e.size, (unsigned) out.mdSize);

		out.streams.push_back(e);

		// Advance past name + padding. name bytes = nameLen + 1 NUL, padded to 4.
		size_t nameBlockLen = (nameLen + 1 + 3) & ~(size_t)3;
		pStream = pName + nameBlockLen;
	}

	// --- Resolve well-known streams ---
	for (size_t i = 0; i < out.streams.size(); ++i) {
		const ClrStreamEntry &e = out.streams[i];
		const BYTE*  data = out.mdRoot + e.offset;
		// Reject zero-sized standard streams only when their presence is required;
		// an empty #US / #Blob is legal for minimal assemblies.
		if (strcmp(e.name, "#~") == 0) {
			if (out.pTables) FAIL("CLR: duplicate '#~' stream");
			out.pTables = data; out.tablesSize = e.size; out.tablesIsEnC = false;
		} else if (strcmp(e.name, "#-") == 0) {
			if (out.pTables) FAIL("CLR: duplicate '#-' stream");
			out.pTables = data; out.tablesSize = e.size; out.tablesIsEnC = true;
		} else if (strcmp(e.name, "#Strings") == 0) {
			out.pStrings = data; out.stringsSize = e.size;
		} else if (strcmp(e.name, "#US") == 0) {
			out.pUS = data; out.usSize = e.size;
		} else if (strcmp(e.name, "#Blob") == 0) {
			out.pBlob = data; out.blobSize = e.size;
		} else if (strcmp(e.name, "#GUID") == 0) {
			out.pGuid = data; out.guidSize = e.size;
		} else if (strcmp(e.name, "#Pdb") == 0) {
			out.pPdb = data; out.pdbSize = e.size;
		}
		// Other streams (non-standard) are silently ignored — valid per spec.
	}

	if (!out.pTables)
		FAIL("CLR: neither '#~' nor '#-' tables-stream present");

	// --- Layer 6: Tables-stream header (#~ / #-) -------------------------
	// Layout:
	//   +0  DWORD Reserved (=0)
	//   +4  BYTE  MajorVersion (=2)
	//   +5  BYTE  MinorVersion (=0)
	//   +6  BYTE  HeapSizes
	//   +7  BYTE  Reserved
	//   +8  UINT64 Valid
	//  +16  UINT64 Sorted
	//  +24  DWORD Rows[popcount(Valid)]
	if (out.tablesSize < 24)
		FAIL("CLR: tables-stream header truncated (< 24 bytes)");
	const BYTE* t   = out.pTables;
	// Reserved must be 0 (spec), but some tooling sets it non-zero; accept.
	out.tablesMajor = t[4];
	out.tablesMinor = t[5];
	out.heapSizes   = t[6];
	out.valid       = ReadU64LE(t + 8);
	out.sorted      = ReadU64LE(t + 16);

	// Count set bits in Valid (tables present).
	int cPresent = 0;
	for (int i = 0; i < 64; ++i)
		if (out.valid & ((UINT64) 1 << i))
			cPresent++;

	DWORD rowsBytes = (DWORD) cPresent * 4;
	if (rowsBytes / 4 != (DWORD) cPresent)  // overflow guard (cPresent <= 64 so fine)
		FAIL("CLR: tables-stream Rows[] size overflow");
	if (out.tablesSize < 24 + rowsBytes)
		FAIL("CLR: tables-stream Rows[] extends past end of stream");

	// Read row counts into rowCount[tableId] (only filled for present tables).
	int rowsIdx = 0;
	for (int i = 0; i < 64; ++i) {
		if (out.valid & ((UINT64) 1 << i)) {
			if (i >= CLR_TBL_COUNT) {
				// Defense-in-depth: CLR_TBL_COUNT is 64 so unreachable, but keeps future-proof.
				rowsIdx++;
				continue;
			}
			out.rowCount[i] = ReadU32LE(t + 24 + rowsIdx * 4);
			rowsIdx++;
		}
	}

	(void) rowsIdx;  // quiet unused warning in edge builds

	// --- Layer 7: Column widths + row sizes + rowBase pointers ------------
	if (!BuildColumnLayout(out, errOut))
		return false;

	return true;
}

// ---------------------------------------------------------------------------
// Heap accessors — bounds-checked; safe defaults on out-of-range.
// ---------------------------------------------------------------------------
const char* ClrGetString(const ClrView &v, DWORD offset)
{
	static const char kEmpty[] = "";
	if (!v.pStrings || offset >= v.stringsSize) return kEmpty;
	const char* s = (const char*) (v.pStrings + offset);
	// Ensure NUL exists within heap bounds.
	for (DWORD i = offset; i < v.stringsSize; ++i) {
		if (v.pStrings[i] == 0) return s;
	}
	return kEmpty;  // Not NUL-terminated — malformed. Safe fallback.
}

const GUID* ClrGetGuid(const ClrView &v, DWORD index1)
{
	if (!v.pGuid || index1 == 0) return NULL;
	DWORD offset = (index1 - 1) * 16;
	if (offset + 16 > v.guidSize) return NULL;
	return (const GUID*) (v.pGuid + offset);
}

bool ClrGetBlob(const ClrView &v, DWORD offset, const BYTE** outPtr, DWORD* outSize)
{
	if (!v.pBlob || offset >= v.blobSize) return false;
	// Compressed length prefix per ECMA-335 §II.23.2:
	//   byte0 bit 7 = 0 → 1-byte length (0..0x7F)
	//   byte0 bit 6 = 0 → 2-byte big-endian, top 2 bits masked (0..0x3FFF)
	//   else            → 4-byte big-endian, top 3 bits masked (0..0x1FFFFFFF)
	const BYTE* p = v.pBlob + offset;
	DWORD remaining = v.blobSize - offset;
	DWORD len = 0, lenBytes = 0;
	if ((p[0] & 0x80) == 0) {
		len = p[0] & 0x7F;
		lenBytes = 1;
	} else if ((p[0] & 0xC0) == 0x80) {
		if (remaining < 2) return false;
		len = ((DWORD)(p[0] & 0x3F) << 8) | p[1];
		lenBytes = 2;
	} else if ((p[0] & 0xE0) == 0xC0) {
		if (remaining < 4) return false;
		len = ((DWORD)(p[0] & 0x1F) << 24) | ((DWORD) p[1] << 16) | ((DWORD) p[2] << 8) | p[3];
		lenBytes = 4;
	} else {
		return false;  // Invalid prefix
	}
	if (len > remaining - lenBytes) return false;
	*outPtr  = p + lenBytes;
	*outSize = len;
	return true;
}

// Row accessor (valid after InitClrView + BuildColumnLayout).
bool ClrGetRow(const ClrView &v, BYTE tableId, DWORD rid1, const BYTE** outRow)
{
	if (tableId >= CLR_TBL_COUNT || rid1 == 0) return false;
	if (rid1 > v.rowCount[tableId]) return false;
	if (!v.rowBase[tableId] || v.rowSize[tableId] == 0) return false;
	*outRow = v.rowBase[tableId] + (size_t)(rid1 - 1) * v.rowSize[tableId];
	return true;
}

DWORD ClrReadCol(const BYTE* row, DWORD offset, DWORD width)
{
	const BYTE* p = row + offset;
	switch (width) {
		case 1: return p[0];
		case 2: return ReadU16LE(p);
		case 4: return ReadU32LE(p);
	}
	return 0;
}

// ---------------------------------------------------------------------------
// Safe RVA+Size -> pointer resolution. Three invariants are proven here before
// returning any pointer — unlike GetReadablePointerFromRVA, which only
// translates the starting offset.
//
//   (1) [rva, rva+size) lies entirely inside a single section's effective
//       readable extent (the same min(VirtualSize, SizeOfRawData) used by
//       GetEnclosingSectionHeader). A span that straddles section boundaries
//       is invalid as an RVA range — file bytes beyond the first section do
//       NOT represent the virtual continuation; they're the raw data of
//       later sections or padding. Without this check, a malformed
//       directory pointing just before a section's end would silently spill
//       into unrelated bytes.
//   (2) rva + size does not wrap around (integer-overflow guard).
//   (3) The resolved file offset + size fits inside the mapped file.
// ---------------------------------------------------------------------------
const BYTE* ClrSafeRvaRange(PE_EXE &pe, DWORD rva, DWORD size)
{
	if (rva == 0 || size == 0) return NULL;

	// (2) Overflow guard on the virtual-space range.
	if (rva > (DWORD) -1 - size) return NULL;
	DWORD endRva = rva + size;

	// (1) Single-section containment.
	PIMAGE_SECTION_HEADER sect = pe.GetEnclosingSectionHeader(rva);
	if (!sect) return NULL;
	DWORD cbOnDisk = min(sect->Misc.VirtualSize, sect->SizeOfRawData);
	if (cbOnDisk == 0) cbOnDisk = max(sect->Misc.VirtualSize, sect->SizeOfRawData);
	if (cbOnDisk == 0) return NULL;
	DWORD sectStart = sect->VirtualAddress;
	if (sectStart > (DWORD) -1 - cbOnDisk) return NULL;  // overflow on section end
	DWORD sectEnd = sectStart + cbOnDisk;
	if (rva < sectStart || endRva > sectEnd) return NULL;

	// (3) File-bytes presence.
	PVOID p = pe.GetReadablePointerFromRVA(rva);
	if (!p || p == (PVOID) -1) return NULL;
	const BYTE* base = (const BYTE*) pe.GetBase();
	DWORD fileSize = pe.GetFileSize();
	if (!base || fileSize == 0) return NULL;
	if ((const BYTE*) p < base) return NULL;
	size_t off = (size_t) ((const BYTE*) p - base);
	if (off > (size_t) fileSize) return NULL;
	if ((size_t) size > (size_t) fileSize - off) return NULL;

	return (const BYTE*) p;
}
