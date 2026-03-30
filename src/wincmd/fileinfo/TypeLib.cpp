// TypeLib.cpp: implementation of the CTypeLib class.
// Code taken from OLEVIEW: ActiveX Object Viewer
// http://msdn.microsoft.com/library/devprods/vs6/visualc/vcsample/_sample_mfc_oleview.htm

#include "stdafx.h"
#include "TypeLib.h"
#include <comutil.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define _com_issue_error(hr) return hr

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTypeLib::CTypeLib()
{
	ASSERT(FALSE);
}

CTypeLib::~CTypeLib()
{
}

#define MAX_NAMES	64 // Max parameters to a function

#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif
LPCTSTR AFXAPI _GetScodeString(SCODE sc)
{
	struct SCODE_ENTRY
	{
		SCODE sc;
		LPCTSTR lpszName;
	};
	#define MAKE_SCODE_ENTRY(sc)    { sc, _T(#sc) }
	static const SCODE_ENTRY scNameTable[] =
	{
		MAKE_SCODE_ENTRY(S_OK),
		MAKE_SCODE_ENTRY(S_FALSE),

		MAKE_SCODE_ENTRY(CACHE_S_FORMATETC_NOTSUPPORTED),
		MAKE_SCODE_ENTRY(CACHE_S_SAMECACHE),
		MAKE_SCODE_ENTRY(CACHE_S_SOMECACHES_NOTUPDATED),
		MAKE_SCODE_ENTRY(CONVERT10_S_NO_PRESENTATION),
		MAKE_SCODE_ENTRY(DATA_S_SAMEFORMATETC),
		MAKE_SCODE_ENTRY(DRAGDROP_S_CANCEL),
		MAKE_SCODE_ENTRY(DRAGDROP_S_DROP),
		MAKE_SCODE_ENTRY(DRAGDROP_S_USEDEFAULTCURSORS),
		MAKE_SCODE_ENTRY(INPLACE_S_TRUNCATED),
		MAKE_SCODE_ENTRY(MK_S_HIM),
		MAKE_SCODE_ENTRY(MK_S_ME),
		MAKE_SCODE_ENTRY(MK_S_MONIKERALREADYREGISTERED),
		MAKE_SCODE_ENTRY(MK_S_REDUCED_TO_SELF),
		MAKE_SCODE_ENTRY(MK_S_US),
		MAKE_SCODE_ENTRY(OLE_S_MAC_CLIPFORMAT),
		MAKE_SCODE_ENTRY(OLE_S_STATIC),
		MAKE_SCODE_ENTRY(OLE_S_USEREG),
		MAKE_SCODE_ENTRY(OLEOBJ_S_CANNOT_DOVERB_NOW),
		MAKE_SCODE_ENTRY(OLEOBJ_S_INVALIDHWND),
		MAKE_SCODE_ENTRY(OLEOBJ_S_INVALIDVERB),
		MAKE_SCODE_ENTRY(OLEOBJ_S_LAST),
		MAKE_SCODE_ENTRY(STG_S_CONVERTED),
		MAKE_SCODE_ENTRY(VIEW_S_ALREADY_FROZEN),

		MAKE_SCODE_ENTRY(E_UNEXPECTED),
		MAKE_SCODE_ENTRY(E_NOTIMPL),
		MAKE_SCODE_ENTRY(E_OUTOFMEMORY),
		MAKE_SCODE_ENTRY(E_INVALIDARG),
		MAKE_SCODE_ENTRY(E_NOINTERFACE),
		MAKE_SCODE_ENTRY(E_POINTER),
		MAKE_SCODE_ENTRY(E_HANDLE),
		MAKE_SCODE_ENTRY(E_ABORT),
		MAKE_SCODE_ENTRY(E_FAIL),
		MAKE_SCODE_ENTRY(E_ACCESSDENIED),

		MAKE_SCODE_ENTRY(CACHE_E_NOCACHE_UPDATED),
		MAKE_SCODE_ENTRY(CLASS_E_CLASSNOTAVAILABLE),
		MAKE_SCODE_ENTRY(CLASS_E_NOAGGREGATION),
		MAKE_SCODE_ENTRY(CLIPBRD_E_BAD_DATA),
		MAKE_SCODE_ENTRY(CLIPBRD_E_CANT_CLOSE),
		MAKE_SCODE_ENTRY(CLIPBRD_E_CANT_EMPTY),
		MAKE_SCODE_ENTRY(CLIPBRD_E_CANT_OPEN),
		MAKE_SCODE_ENTRY(CLIPBRD_E_CANT_SET),
		MAKE_SCODE_ENTRY(CO_E_ALREADYINITIALIZED),
		MAKE_SCODE_ENTRY(CO_E_APPDIDNTREG),
		MAKE_SCODE_ENTRY(CO_E_APPNOTFOUND),
		MAKE_SCODE_ENTRY(CO_E_APPSINGLEUSE),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(CO_E_BAD_PATH),
		#endif
		MAKE_SCODE_ENTRY(CO_E_CANTDETERMINECLASS),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(CO_E_CLASS_CREATE_FAILED),
		#endif
		MAKE_SCODE_ENTRY(CO_E_CLASSSTRING),
		MAKE_SCODE_ENTRY(CO_E_DLLNOTFOUND),
		MAKE_SCODE_ENTRY(CO_E_ERRORINAPP),
		MAKE_SCODE_ENTRY(CO_E_ERRORINDLL),
		MAKE_SCODE_ENTRY(CO_E_IIDSTRING),
		MAKE_SCODE_ENTRY(CO_E_NOTINITIALIZED),
		MAKE_SCODE_ENTRY(CO_E_OBJISREG),
		MAKE_SCODE_ENTRY(CO_E_OBJNOTCONNECTED),
		MAKE_SCODE_ENTRY(CO_E_OBJNOTREG),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(CO_E_OBJSRV_RPC_FAILURE),
		MAKE_SCODE_ENTRY(CO_E_SCM_ERROR),
		MAKE_SCODE_ENTRY(CO_E_SCM_RPC_FAILURE),
		MAKE_SCODE_ENTRY(CO_E_SERVER_EXEC_FAILURE),
		MAKE_SCODE_ENTRY(CO_E_SERVER_STOPPING),
		#endif
		MAKE_SCODE_ENTRY(CO_E_WRONGOSFORAPP),
		MAKE_SCODE_ENTRY(CONVERT10_E_OLESTREAM_BITMAP_TO_DIB),
		MAKE_SCODE_ENTRY(CONVERT10_E_OLESTREAM_FMT),
		MAKE_SCODE_ENTRY(CONVERT10_E_OLESTREAM_GET),
		MAKE_SCODE_ENTRY(CONVERT10_E_OLESTREAM_PUT),
		MAKE_SCODE_ENTRY(CONVERT10_E_STG_DIB_TO_BITMAP),
		MAKE_SCODE_ENTRY(CONVERT10_E_STG_FMT),
		MAKE_SCODE_ENTRY(CONVERT10_E_STG_NO_STD_STREAM),
		MAKE_SCODE_ENTRY(DISP_E_ARRAYISLOCKED),
		MAKE_SCODE_ENTRY(DISP_E_BADCALLEE),
		MAKE_SCODE_ENTRY(DISP_E_BADINDEX),
		MAKE_SCODE_ENTRY(DISP_E_BADPARAMCOUNT),
		MAKE_SCODE_ENTRY(DISP_E_BADVARTYPE),
		MAKE_SCODE_ENTRY(DISP_E_EXCEPTION),
		MAKE_SCODE_ENTRY(DISP_E_MEMBERNOTFOUND),
		MAKE_SCODE_ENTRY(DISP_E_NONAMEDARGS),
		MAKE_SCODE_ENTRY(DISP_E_NOTACOLLECTION),
		MAKE_SCODE_ENTRY(DISP_E_OVERFLOW),
		MAKE_SCODE_ENTRY(DISP_E_PARAMNOTFOUND),
		MAKE_SCODE_ENTRY(DISP_E_PARAMNOTOPTIONAL),
		MAKE_SCODE_ENTRY(DISP_E_TYPEMISMATCH),
		MAKE_SCODE_ENTRY(DISP_E_UNKNOWNINTERFACE),
		MAKE_SCODE_ENTRY(DISP_E_UNKNOWNLCID),
		MAKE_SCODE_ENTRY(DISP_E_UNKNOWNNAME),
		MAKE_SCODE_ENTRY(DRAGDROP_E_ALREADYREGISTERED),
		MAKE_SCODE_ENTRY(DRAGDROP_E_INVALIDHWND),
		MAKE_SCODE_ENTRY(DRAGDROP_E_NOTREGISTERED),
		MAKE_SCODE_ENTRY(DV_E_CLIPFORMAT),
		MAKE_SCODE_ENTRY(DV_E_DVASPECT),
		MAKE_SCODE_ENTRY(DV_E_DVTARGETDEVICE),
		MAKE_SCODE_ENTRY(DV_E_DVTARGETDEVICE_SIZE),
		MAKE_SCODE_ENTRY(DV_E_FORMATETC),
		MAKE_SCODE_ENTRY(DV_E_LINDEX),
		MAKE_SCODE_ENTRY(DV_E_NOIVIEWOBJECT),
		MAKE_SCODE_ENTRY(DV_E_STATDATA),
		MAKE_SCODE_ENTRY(DV_E_STGMEDIUM),
		MAKE_SCODE_ENTRY(DV_E_TYMED),
		MAKE_SCODE_ENTRY(INPLACE_E_NOTOOLSPACE),
		MAKE_SCODE_ENTRY(INPLACE_E_NOTUNDOABLE),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(MEM_E_INVALID_LINK),
		MAKE_SCODE_ENTRY(MEM_E_INVALID_ROOT),
		MAKE_SCODE_ENTRY(MEM_E_INVALID_SIZE),
		#endif
		MAKE_SCODE_ENTRY(MK_E_CANTOPENFILE),
		MAKE_SCODE_ENTRY(MK_E_CONNECTMANUALLY),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(MK_E_ENUMERATION_FAILED),
		#endif
		MAKE_SCODE_ENTRY(MK_E_EXCEEDEDDEADLINE),
		MAKE_SCODE_ENTRY(MK_E_INTERMEDIATEINTERFACENOTSUPPORTED),
		MAKE_SCODE_ENTRY(MK_E_INVALIDEXTENSION),
		MAKE_SCODE_ENTRY(MK_E_MUSTBOTHERUSER),
		MAKE_SCODE_ENTRY(MK_E_NEEDGENERIC),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(MK_E_NO_NORMALIZED),
		#endif
		MAKE_SCODE_ENTRY(MK_E_NOINVERSE),
		MAKE_SCODE_ENTRY(MK_E_NOOBJECT),
		MAKE_SCODE_ENTRY(MK_E_NOPREFIX),
		MAKE_SCODE_ENTRY(MK_E_NOSTORAGE),
		MAKE_SCODE_ENTRY(MK_E_NOTBINDABLE),
		MAKE_SCODE_ENTRY(MK_E_NOTBOUND),
		MAKE_SCODE_ENTRY(MK_E_SYNTAX),
		MAKE_SCODE_ENTRY(MK_E_UNAVAILABLE),
		MAKE_SCODE_ENTRY(OLE_E_ADVF),
		MAKE_SCODE_ENTRY(OLE_E_ADVISENOTSUPPORTED),
		MAKE_SCODE_ENTRY(OLE_E_BLANK),
		MAKE_SCODE_ENTRY(OLE_E_CANT_BINDTOSOURCE),
		MAKE_SCODE_ENTRY(OLE_E_CANT_GETMONIKER),
		MAKE_SCODE_ENTRY(OLE_E_CANTCONVERT),
		MAKE_SCODE_ENTRY(OLE_E_CLASSDIFF),
		MAKE_SCODE_ENTRY(OLE_E_ENUM_NOMORE),
		MAKE_SCODE_ENTRY(OLE_E_INVALIDHWND),
		MAKE_SCODE_ENTRY(OLE_E_INVALIDRECT),
		MAKE_SCODE_ENTRY(OLE_E_NOCACHE),
		MAKE_SCODE_ENTRY(OLE_E_NOCONNECTION),
		MAKE_SCODE_ENTRY(OLE_E_NOSTORAGE),
		MAKE_SCODE_ENTRY(OLE_E_NOT_INPLACEACTIVE),
		MAKE_SCODE_ENTRY(OLE_E_NOTRUNNING),
		MAKE_SCODE_ENTRY(OLE_E_OLEVERB),
		MAKE_SCODE_ENTRY(OLE_E_PROMPTSAVECANCELLED),
		MAKE_SCODE_ENTRY(OLE_E_STATIC),
		MAKE_SCODE_ENTRY(OLE_E_WRONGCOMPOBJ),
		MAKE_SCODE_ENTRY(OLEOBJ_E_INVALIDVERB),
		MAKE_SCODE_ENTRY(OLEOBJ_E_NOVERBS),
		MAKE_SCODE_ENTRY(REGDB_E_CLASSNOTREG),
		MAKE_SCODE_ENTRY(REGDB_E_IIDNOTREG),
		MAKE_SCODE_ENTRY(REGDB_E_INVALIDVALUE),
		MAKE_SCODE_ENTRY(REGDB_E_KEYMISSING),
		MAKE_SCODE_ENTRY(REGDB_E_READREGDB),
		MAKE_SCODE_ENTRY(REGDB_E_WRITEREGDB),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(RPC_E_ATTEMPTED_MULTITHREAD),
		#endif
		MAKE_SCODE_ENTRY(RPC_E_CALL_CANCELED),
		MAKE_SCODE_ENTRY(RPC_E_CALL_REJECTED),
		MAKE_SCODE_ENTRY(RPC_E_CANTCALLOUT_AGAIN),
		MAKE_SCODE_ENTRY(RPC_E_CANTCALLOUT_INASYNCCALL),
		MAKE_SCODE_ENTRY(RPC_E_CANTCALLOUT_INEXTERNALCALL),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(RPC_E_CANTCALLOUT_ININPUTSYNCCALL),
		#endif
		MAKE_SCODE_ENTRY(RPC_E_CANTPOST_INSENDCALL),
		MAKE_SCODE_ENTRY(RPC_E_CANTTRANSMIT_CALL),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(RPC_E_CHANGED_MODE),
		#endif
		MAKE_SCODE_ENTRY(RPC_E_CLIENT_CANTMARSHAL_DATA),
		MAKE_SCODE_ENTRY(RPC_E_CLIENT_CANTUNMARSHAL_DATA),
		MAKE_SCODE_ENTRY(RPC_E_CLIENT_DIED),
		MAKE_SCODE_ENTRY(RPC_E_CONNECTION_TERMINATED),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(RPC_E_DISCONNECTED),
		MAKE_SCODE_ENTRY(RPC_E_FAULT),
		MAKE_SCODE_ENTRY(RPC_E_INVALID_CALLDATA),
		#endif
		MAKE_SCODE_ENTRY(RPC_E_INVALID_DATA),
		MAKE_SCODE_ENTRY(RPC_E_INVALID_DATAPACKET),
		MAKE_SCODE_ENTRY(RPC_E_INVALID_PARAMETER),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(RPC_E_INVALIDMETHOD),
		MAKE_SCODE_ENTRY(RPC_E_NOT_REGISTERED),
		MAKE_SCODE_ENTRY(RPC_E_OUT_OF_RESOURCES),
		MAKE_SCODE_ENTRY(RPC_E_RETRY),
		#endif
		MAKE_SCODE_ENTRY(RPC_E_SERVER_CANTMARSHAL_DATA),
		MAKE_SCODE_ENTRY(RPC_E_SERVER_CANTUNMARSHAL_DATA),
		MAKE_SCODE_ENTRY(RPC_E_SERVER_DIED),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(RPC_E_SERVER_DIED_DNE),
		MAKE_SCODE_ENTRY(RPC_E_SERVERCALL_REJECTED),
		MAKE_SCODE_ENTRY(RPC_E_SERVERCALL_RETRYLATER),
		MAKE_SCODE_ENTRY(RPC_E_SERVERFAULT),
		MAKE_SCODE_ENTRY(RPC_E_SYS_CALL_FAILED),
		MAKE_SCODE_ENTRY(RPC_E_THREAD_NOT_INIT),
		MAKE_SCODE_ENTRY(RPC_E_WRONG_THREAD),
		#endif
		MAKE_SCODE_ENTRY(RPC_E_UNEXPECTED),
		MAKE_SCODE_ENTRY(STG_E_ABNORMALAPIEXIT),
		MAKE_SCODE_ENTRY(STG_E_ACCESSDENIED),
		MAKE_SCODE_ENTRY(STG_E_CANTSAVE),
		MAKE_SCODE_ENTRY(STG_E_DISKISWRITEPROTECTED),
		MAKE_SCODE_ENTRY(STG_E_EXTANTMARSHALLINGS),
		MAKE_SCODE_ENTRY(STG_E_FILEALREADYEXISTS),
		MAKE_SCODE_ENTRY(STG_E_FILENOTFOUND),
		MAKE_SCODE_ENTRY(STG_E_INSUFFICIENTMEMORY),
		MAKE_SCODE_ENTRY(STG_E_INUSE),
		MAKE_SCODE_ENTRY(STG_E_INVALIDFLAG),
		MAKE_SCODE_ENTRY(STG_E_INVALIDFUNCTION),
		MAKE_SCODE_ENTRY(STG_E_INVALIDHANDLE),
		MAKE_SCODE_ENTRY(STG_E_INVALIDHEADER),
		MAKE_SCODE_ENTRY(STG_E_INVALIDNAME),
		MAKE_SCODE_ENTRY(STG_E_INVALIDPARAMETER),
		MAKE_SCODE_ENTRY(STG_E_INVALIDPOINTER),
		MAKE_SCODE_ENTRY(STG_E_LOCKVIOLATION),
		MAKE_SCODE_ENTRY(STG_E_MEDIUMFULL),
		MAKE_SCODE_ENTRY(STG_E_NOMOREFILES),
		MAKE_SCODE_ENTRY(STG_E_NOTCURRENT),
		MAKE_SCODE_ENTRY(STG_E_NOTFILEBASEDSTORAGE),
		MAKE_SCODE_ENTRY(STG_E_OLDDLL),
		MAKE_SCODE_ENTRY(STG_E_OLDFORMAT),
		MAKE_SCODE_ENTRY(STG_E_PATHNOTFOUND),
		MAKE_SCODE_ENTRY(STG_E_READFAULT),
		MAKE_SCODE_ENTRY(STG_E_REVERTED),
		MAKE_SCODE_ENTRY(STG_E_SEEKERROR),
		MAKE_SCODE_ENTRY(STG_E_SHAREREQUIRED),
		MAKE_SCODE_ENTRY(STG_E_SHAREVIOLATION),
		MAKE_SCODE_ENTRY(STG_E_TOOMANYOPENFILES),
		MAKE_SCODE_ENTRY(STG_E_UNIMPLEMENTEDFUNCTION),
		MAKE_SCODE_ENTRY(STG_E_UNKNOWN),
		MAKE_SCODE_ENTRY(STG_E_WRITEFAULT),
		MAKE_SCODE_ENTRY(TYPE_E_AMBIGUOUSNAME),
		MAKE_SCODE_ENTRY(TYPE_E_BADMODULEKIND),
		MAKE_SCODE_ENTRY(TYPE_E_BUFFERTOOSMALL),
		MAKE_SCODE_ENTRY(TYPE_E_CANTCREATETMPFILE),
		MAKE_SCODE_ENTRY(TYPE_E_CANTLOADLIBRARY),
		MAKE_SCODE_ENTRY(TYPE_E_CIRCULARTYPE),
		MAKE_SCODE_ENTRY(TYPE_E_DLLFUNCTIONNOTFOUND),
		MAKE_SCODE_ENTRY(TYPE_E_DUPLICATEID),
		MAKE_SCODE_ENTRY(TYPE_E_ELEMENTNOTFOUND),
		MAKE_SCODE_ENTRY(TYPE_E_INCONSISTENTPROPFUNCS),
		MAKE_SCODE_ENTRY(TYPE_E_INVALIDSTATE),
		MAKE_SCODE_ENTRY(TYPE_E_INVDATAREAD),
		MAKE_SCODE_ENTRY(TYPE_E_IOERROR),
		MAKE_SCODE_ENTRY(TYPE_E_LIBNOTREGISTERED),
		MAKE_SCODE_ENTRY(TYPE_E_NAMECONFLICT),
		MAKE_SCODE_ENTRY(TYPE_E_OUTOFBOUNDS),
		MAKE_SCODE_ENTRY(TYPE_E_QUALIFIEDNAMEDISALLOWED),
		MAKE_SCODE_ENTRY(TYPE_E_REGISTRYACCESS),
		MAKE_SCODE_ENTRY(TYPE_E_SIZETOOBIG),
		MAKE_SCODE_ENTRY(TYPE_E_TYPEMISMATCH),
		MAKE_SCODE_ENTRY(TYPE_E_UNDEFINEDTYPE),
		MAKE_SCODE_ENTRY(TYPE_E_UNKNOWNLCID),
		MAKE_SCODE_ENTRY(TYPE_E_UNSUPFORMAT),
		MAKE_SCODE_ENTRY(TYPE_E_WRONGTYPEKIND),
		MAKE_SCODE_ENTRY(VIEW_E_DRAW),
	};
	#undef MAKE_SCODE_ENTRY

	// look for it in the table
	for (int i = 0; i < _countof(scNameTable); i++)
	{
		if (sc == scNameTable[i].sc)
			return scNameTable[i].lpszName;
	}
	return NULL;    // not found
}

// Write string s with no indent and no CR
#define WRITE( s )          WriteLine( pstm, s, 0, FALSE )

// Write string s with indent and no CR
#define WRITE1( s )         WriteLine( pstm, s, uiIndent, FALSE )
#define WRITE2( s, ui )     WriteLine( pstm, s, uiIndent+ui, FALSE )

// Write string s with indent and CR
#define WRITELINE( s )      WriteLine( pstm, s, uiIndent, TRUE )
#define WRITELINE2( s, ui ) WriteLine( pstm, s, uiIndent+ui, TRUE )
#define WRITECR( s )        WriteLine( pstm, s, 0, TRUE )

#define WRITERAW( p, len )     WriteRaw( pstm, p, len )
#define WRITEBSTR( p )         WriteBSTR( pstm, p )

inline void WriteRaw( IStream* pstm, const void* pv, UINT cb )
{
	HRESULT hr ;
	if (FAILED(hr = pstm->Write( pv, cb, NULL )))
		return;
//		_com_issue_error( hr ) ;
}

inline void WriteLine( IStream* pstm, const CString &rstr, UINT uiIndent, BOOL fNewLine )
{
	while(uiIndent--)
	{
		WriteRaw( pstm, _T("    "), 4 * sizeof(TCHAR));
	}

	WriteRaw( pstm, rstr, rstr.GetLength() * sizeof(TCHAR)) ;

	if (fNewLine)
		WriteRaw(pstm, _T("\r\n"), 2 * sizeof(TCHAR)) ;
}

inline void WriteBSTR( IStream* pstm, BSTR bstr )
{
	UINT len = ::SysStringLen(bstr) ;
	if (!len)
		return ;

	USES_CONVERSION;
	LPTSTR lpszSource = OLE2T(bstr);
	TCHAR *pstrTemp = new TCHAR[((len + 1) * sizeof(TCHAR)) * 2];
	LPTSTR lpD, lpS = lpszSource ;

	lpD = pstrTemp;
	for (UINT n = 0 ; n < len ; n++)
	{
		if (!isprint(*lpS) || (*lpS) == '\"' || (*lpS) == '\\')
		{
			// \"  \\ \a  \b  \f  \n  \r  \t  \v
			*lpD++ = '\\' ;
			switch(*lpS)
			{
			case '\"':
				*lpD++ = '\"' ;
			break ;
			case '\\':
				*lpD++ = '\\' ;
			break ;
			case '\a':
				*lpD++ = 'a' ;
			break ;
			case '\b':
				*lpD++ = 'b' ;
			break ;
			case '\f':
				*lpD++ = 'f' ;
			break ;
			case '\n':
				*lpD++ = 'n' ;
			break ;
			case '\r':
				*lpD++ = 'r' ;
			break ;
			case '\t':
				*lpD++ = 't' ;
			break ;
			case '\v':
				*lpD++ = 'v' ;
			break ;
			case '\0':
				*lpD++ = '0' ;
			break ;
			default:
				lpD += wsprintf( lpD, _T("x%02X"), (UINT)*lpS );
			break ;
			}
			lpS++;
		}
		else
			*lpD++ = *lpS++;
	}
	*lpD = '\0';

	WriteRaw( pstm, pstrTemp, lstrlen(pstrTemp)*sizeof(TCHAR) ) ;
	delete []pstrTemp ;
}

HRESULT  DeCompileTypeLib( IStream* pstm, ITypeLib* ptlb, UINT uiIndent  = 0 ) ;
HRESULT  DeCompileTypedef( IStream* pstm, ITypeInfo* pti, UINT uiIndent  = 0 ) ;
HRESULT  DeCompileModule( IStream* pstm, ITypeInfo* pti, UINT uiIndent  = 0 ) ;
HRESULT  DeCompileInterface( IStream* pstm, ITypeInfo* pti, UINT uiIndent  = 0 ) ;
HRESULT  DeCompileDispinterface( IStream* pstm, ITypeInfo* pti, UINT uiIndent  = 0 ) ;
HRESULT  DeCompileCoClass( IStream* pstm, ITypeInfo* pti, UINT uiIndent  = 0 ) ;
HRESULT  DeCompileFunc( IStream* pstm, ITypeInfo* pti, MEMBERID memid, UINT uiIndent  = 0 ) ;
HRESULT  DumpFunc( IStream* pstm, ITypeInfo* pti, TYPEATTR* pattr, MEMBERID memid, UINT uiIndent  = 0 ) ;
HRESULT  DeCompileVar( IStream* pstm, ITypeInfo* pti, MEMBERID memid, UINT uiIndent  = 0 ) ;
HRESULT  DumpVar( IStream* pstm, ITypeInfo* pti, TYPEATTR* pattr, MEMBERID memid, UINT uiIndent  = 0 ) ;
HRESULT  DeCompileConst( IStream* pstm, ITypeInfo* pti, MEMBERID memid, UINT uiIndent  = 0, BOOL fConst = TRUE  ) ;
HRESULT  DumpConst( IStream* pstm, ITypeInfo* pti, TYPEATTR* pattr, MEMBERID memid, UINT uiIndent = 0, BOOL fConst = TRUE ) ;

static TCHAR* BASED_CODE g_rgszVT[] =
{
	_T("void"),             //VT_EMPTY           = 0,   /* [V]   [P]  nothing                     */
	_T("null"),             //VT_NULL            = 1,   /* [V]        SQL style Null              */
	_T("short"),            //VT_I2              = 2,   /* [V][T][P]  2 byte signed int           */
	_T("long"),             //VT_I4              = 3,   /* [V][T][P]  4 byte signed int           */
	_T("single"),           //VT_R4              = 4,   /* [V][T][P]  4 byte real                 */
	_T("double"),           //VT_R8              = 5,   /* [V][T][P]  8 byte real                 */
	_T("CURRENCY"),         //VT_CY              = 6,   /* [V][T][P]  currency                    */
	_T("DATE"),             //VT_DATE            = 7,   /* [V][T][P]  date                        */
	_T("BSTR"),             //VT_BSTR            = 8,   /* [V][T][P]  binary string               */
	_T("IDispatch*"),       //VT_DISPATCH        = 9,   /* [V][T]     IDispatch FAR*              */
	_T("SCODE"),            //VT_ERROR           = 10,  /* [V][T]     SCODE                       */
	_T("boolean"),          //VT_BOOL            = 11,  /* [V][T][P]  True=-1, False=0            */
	_T("VARIANT"),          //VT_VARIANT         = 12,  /* [V][T][P]  VARIANT FAR*                */
	_T("IUnknown*"),        //VT_UNKNOWN         = 13,  /* [V][T]     IUnknown FAR*               */
	_T("wchar_t"),          //VT_WBSTR           = 14,  /* [V][T]     wide binary string          */
	_T(""),                 //                   = 15,
	_T("char"),             //VT_I1              = 16,  /*    [T]     signed char                 */
	_T("unsigned char"),             //VT_UI1             = 17,  /*    [T]     unsigned char               */
	_T("unsigned short"),           //VT_UI2             = 18,  /*    [T]     unsigned short              */
	_T("unsigned long"),            //VT_UI4             = 19,  /*    [T]     unsigned short              */
	_T("int64"),            //VT_I8              = 20,  /*    [T][P]  signed 64-bit int           */
	_T("uint64"),           //VT_UI8             = 21,  /*    [T]     unsigned 64-bit int         */
	_T("int"),              //VT_INT             = 22,  /*    [T]     signed machine int          */
	_T("unsigned int"),             //VT_UINT            = 23,  /*    [T]     unsigned machine int        */
	_T("void"),             //VT_VOID            = 24,  /*    [T]     C style void                */
	_T("HRESULT"),          //VT_HRESULT         = 25,  /*    [T]                                 */
	_T("PTR"),              //VT_PTR             = 26,  /*    [T]     pointer type                */
	_T("SAFEARRAY"),        //VT_SAFEARRAY       = 27,  /*    [T]     (use VT_ARRAY in VARIANT)   */
	_T("CARRAY"),           //VT_CARRAY          = 28,  /*    [T]     C style array               */
	_T("USERDEFINED"),      //VT_USERDEFINED     = 29,  /*    [T]     user defined type         */
	_T("LPSTR"),            //VT_LPSTR           = 30,  /*    [T][P]  null terminated string      */
	_T("LPWSTR"),           //VT_LPWSTR          = 31,  /*    [T][P]  wide null terminated string */
	_T(""),                 //                   = 32,
	_T(""),                 //                   = 33,
	_T(""),                 //                   = 34,
	_T(""),                 //                   = 35,
	_T(""),                 //                   = 36,
	_T(""),                 //                   = 37,
	_T(""),                 //                   = 38,
	_T(""),                 //                   = 39,
	_T(""),                 //                   = 40,
	_T(""),                 //                   = 41,
	_T(""),                 //                   = 42,
	_T(""),                 //                   = 43,
	_T(""),                 //                   = 44,
	_T(""),                 //                   = 45,
	_T(""),                 //                   = 46,
	_T(""),                 //                   = 47,
	_T(""),                 //                   = 48,
	_T(""),                 //                   = 49,
	_T(""),                 //                   = 50,
	_T(""),                 //                   = 51,
	_T(""),                 //                   = 52,
	_T(""),                 //                   = 53,
	_T(""),                 //                   = 54,
	_T(""),                 //                   = 55,
	_T(""),                 //                   = 56,
	_T(""),                 //                   = 57,
	_T(""),                 //                   = 58,
	_T(""),                 //                   = 59,
	_T(""),                 //                   = 60,
	_T(""),                 //                   = 61,
	_T(""),                 //                   = 62,
	_T(""),                 //                   = 63,
	_T("FILETIME"),         //VT_FILETIME        = 64,  /*       [P]  FILETIME                    */
	_T("BLOB"),             //VT_BLOB            = 65,  /*       [P]  Length prefixed bytes       */
	_T("STREAM"),           //VT_STREAM          = 66,  /*       [P]  Name of the stream follows  */
	_T("STORAGE"),          //VT_STORAGE         = 67,  /*       [P]  Name of the storage follows */
	_T("STREAMED_OBJECT"),  //VT_STREAMED_OBJECT = 68,  /*       [P]  Stream contains an object   */
	_T("STORED_OBJECT"),    //VT_STORED_OBJECT   = 69,  /*       [P]  Storage contains an object  */
	_T("BLOB_OBJECT"),      //VT_BLOB_OBJECT     = 70,  /*       [P]  Blob contains an object     */
	_T("CF"),               //VT_CF              = 71,  /*       [P]  Clipboard format            */
	_T("CLSID"),            //VT_CLSID           = 72   /*       [P]  A Class ID                  */
};

CString VTtoString( VARTYPE vt )
{
	CString str ;
	vt &= ~0xF000 ;
	if (vt <= VT_CLSID)
	   str = g_rgszVT[vt]  ;
	else
	   str = "BAD VARTYPE" ;

	return str ;
}

CString TYPEDESCtoString( ITypeInfo* pti, TYPEDESC* ptdesc )
{
	CString str ;

	if (ptdesc->vt == VT_PTR)
	{
		// ptdesc->lptdesc points to a TYPEDESC that specifies the thing pointed to

		str = TYPEDESCtoString( pti, ptdesc->lptdesc ) ;
		str += "*" ;
		return str.AllocSysString() ;
	}

	if ((ptdesc->vt & 0x0FFF) == VT_CARRAY)
	{
		// ptdesc->lpadesc points to an ARRAYDESC
		str = TYPEDESCtoString( pti, &ptdesc->lpadesc->tdescElem ) ;

		// Allocate cDims * lstrlen("[123456]")
		CString strTemp ;
		for (USHORT n = 0 ; n < ptdesc->lpadesc->cDims ; n++)
		{
			strTemp.Format( _T("[%d]"), ptdesc->lpadesc->rgbounds[n].cElements ) ;
			str += strTemp ;
		}
		return str ;
	}

	if ((ptdesc->vt & 0x0FFF) == VT_SAFEARRAY)
	{
		str = "SAFEARRAY(" + TYPEDESCtoString( pti, ptdesc->lptdesc ) + ")" ;
		return str ;
	}

	if (ptdesc->vt == VT_USERDEFINED)
	{
		// Use ptdesc->hreftype and pti->GetRefTypeInfo
		//
		ASSERT(pti) ;
		ITypeInfo* ptiRefType = NULL ;
		HRESULT hr = pti->GetRefTypeInfo( ptdesc->hreftype, &ptiRefType ) ;
		if (SUCCEEDED(hr))
		{
			BSTR            bstrName = NULL ;
			BSTR            bstrDoc = NULL ;
			BSTR            bstrHelp = NULL ;
			DWORD           dwHelpID ;
			hr = ptiRefType->GetDocumentation( MEMBERID_NIL, &bstrName, &bstrDoc, &dwHelpID, &bstrHelp ) ;
			if (FAILED(hr))
				return _T("ITypeInfo::GetDocumentation() failed in TYPEDESCToString") ;

			str = bstrName ;

			SysFreeString(bstrName) ;
			SysFreeString( bstrDoc ) ;
			SysFreeString( bstrHelp ) ;

			ptiRefType->Release() ;
		}
		else
			return _T("<GetRefTypeInfo failed>") ;

		return str ;
	}
	return VTtoString( ptdesc->vt );
}

HRESULT DumpVar( IStream* pstm, ITypeInfo* pti, TYPEATTR* pattr, MEMBERID memid, UINT uiIndent /* = 0 */)
{
	HRESULT     hr = S_OK ;
	VARDESC*    pvardesc = NULL ;
	BSTR            rgbstrNames[1] ;
	BSTR            bstrName = NULL ;
	BSTR            bstrDoc = NULL ;
	BSTR            bstrHelp = NULL ;
	DWORD           dwHelpID ;
	ASSERT(pti) ;

	TRY
	{
		if (FAILED(hr = pti->GetVarDesc( memid, &pvardesc )))
			_com_issue_error( hr ) ;

		ASSERT(pvardesc->varkind != VAR_CONST) ;    // must not be a const

		// If pattr->typekind == TKIND_RECORD (struct) || TKIND_UNION
		//    type name[array];
		//
		// If pattr->typekind == TKIND_DISPATCH (dispinterface)
		//    [id(<id>), bindable, defaultbind, displaybind, readonly,
		//      requestedit, source, hidden, helpstring("<helpstring>"),
		//      helpcontext(<id>)] type name;
		//
		BOOL    fAttributes = FALSE ;
		WRITE1("") ;    // indent
		if (pattr->typekind == TKIND_DISPATCH)
		{
			CString str ;
			fAttributes = TRUE ;
			str.Format(_T("[id(%d)"), memid) ;
			WRITE(str) ;

			if (pvardesc->wVarFlags & VARFLAG_FREADONLY)
				WRITE(", readonly") ;
			if (pvardesc->wVarFlags & VARFLAG_FSOURCE)
				WRITE(", source") ;
			if (pvardesc->wVarFlags & VARFLAG_FBINDABLE)
				WRITE(", bindable") ;
			if (pvardesc->wVarFlags & VARFLAG_FREQUESTEDIT)
				WRITE(", requestedit") ;
			if (pvardesc->wVarFlags & VARFLAG_FDISPLAYBIND)
				WRITE(", displaybind") ;
			if (pvardesc->wVarFlags & VARFLAG_FDEFAULTBIND)
				WRITE(", defaultbind") ;
			if (pvardesc->wVarFlags & VARFLAG_FHIDDEN)
				WRITE(", hidden") ;
		}

		if (SUCCEEDED(pti->GetDocumentation( pvardesc->memid, &bstrName, &bstrDoc, &dwHelpID, &bstrHelp )))
		{
			CString str ;
			if (bstrDoc && *bstrDoc)
			{
				if (fAttributes == FALSE)
					WRITE("[") ;
				else
					WRITE(", ") ;
				fAttributes = TRUE ;
				USES_CONVERSION;
				LPCTSTR lpszDoc = OLE2CT(bstrDoc);
				::SysFreeString(bstrDoc);
				bstrDoc = NULL;
				str.Format( _T("helpstring(\"%s\")"), lpszDoc ) ;
				WRITE( str ) ;
				if (dwHelpID > 0)
				{
					str.Format( _T(", helpcontext(%#08.8x)"), dwHelpID ) ;
					WRITE( str ) ;
				}
			}
			else if (dwHelpID > 0)
			{
				if (fAttributes == FALSE)
					WRITE("[") ;
				else
					WRITE(", ") ;
				fAttributes = TRUE ;
				str.Format( _T("helpcontext(%#08.8x)"), dwHelpID ) ;
				WRITE( str ) ;
			}
		}

		if (fAttributes)
			WRITE("] ") ;
		UINT cNames ;
		if (FAILED( hr = pti->GetNames( pvardesc->memid, rgbstrNames, 1, (UINT FAR*)&cNames )))
			_com_issue_error( hr ) ;

		CString str ;
		if ((pvardesc->elemdescVar.tdesc.vt & 0x0FFF) == VT_CARRAY)
		{
			// type name[n]
			WRITE(TYPEDESCtoString( pti, &pvardesc->elemdescVar.tdesc.lpadesc->tdescElem )) ;
			WRITE(" ") ;
			if (rgbstrNames[0])
				WRITEBSTR(rgbstrNames[0]);
			else
				WRITE(_T("(nameless)")) ;
			// Allocate cDims * lstrlen("[123456]")
			for (USHORT n = 0 ; n < pvardesc->elemdescVar.tdesc.lpadesc->cDims ; n++)
			{
				str.Format( _T("[%d]"), pvardesc->elemdescVar.tdesc.lpadesc->rgbounds[n].cElements ) ;
				WRITE(str) ;
			}
		}
		else
		{
			WRITE(TYPEDESCtoString( pti, &pvardesc->elemdescVar.tdesc ) + _T(" "));
			if (rgbstrNames[0])
				WRITEBSTR(rgbstrNames[0]);
			else
				WRITE(_T("(nameless)")) ;
		}
		WRITECR(";") ;

		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;

		pti->ReleaseVarDesc( pvardesc ) ;
	}
	CATCH(CException, pException)
	{
		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;
		if (pvardesc)
			pti->ReleaseVarDesc( pvardesc ) ;

		THROW_LAST();
	}
	END_CATCH

	return hr ;
}

int StringFromGUID2T(REFGUID rguid, LPTSTR lpszGUID, int cbMax )
{
	USES_CONVERSION;
	OLECHAR* lpszOle = (OLECHAR*)_alloca(cbMax*sizeof(OLECHAR));
	int nCount = ::StringFromGUID2(rguid,lpszOle, cbMax*sizeof(OLECHAR));
	if (nCount == 0)
	{
		lpszGUID[0] = _T('\0');
		return 0; // buffer too small for the returned string
	}
	LPTSTR lpszRes = OLE2T(lpszOle);
	lstrcpy(lpszGUID, lpszRes);
	return 0;
}

HRESULT DeCompileTypedef( IStream* pstm, ITypeInfo* pti, UINT uiIndent /* = 0 */ )
{
	USES_CONVERSION;
	HRESULT     hr = S_OK ;
	TYPEATTR*   pattr = NULL ;
	BSTR            bstrName = NULL ;
	BSTR            bstrDoc = NULL ;
	BSTR            bstrHelp = NULL ;
	DWORD           dwHelpID ;

	WRITE1("typedef ") ;

	TRY
	{
		if (FAILED(hr = pti->GetTypeAttr( &pattr)))
			_com_issue_error( hr ) ;

		BOOL    fAttributes = FALSE ;      // any attributes?
		BOOL    fAttribute = FALSE ;       // at least one (insert ",")

		// Was 'uuid' specified?
		if (!IsEqualGUID( pattr->guid, GUID_NULL ))
		{
			TCHAR szGUID[64] ;
			StringFromGUID2T( pattr->guid, szGUID, sizeof(szGUID) ) ;
			fAttributes = TRUE ;
			WRITE("[uuid(") ;
			WRITE(szGUID) ;
			WRITE(")") ;
			fAttribute = TRUE ;
		}

		// was version specified
		if (pattr->wMajorVerNum || pattr->wMinorVerNum)
		{
			if (fAttributes == FALSE)
				WRITE("[") ;
			fAttributes = TRUE ;
			if (fAttribute)
				WRITE(", ") ;

			CString str ;
			str.Format(_T("version(%d.%d)"), pattr->wMajorVerNum, pattr->wMinorVerNum) ;
			WRITE(str) ;
			fAttribute = TRUE ;
		}

		if (SUCCEEDED(pti->GetDocumentation( MEMBERID_NIL, &bstrName, &bstrDoc, &dwHelpID, &bstrHelp )))
		{
			CString str ;
			if (bstrDoc && *bstrDoc)
			{
				if (fAttributes == FALSE) WRITE("[") ;
				fAttributes = TRUE ;
				if (fAttribute)
					WRITE(", ") ;
				LPCTSTR lpszDoc = OLE2CT(bstrDoc);
				::SysFreeString(bstrDoc);
				bstrDoc = NULL;

				str.Format( _T("helpstring(\"%s\")"), lpszDoc );
				WRITE( str ) ;
				if (dwHelpID > 0)
				{
					str.Format( _T(", helpcontext(%#08.8x)"), dwHelpID ) ;
					WRITE( str ) ;
				}
			}
			else if (dwHelpID > 0)
			{
				if (fAttributes == FALSE) WRITE("[") ;
				fAttributes = TRUE ;
				if (fAttribute)
					WRITE(", ") ;
				str.Format( _T("helpcontext(%#08.8x)"), dwHelpID ) ;
				WRITE( str ) ;
				fAttribute = TRUE ;
			}
		}

		if (pattr->typekind == TKIND_ALIAS)
		{
			if (fAttributes == FALSE) WRITE("[") ;
			fAttributes = TRUE ;
			if (fAttribute)
				WRITE(", ") ;
			WRITE("public") ;       // if it's in the tlb it had public
		}

		if (fAttributes)
			WRITE("] ") ;

		switch(pattr->typekind)
		{
		case TKIND_RECORD:
			#ifdef _WRITE_TAG
			WRITE("struct tag") ;
			WRITEBSTR(bstrName) ;
			#else
			WRITE("struct ") ;
			#endif
			WRITECR("\n    {" );
		break ;

		case TKIND_UNION:
			#ifdef _WRITE_TAG
			WRITE("union tag") ;
			WRITEBSTR(bstrName) ;
			#else
			WRITE("union ") ;
			#endif
			WRITECR("\n    {" );
		break ;

		case TKIND_ALIAS:  //typedef
			// write base type
			WRITE(TYPEDESCtoString( pti, &pattr->tdescAlias )) ;
			WRITE(" ") ;

			// write aliasname
		break ;

		case TKIND_ENUM:
			WRITECR("enum\n    {" );
		break ;

		default:
		ASSERT(0) ;
		break ;
		}

		if (pattr->typekind == TKIND_RECORD || pattr->typekind == TKIND_UNION)
		{
			for (UINT n = 0 ; n < pattr->cVars ; n++)
				DumpVar( pstm, pti, pattr, n, uiIndent + 1 ) ;
			WRITE1("} ");
		}

		if (pattr->typekind == TKIND_ENUM)
		{
			for (int n = 0 ; n < pattr->cVars ; n++)
			{
				DumpConst( pstm, pti, pattr, n, uiIndent + 1, FALSE ) ;
				if (n < pattr->cVars-1)
					WRITECR(",") ;
				else
					WRITECR("") ;
			}
			WRITE1("} ");
		}

		WRITEBSTR(bstrName) ;
		WRITECR(";") ;


		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;

		pti->ReleaseTypeAttr( pattr ) ;
	}
	CATCH(CException, pException)
	{
		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;
		if (pattr)
			pti->ReleaseTypeAttr( pattr ) ;

		THROW_LAST();
	}
	END_CATCH
	return hr ;
}

//  [
//    attributes
//  ]
//  module modulename {
//      elementlist
//  };
//  attributes are
//      uuid, version, helpstring, helpcontext, dllname
//      The dllname attribute is required.
//
//  [attributes] returntype [calling convention] funcname(params);
//  [attributes] const constname = constval;
//
HRESULT DeCompileModule( IStream* pstm, ITypeInfo* pti, UINT uiIndent /* = 0 */ )
{
	HRESULT     hr = S_OK ;
	TYPEATTR*   pattr = NULL ;
	BSTR            bstrName = NULL ;
	BSTR            bstrDoc = NULL ;
	BSTR            bstrHelp = NULL ;
	DWORD           dwHelpID ;

	TRY
	{
		if(FAILED(hr = pti->GetTypeAttr( &pattr)))
			_com_issue_error(hr) ;

		WRITELINE(_T("[")) ;
		WRITE1( _T("  dllname(")) ;
		try
		{
			if(FAILED(hr = pti->GetDllEntry(
				MEMBERID_NIL, INVOKE_FUNC, &bstrName, NULL, NULL))
				)
				_com_issue_error( hr ) ;
		}
		catch(...)
		{
			bstrName = ::SysAllocString(L"<No entry points were found>");
		}
		WRITEBSTR(bstrName) ;
		SysFreeString(bstrName) ;
		bstrName = NULL ;
		WRITE(_T(")")) ;

		// Was 'uuid' specified?
		if (!IsEqualGUID( pattr->guid, GUID_NULL ))
		{
			TCHAR szGUID[64] ;
			StringFromGUID2T( pattr->guid, szGUID, sizeof(szGUID) ) ;
			WRITECR(",") ;
			WRITE1("  uuid(") ;
			WRITE(szGUID) ;
			WRITE(")") ;
		}

		// was version specified
		if (pattr->wMajorVerNum || pattr->wMinorVerNum)
		{
			WRITECR(",") ;
			CString str ;
			str.Format(_T("  version(%d.%d)"), pattr->wMajorVerNum, pattr->wMinorVerNum) ;
			WRITE1(str) ;
		}

		if (SUCCEEDED(pti->GetDocumentation( MEMBERID_NIL, &bstrName, &bstrDoc, &dwHelpID, &bstrHelp )))
		{
			CString str ;
			if (bstrDoc && *bstrDoc)
			{
				WRITECR(",") ;
				USES_CONVERSION;
				LPCTSTR lpszDoc = OLE2CT(bstrDoc);
				::SysFreeString(bstrDoc);
				bstrDoc = NULL;
				str.Format( _T("  helpstring(\"%s\")"), lpszDoc ) ;
				WRITE1( str ) ;
				if (dwHelpID > 0)
				{
					WRITECR(",") ;
					str.Format( _T("  helpcontext(%#08.8x)"), dwHelpID ) ;
					WRITE1( str ) ;
				}
			}
			else if (dwHelpID > 0)
			{
				WRITECR(",") ;
				str.Format( _T("  helpcontext(%#08.8x)"), dwHelpID ) ;
				WRITE1( str ) ;
			}
		}

		WRITECR("") ;
		WRITELINE(_T("]")) ;
		WRITE1(_T("module ")) ;
		WRITEBSTR( bstrName ) ;
		WRITECR( _T("\n    {")) ;

		for (int n = 0 ; n < pattr->cFuncs ; n++)
			DumpFunc( pstm, pti, pattr, n, uiIndent + 1 ) ;

		for (int n = 0 ; n < pattr->cVars ; n++)
			DumpConst( pstm, pti, pattr, n, uiIndent + 1, TRUE ) ;

		WRITELINE("};") ;


		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;

		pti->ReleaseTypeAttr( pattr ) ;
	}
	CATCH(CException, pException)
	{
		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;
		if (pattr)
			pti->ReleaseTypeAttr( pattr ) ;

		THROW_LAST();
	}
	END_CATCH
	return hr ;
}

//  [
//    attributes
//  ]
//  interface interfacename  [:baseinterface] {
//      functionlist
//  };
//
//  attributes include source, default, and restricted
//
HRESULT DeCompileInterface( IStream* pstm, ITypeInfo* pti, UINT uiIndent /* = 0 */ )
{
	HRESULT     hr = S_OK ;
	TYPEATTR*   pattr = NULL ;
	BSTR            bstrName = NULL ;
	BSTR            bstrDoc = NULL ;
	BSTR            bstrHelp = NULL ;
	DWORD           dwHelpID ;
	ITypeInfo*      ptiImpl = NULL ;

	TRY
	{
		if (FAILED(hr = pti->GetTypeAttr( &pattr)))
			_com_issue_error( hr ) ;

		WRITELINE(_T("[")) ;
		WRITE1( _T("  odl")) ;

		TCHAR szGUID[64] ;
		StringFromGUID2T( pattr->guid, szGUID, sizeof(szGUID) ) ;
		WRITECR(",") ;
		WRITE1("  uuid(") ;
		WRITE(szGUID) ;
		WRITE(")") ;

		// was version specified
		if (pattr->wMajorVerNum || pattr->wMinorVerNum)
		{
			WRITECR(",") ;
			CString str ;
			str.Format(_T("  version(%d.%d)"), pattr->wMajorVerNum, pattr->wMinorVerNum) ;
			WRITE1(str) ;
		}

		if (SUCCEEDED(pti->GetDocumentation( MEMBERID_NIL, &bstrName, &bstrDoc, &dwHelpID, &bstrHelp )))
		{
			CString str ;
			if (bstrDoc && *bstrDoc)
			{
				WRITECR(",") ;
				USES_CONVERSION;
				LPCTSTR lpszDoc = OLE2CT(bstrDoc);
				::SysFreeString(bstrDoc);
				bstrDoc = NULL;
				str.Format( _T("  helpstring(\"%s\")"), lpszDoc ) ;
				WRITE1( str ) ;
				if (dwHelpID > 0)
				{
					WRITECR(",") ;
					str.Format( _T("  helpcontext(%#08.8x)"), dwHelpID ) ;
					WRITE1( str ) ;
				}
			}
			else if (dwHelpID > 0)
			{
				WRITECR(",") ;
				str.Format( _T("  helpcontext(%#08.8x)"), dwHelpID ) ;
				WRITE1( str ) ;
			}
		}

		// source, default, or restricted
		if (pattr->wTypeFlags == TYPEFLAG_FHIDDEN)
		{
			WRITECR(",") ;
			WRITE1("  hidden") ;
		}

	
		int    implflags = NULL ;
		if (FAILED(hr = pti->GetImplTypeFlags(0, &implflags )))
			_com_issue_error(hr) ;

		if (implflags & IMPLTYPEFLAG_FDEFAULT)
		{
			WRITECR(",") ;
			WRITE1("  default") ;
		}

		if (implflags & IMPLTYPEFLAG_FSOURCE)
		{
			WRITECR(",") ;
			WRITE1("  source") ;
		}

		if (implflags & IMPLTYPEFLAG_FRESTRICTED)
		{
			WRITECR(",") ;
			WRITE1("  restricted") ;
		}
	 
		WRITECR("") ;
		WRITELINE(_T("]")) ;
		WRITE1(_T("interface ")) ;
		// interface name
		WRITEBSTR( bstrName ) ;

		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;
		bstrName = bstrDoc = bstrHelp = NULL ;

		// is there a base interface?
		for (UINT n = 0 ; n <  pattr->cImplTypes; n++)
		{
			HREFTYPE href = NULL ;
			if (FAILED(hr = pti->GetRefTypeOfImplType(n, &href)))
				_com_issue_error(hr) ;

			if (FAILED(hr = pti->GetRefTypeInfo( href, &ptiImpl )))
				_com_issue_error(hr) ;

			if (FAILED(hr = ptiImpl->GetDocumentation( MEMBERID_NIL, &bstrName, &bstrDoc, &dwHelpID, &bstrHelp )))
				_com_issue_error(hr) ;

			WRITE(_T(" : ")) ;
			WRITEBSTR( bstrName ) ;

			SysFreeString( bstrName ) ;
			bstrName = NULL ;
			SysFreeString( bstrDoc ) ;
			bstrDoc = NULL ;
			SysFreeString( bstrHelp ) ;
			bstrHelp = NULL ;

			ptiImpl->Release() ;
			ptiImpl = NULL ;
		}

		WRITECR(_T("\n    {")) ;

		for (int n = 0 ; n < pattr->cFuncs ; n++)
			DumpFunc( pstm, pti, pattr, n, uiIndent + 1 ) ;

		WRITELINE("};") ;

		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;

		pti->ReleaseTypeAttr( pattr ) ;
	}
	CATCH(CException, pException)
	{
		if (ptiImpl)
			ptiImpl->Release() ;

		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;
		if (pattr)
			pti->ReleaseTypeAttr( pattr ) ;

		THROW_LAST();
	}
	END_CATCH
	return hr ;
}


//  Syntax 1
//
//  [
//    attributes
//  ]
//  dispinterface intfname {
//      properties:
//          proplist
//      methods:
//          methlist
//  };
//
//  Syntax 2
//
//  [
//    attributes
//  ]
//  dispinterface intfname {
//      interface interfacename
//  };
//
HRESULT DeCompileDispinterface( IStream* pstm, ITypeInfo* pti, UINT uiIndent /* = 0 */ )
{
	HRESULT     hr = S_OK ;
	TYPEATTR*   pattr = NULL ;
	BSTR            bstrName = NULL ;
	BSTR            bstrDoc = NULL ;
	BSTR            bstrHelp = NULL ;
	DWORD           dwHelpID ;
	ITypeInfo*      ptiImpl = NULL ;

	TRY
	{
		if (FAILED(hr = pti->GetTypeAttr( &pattr)))
			_com_issue_error( hr ) ;

		WRITELINE(_T("[")) ;

		TCHAR szGUID[64] ;
		StringFromGUID2T( pattr->guid, szGUID, sizeof(szGUID) ) ;
		WRITE1("  uuid(") ;
		WRITE(szGUID) ;
		WRITE(")") ;

		// was version specified
		if (pattr->wMajorVerNum || pattr->wMinorVerNum)
		{
			WRITECR(",") ;
			CString str ;
			str.Format(_T("  version(%d.%d)"), pattr->wMajorVerNum, pattr->wMinorVerNum) ;
			WRITE1(str) ;
		}

		if (SUCCEEDED(pti->GetDocumentation( MEMBERID_NIL, &bstrName, &bstrDoc, &dwHelpID, &bstrHelp )))
		{
			CString str ;
			if (bstrDoc && *bstrDoc)
			{
				WRITECR(",") ;
				USES_CONVERSION;
				LPCTSTR lpszDoc = OLE2CT(bstrDoc);
				::SysFreeString(bstrDoc);
				bstrDoc = NULL;
				str.Format( _T("  helpstring(\"%s\")"), lpszDoc ) ;
				WRITE1( str ) ;
				if (dwHelpID > 0)
				{
					WRITECR(",") ;
					str.Format( _T("  helpcontext(%#08.8x)"), dwHelpID ) ;
					WRITE1( str ) ;
				}
			}
			else if (dwHelpID > 0)
			{
				WRITECR(",") ;
				str.Format( _T("  helpcontext(%#08.8x)"), dwHelpID ) ;
				WRITE1( str ) ;
			}
		}

		// source, default, or restricted
		if (pattr->wTypeFlags == TYPEFLAG_FHIDDEN)
		{
			WRITECR(",") ;
			WRITE1("  hidden") ;
		}

		if (pattr->wTypeFlags == TYPEFLAG_FDUAL)
		{
			WRITECR(",") ;
			WRITE1("  dual") ;
		}

		WRITECR("") ;
		WRITELINE(_T("]")) ;
		WRITE1(_T("dispinterface ")) ;
		// interface name
		WRITEBSTR( bstrName ) ;

		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;
		bstrName = bstrDoc = bstrHelp = NULL ;

		WRITECR(_T("\n    {")) ;

		WRITELINE2(_T("properties:"), 1) ;
		for (UINT n = 0 ; n < pattr->cVars ; n++)
			DumpVar( pstm, pti, pattr, n, uiIndent + 2 ) ;

		WRITELINE2(_T("methods:"), 1) ;
		for (int n = 0 ; n < pattr->cFuncs ; n++)
			DumpFunc( pstm, pti, pattr, n, uiIndent + 2 ) ;

		WRITELINE("};") ;

		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;

		pti->ReleaseTypeAttr( pattr ) ;
	}
	CATCH(CException, pException)
	{
		if (ptiImpl)
			ptiImpl->Release() ;

		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;
		if (pattr)
			pti->ReleaseTypeAttr( pattr ) ;
	}
	END_CATCH
	return hr ;
}

//  [
//    attributes
//  ]
//  coclass classname {
//      [attributes2] [interface | dispinterface] interfacename;
//      ...
//  };
//
//  attributes
//      uuid, helpstring, helpcontext, licensed, version, and appobject
//
HRESULT DeCompileCoClass( IStream* pstm, ITypeInfo* pti, UINT uiIndent /* = 0 */ )
{
	HRESULT     hr = S_OK ;
	TYPEATTR*   pattr = NULL ;
	BSTR            bstrName = NULL ;
	BSTR            bstrDoc = NULL ;
	BSTR            bstrHelp = NULL ;
	DWORD           dwHelpID ;
	ITypeInfo*      ptiImpl = NULL ;
	TYPEATTR*       pattrImpl = NULL ;

	TRY
	{
		if (FAILED(hr = pti->GetTypeAttr( &pattr)))
			_com_issue_error( hr ) ;

		WRITELINE(_T("[")) ;

		TCHAR szGUID[64] ;
		StringFromGUID2T( pattr->guid, szGUID, sizeof(szGUID) ) ;
		WRITE1("  uuid(") ;
		WRITE(szGUID) ;
		WRITE(")") ;

		// was version specified
		if (pattr->wMajorVerNum || pattr->wMinorVerNum)
		{
			WRITECR(",") ;
			CString str ;
			str.Format(_T("  version(%d.%d)"), pattr->wMajorVerNum, pattr->wMinorVerNum) ;
			WRITE1(str) ;
		}

		if (SUCCEEDED(pti->GetDocumentation( MEMBERID_NIL, &bstrName, &bstrDoc, &dwHelpID, &bstrHelp )))
		{
			CString str ;
			if (bstrDoc && *bstrDoc)
			{
				WRITECR(",") ;
				USES_CONVERSION;
				LPCTSTR lpszDoc = OLE2CT(bstrDoc);
				::SysFreeString(bstrDoc);
				bstrDoc = NULL;
				str.Format( _T("  helpstring(\"%s\")"), lpszDoc ) ;
				WRITE1( str ) ;
				if (dwHelpID > 0)
				{
					WRITECR(",") ;
					str.Format( _T("  helpcontext(%#08.8x)"), dwHelpID ) ;
					WRITE1( str ) ;
				}
			}
			else if (dwHelpID > 0)
			{
				WRITECR(",") ;
				str.Format( _T("  helpcontext(%#08.8x)"), dwHelpID ) ;
				WRITE1( str ) ;
			}
		}

		if (pattr->wTypeFlags == TYPEFLAG_FAPPOBJECT)
		{
			WRITECR(",") ;
			WRITE1("  appobject") ;
		}
		if (pattr->wTypeFlags == TYPEFLAG_FHIDDEN)
		{
			WRITECR(",") ;
			WRITE1("  hidden") ;
		}
		if (pattr->wTypeFlags == TYPEFLAG_FLICENSED)
		{
			WRITECR(",") ;
			WRITE1("  licensed") ;
		}
		if (pattr->wTypeFlags == TYPEFLAG_FCONTROL)
		{
			WRITECR(",") ;
			WRITE1("  control") ;
		}

		WRITECR("") ;
		WRITELINE(_T("]")) ;
		WRITE1(_T("coclass ")) ;
		// coclass name
		WRITEBSTR( bstrName ) ;

		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;
		bstrName = bstrDoc = bstrHelp = NULL ;

		WRITECR(_T("\n    {")) ;

		//  [attributes2] [interface | dispinterface] interfacename;
		for (UINT n = 0 ; n < pattr->cImplTypes ; n++)
		{
			HREFTYPE href = NULL ;

			int impltype = NULL ;
			if (FAILED(hr = pti->GetImplTypeFlags( n, &impltype )))
				_com_issue_error(hr) ;

			if (FAILED(hr = pti->GetRefTypeOfImplType(n, &href)))
				_com_issue_error(hr) ;

			if (FAILED(hr = pti->GetRefTypeInfo( href, &ptiImpl )))
				_com_issue_error(hr) ;

			if (FAILED(hr = ptiImpl->GetDocumentation( MEMBERID_NIL, &bstrName, &bstrDoc, &dwHelpID, &bstrHelp )))
				_com_issue_error(hr) ;

			if (FAILED(hr = ptiImpl->GetTypeAttr( &pattrImpl)))
				_com_issue_error( hr ) ;

			WRITE2(_T(""), 1 ) ;

			if (impltype)
			{
				WRITE(_T("[")) ;
				BOOL    fComma = FALSE ;
				if (impltype & IMPLTYPEFLAG_FDEFAULT)
				{
					WRITE(_T("default")) ;
					fComma = TRUE ;
				}
				if (impltype & IMPLTYPEFLAG_FSOURCE)
				{
					if (fComma)
						WRITE(_T(", ")) ;
					WRITE(_T("source")) ;
					fComma = TRUE ;
				}
				if (impltype & IMPLTYPEFLAG_FRESTRICTED)
				{
					if (fComma)
						WRITE(_T(", ")) ;
					WRITE(_T("restricted")) ;
				}

				WRITE(_T("] "));
			}

			if (pattrImpl->typekind == TKIND_INTERFACE)
				WRITE(_T("interface ")) ;

			if (pattrImpl->typekind == TKIND_DISPATCH)
				WRITE(_T("dispinterface ")) ;

			WRITE( bstrName ) ;

			WRITECR(_T(";")) ;

			SysFreeString( bstrName ) ;
			bstrName = NULL ;
			SysFreeString( bstrDoc ) ;
			bstrDoc = NULL ;
			SysFreeString( bstrHelp ) ;
			bstrHelp = NULL ;

			ptiImpl->ReleaseTypeAttr( pattrImpl ) ;
			pattrImpl = NULL ;

			ptiImpl->Release() ;
			ptiImpl = NULL ;
		}

		WRITELINE("};") ;

		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;

		pti->ReleaseTypeAttr( pattr ) ;
	}
	CATCH(CException, pException)
	{
		if (ptiImpl)
		{
			if (pattrImpl)
				ptiImpl->ReleaseTypeAttr( pattrImpl ) ;

			ptiImpl->Release() ;
		}
		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;
		if (pattr)
			pti->ReleaseTypeAttr( pattr ) ;

		THROW_LAST();
	}
	END_CATCH
	return hr ;
}

HRESULT DumpFunc( IStream* pstm, ITypeInfo* pti, TYPEATTR* pattr, MEMBERID memid, UINT uiIndent /* = 0 */ )
{
	HRESULT     hr = S_OK ;
	FUNCDESC*   pfuncdesc = NULL ;
	BSTR            rgbstrNames[MAX_NAMES] ;
	BSTR            bstrName = NULL ;
	BSTR            bstrDoc = NULL ;
	BSTR            bstrHelp = NULL ;
	DWORD           dwHelpID ;
	ASSERT(pti) ;

	for (UINT ui = 0 ; ui < MAX_NAMES ; ui++)
		rgbstrNames[ui] = NULL ;

	TRY
	{
		try
		{
			if(FAILED(hr = pti->GetFuncDesc( memid, &pfuncdesc )))
				_com_issue_error( hr ) ;
		}
		catch(...)
		{
			TRACE(_T("Error in DumpFunc.\n"));

			if(pfuncdesc == NULL)
				return hr;
		}

		// If pattr->typekind == TKIND_DISPATCH (dispinterface)
		//    [attributes] returntype methname(params);
		// where attributes can be
		//      id(<id>), propput, propget,
		//      propputref, bindable, defaultbind, displaybind,
		//      requestedit, source, vararg, hidden, helpstring("<helpstring>"),
		//      helpcontext(<id>)
		//
		// If pattr->typekind == TKIND_INTERFACE || TKIND_MODULE
		//  [attributes] returntype [calling convention]  funcname(params);
		// where attributes can be
		//      restricted, bindable, defaultbind, displaybind,
		//      requestedit, source, vararg, hidden, helpstring("<helpstring>"),
		//      helpcontext(<id>)
		// and calling convention can be
		//      pascal, cdecl, stdcall
		//
		// Write [attributes]
		//
		BOOL    fAttributes = FALSE ;      // any attributes?
		BOOL    fAttribute = FALSE ;       // at least one (insert ",")
		WRITE1("") ;    // indent
		CString str ;
		if (pattr->typekind == TKIND_DISPATCH)
		{
			fAttributes = TRUE ;
			fAttribute = TRUE ;
			str.Format(_T("[id(%d)"), memid) ;
			WRITE(str) ;
		}
		else if (pattr->typekind == TKIND_MODULE)
		{
			fAttributes = TRUE ;
			fAttribute = TRUE ;
			str.Format(_T("[entry(%d)"), memid) ;
			WRITE(str) ;
		}
		else
			// if there are some attributes
			if ((pfuncdesc->invkind > 1)|| pfuncdesc->wFuncFlags || pfuncdesc->cParamsOpt == -1)
			{
				WRITE("[") ;
				fAttributes = TRUE ;
			}

		if (pfuncdesc->invkind & INVOKE_PROPERTYGET)
		{
			if (fAttribute)
				WRITE(", ") ;
			fAttribute = TRUE ;
			WRITE("propget") ;
		}
		if (pfuncdesc->invkind & INVOKE_PROPERTYPUT)
		{
			if (fAttribute)
				WRITE(", ") ;
			fAttribute = TRUE ;
			WRITE("propput") ;
		}
		if (pfuncdesc->invkind & INVOKE_PROPERTYPUTREF)
		{
			if (fAttribute)
				WRITE(", ") ;
			fAttribute = TRUE ;
			WRITE("propputref") ;
		}
		if (pfuncdesc->wFuncFlags & FUNCFLAG_FRESTRICTED)
		{
			if (fAttribute)
				WRITE(", ") ;
			fAttribute = TRUE ;
			WRITE("restricted") ;
		}
		if (pfuncdesc->wFuncFlags & FUNCFLAG_FSOURCE)
		{
			if (fAttribute)
				WRITE(", ") ;
			fAttribute = TRUE ;
			WRITE("source") ;
		}
		if (pfuncdesc->wFuncFlags & FUNCFLAG_FBINDABLE)
		{
			if (fAttribute)
				WRITE(", ") ;
			fAttribute = TRUE ;
			WRITE("bindable") ;
		}
		if (pfuncdesc->wFuncFlags & FUNCFLAG_FREQUESTEDIT)
		{
			if (fAttribute)
				WRITE(", ") ;
			fAttribute = TRUE ;
			WRITE("requestedit") ;
		}
		if (pfuncdesc->wFuncFlags & FUNCFLAG_FDISPLAYBIND)
		{
			if (fAttribute)
				WRITE(", ") ;
			fAttribute = TRUE ;
			WRITE("displaybind") ;
		}
		if (pfuncdesc->wFuncFlags & FUNCFLAG_FDEFAULTBIND)
		{
			if (fAttribute)
				WRITE(", ") ;
			fAttribute = TRUE ;
			WRITE("defaultbind") ;
		}
		if (pfuncdesc->wFuncFlags & FUNCFLAG_FHIDDEN)
		{
			if (fAttribute)
				WRITE(", ") ;
			fAttribute = TRUE ;
			WRITE("hidden") ;
		}
		if (pfuncdesc->cParamsOpt == -1)    // cParamsOpt > 0 indicates VARIANT style
		{
			if (fAttribute)
				WRITE(", ") ;
			fAttribute = TRUE ;
			WRITE("vararg") ;         // optional params
		}

		if (SUCCEEDED(pti->GetDocumentation( pfuncdesc->memid, &bstrName, &bstrDoc, &dwHelpID, &bstrHelp )))
		{
			CString str ;
			if (bstrDoc && *bstrDoc)
			{
				if (fAttributes == FALSE) WRITE("[") ;
				fAttributes = TRUE ;
				if (fAttribute)
					WRITE(", ") ;
				USES_CONVERSION;
				LPCTSTR lpszDoc = OLE2CT(bstrDoc);
				::SysFreeString(bstrDoc);
				bstrDoc = NULL;
				str.Format( _T("helpstring(\"%s\")"), lpszDoc ) ;
				WRITE( str ) ;
				if (dwHelpID > 0)
				{
					str.Format( _T(", helpcontext(%#08.8x)"), dwHelpID ) ;
					WRITE( str ) ;
				}
			}
			else if (dwHelpID > 0)
			{
				if (fAttributes == FALSE) WRITE("[") ;
				if (fAttribute)
					WRITE(", ") ;
				fAttributes = TRUE ;
				str.Format( _T("helpcontext(%#08.8x)"), dwHelpID ) ;
				WRITE( str ) ;
			}
		}
		if (fAttributes)
			WRITE("] ") ;

		// Write return type
		WRITE(TYPEDESCtoString( pti, &pfuncdesc->elemdescFunc.tdesc )) ;
		WRITE(" ") ;

		if (pattr->typekind != TKIND_DISPATCH)
		{   // Write calling convention
			switch(pfuncdesc->callconv)
			{
				case CC_CDECL:      WRITE("_cdecl ") ;           break ;
				//case CC_MSCPASCAL:  WRITE("_mspascal ") ;        break ;
				case CC_PASCAL:     WRITE("_pascal ") ;          break ;
				case CC_MACPASCAL:  WRITE("_macpascal ") ;       break ;
				case CC_STDCALL :   WRITE("_stdcall ") ;          break ;
				//case CC_RESERVED:   WRITE("_reserved ") ;         break ;
				case CC_SYSCALL:    WRITE("_syscall ") ;          break ;
				case CC_MPWCDECL:   WRITE("_mpwcdecl ") ;         break ;
				case CC_MPWPASCAL:  WRITE("_mpwpascal ") ;        break ;
			}
		}

		// Write methodname

		// NOTE:  If a property has the propput or propputref attributes the
		// 'right hand side' (rhs) is *always* the last parameter and MkTypeLib
		// strips the parameter name.  Thus you will always get 1 less name
		// back from ::GetNames than normal.
		//
		// Thus for the example below
		//  [propput] void Color([in] VARIANT rgb, [in] VARIANT rgb2 );
		// without taking this into consderation the output would be
		//  [propput] void Color([in] VARIANT rgb, [in] VARIANT );
		// when it should be
		//  [propput] void Color([in] VARIANT rgb, [in] VARIANT rhs );
		//
		// Another weirdness comes from a bug (which will never be fixed)
		// where optional parameters on property functions were allowed.
		// Because they were allowed by accident people used them, so they
		// are still allowed.
		//

		UINT cNames = 0 ;
		if (FAILED( hr = pti->GetNames( pfuncdesc->memid, rgbstrNames, MAX_NAMES, &cNames )))
			_com_issue_error( hr ) ;

		// fix for 'rhs' problem
		if ((int)cNames < pfuncdesc->cParams + 1)
		{
			rgbstrNames[cNames] = ::SysAllocString(OLESTR("rhs")) ;
			cNames++ ;
		}

		ASSERT((int)cNames == pfuncdesc->cParams+1) ;
		WRITEBSTR( rgbstrNames[0] ) ;
		WRITE("(") ;

		// params have the format
		//   [attributes] type paramname
		// where attributes can be
		//   in, out, optional, string   (string is not valid for TKIND_MODULE)
		//
		if (pfuncdesc->cParams > 1)
			WRITECR("") ;
		for ( int n = 0 ; n < pfuncdesc->cParams ; n++ )
		{
			if (pfuncdesc->cParams > 1)
				WRITE2("", 4 ) ;    // indent 4
			fAttributes = FALSE ;
			fAttribute = FALSE ;

			if (pfuncdesc->lprgelemdescParam[n].idldesc.wIDLFlags)
			{
				WRITE("[") ;
				fAttributes = TRUE ;
			}

			if (pfuncdesc->lprgelemdescParam[n].idldesc.wIDLFlags & IDLFLAG_FIN)
			{
				if (fAttribute)
					WRITE(", ") ;
				WRITE("in") ;
				fAttribute = TRUE ;
			}
			if (pfuncdesc->lprgelemdescParam[n].idldesc.wIDLFlags & IDLFLAG_FOUT)
			{
				if (fAttribute)
					WRITE(", ") ;
				WRITE("out") ;
				fAttribute = TRUE ;
			}
			if (pfuncdesc->lprgelemdescParam[n].idldesc.wIDLFlags & IDLFLAG_FLCID)
			{
				if (fAttribute)
					WRITE(", ") ;
				WRITE("lcid") ;
				fAttribute = TRUE ;
			}
			if (pfuncdesc->lprgelemdescParam[n].idldesc.wIDLFlags & IDLFLAG_FRETVAL)
			{
				if (fAttribute)
					WRITE(", ") ;
				WRITE("retval") ;
				fAttribute = TRUE ;
			}

			// If we have an optional last parameter and we're on the last paramter
			// or we are into the optional parameters...
			if ((pfuncdesc->cParamsOpt == -1 && n == pfuncdesc->cParams - 1) ||
				(n > (pfuncdesc->cParams - pfuncdesc->cParamsOpt)))
			{
				if (fAttribute)
					WRITE(", ") ;
				if (!fAttributes)
					WRITE("[") ;
				WRITE("optional") ;
				fAttributes = TRUE ;
				fAttribute = TRUE ;
			}

			if (fAttributes)
				WRITE("] ") ;

			// type
			CString str ;
			if ((pfuncdesc->lprgelemdescParam[n].tdesc.vt & 0x0FFF) == VT_CARRAY)
			{
				// type name[n]
				WRITE(TYPEDESCtoString( pti, &pfuncdesc->lprgelemdescParam[n].tdesc.lpadesc->tdescElem )) ;
				WRITE(" ") ;

				WRITEBSTR(rgbstrNames[n+1]) ;

				// Allocate cDims * lstrlen("[123456]")
				for (USHORT n = 0 ; n < pfuncdesc->lprgelemdescParam[n].tdesc.lpadesc->cDims ; n++)
				{
					str.Format( _T("[%d]"), pfuncdesc->lprgelemdescParam[n].tdesc.lpadesc->rgbounds[n].cElements ) ;
					WRITE(str) ;
				}
			}
			else
			{
				WRITE(TYPEDESCtoString( pti, &pfuncdesc->lprgelemdescParam[n].tdesc ) + " " ) ;
				WRITEBSTR(rgbstrNames[n+1]) ;
			}
			if (n < pfuncdesc->cParams - 1)
				WRITECR(", ") ;
		}

		WRITECR(");") ;

		try
		{
			for (UINT ui = 0 ; ui < MAX_NAMES ; ui++)
				if (rgbstrNames[ui])
					SysFreeString(rgbstrNames[ui]) ;
			if (bstrName)
				SysFreeString( bstrName ) ;
			if (bstrDoc)
				SysFreeString( bstrDoc ) ;
			if (bstrHelp)
				SysFreeString( bstrHelp ) ;

			pti->ReleaseFuncDesc( pfuncdesc ) ;
		}
		catch(...)
		{
			TRACE(_T("Error in DumpFunc.\n"));
		}
	}
	CATCH(CException, pException)
	{
		for (UINT ui = 0 ; ui < MAX_NAMES ; ui++)
			if (rgbstrNames[ui])
				SysFreeString(rgbstrNames[ui]) ;

		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;
		if (pfuncdesc)
			pti->ReleaseFuncDesc( pfuncdesc ) ;

		THROW_LAST();
	}
	END_CATCH

	return hr ;
}

HRESULT DeCompileVar( IStream* pstm, ITypeInfo* pti, MEMBERID memid, UINT uiIndent /* = 0 */)
{
	HRESULT     hr = S_OK ;
	TYPEATTR*   pattr = NULL ;
	ASSERT(pti) ;

	TRY
	{
		if (FAILED(hr = pti->GetTypeAttr( &pattr)))
			_com_issue_error( hr ) ;

		if (FAILED(hr = DumpVar( pstm, pti, pattr, memid, uiIndent )))
			_com_issue_error( hr ) ;

		pti->ReleaseTypeAttr( pattr ) ;
	}
	CATCH(CException, pException)
	{
		if (pattr)
			pti->ReleaseTypeAttr( pattr ) ;

		THROW_LAST();
	}
	END_CATCH

	return hr ;
}

// if fConst ==  TURE
//  const type name = value ;
// else
//  name = value  (no commas)
//
HRESULT DeCompileConst( IStream* pstm, ITypeInfo* pti, MEMBERID memid, UINT uiIndent /* = 0 */, BOOL fConst /* = TRUE */  )
{
	HRESULT     hr = S_OK ;
	TYPEATTR*       pattr = NULL ;
	ASSERT(pti) ;

	TRY
	{
		if (FAILED(hr = pti->GetTypeAttr( &pattr)))
			_com_issue_error( hr ) ;

		if (FAILED(hr = DumpConst( pstm, pti, pattr, memid, uiIndent, fConst )))
			_com_issue_error( hr ) ;

		pti->ReleaseTypeAttr( pattr ) ;
	}
	CATCH(CException, pException)
	{
		if (pattr)
			pti->ReleaseTypeAttr( pattr ) ;
		THROW_LAST();
	}
	END_CATCH

	return hr ;
}

HRESULT DumpConst( IStream* pstm, ITypeInfo* pti, TYPEATTR* pattr, MEMBERID memid, UINT uiIndent /* = 0 */, BOOL fConst /* = TRUE */  )
{
	USES_CONVERSION;
	HRESULT     hr = S_OK ;
	VARDESC*    pvardesc = NULL ;
	BSTR            rgbstrNames[1] ;
	BSTR            bstrName = NULL ;
	BSTR            bstrDoc = NULL ;
	BSTR            bstrHelp = NULL ;
	DWORD           dwHelpID ;
	ASSERT(pti) ;
	VARIANT varValue ;
	VariantInit( &varValue ) ;

	TRY
	{
		if (FAILED(hr = pti->GetVarDesc( memid, &pvardesc )))
			_com_issue_error( hr ) ;

		ASSERT(pvardesc->varkind == VAR_CONST) ;

		CString str = TYPEDESCtoString( pti, &pvardesc->elemdescVar.tdesc ) ;
		if (FAILED(hr = VariantChangeType( &varValue, pvardesc->lpvarValue, 0, VT_BSTR )))
		{
			if (pvardesc->lpvarValue->vt == VT_ERROR || pvardesc->lpvarValue->vt == VT_HRESULT)
			{
				varValue.vt = VT_BSTR ;
				varValue.bstrVal = ::SysAllocString(
					CT2OLE(_GetScodeString(pvardesc->lpvarValue->scode))
					) ;
				hr = S_OK ;
			}
			else
				_com_issue_error( hr ) ;
		}

		BOOL fIndent = FALSE ;
		if (fConst)
		{
			CString str ;
			if (pattr->typekind == TKIND_MODULE)
			{
				str.Format(_T("[entry(%d)"), memid) ;
				WRITE1(str) ;
				fIndent = TRUE ;
			}

			// [helpstring("<helpstring>"), helpcontext(<id>)] const type name = expression ;
			if (SUCCEEDED(pti->GetDocumentation( pvardesc->memid, &bstrName, &bstrDoc, &dwHelpID, &bstrHelp )))
			{
				if (bstrDoc && *bstrDoc)
				{
					if (!fIndent)
						WRITE1("[") ;
					else
						WRITE(", ") ;
					LPCTSTR lpszDoc = OLE2CT(bstrDoc);
					::SysFreeString(bstrDoc);
					bstrDoc = NULL;
					str.Format( _T("helpstring(\"%s\")"), lpszDoc ) ;
					WRITE( str ) ;
					if (dwHelpID > 0)
					{
						str.Format( _T(", helpcontext(%#08.8x)"), dwHelpID ) ;
						WRITE( str ) ;
					}
					WRITE("] ");
					fIndent = TRUE ;
				}
				else if (dwHelpID > 0)
				{
					if (!fIndent)
						WRITE1("[") ;
					else
						WRITE(", ") ;
					str.Format( _T("helpcontext(%#08.8x)] "), dwHelpID ) ;
					WRITE( str ) ;
					fIndent = TRUE ;
				}
			}
		}

		UINT cNames ;
		if (FAILED( hr = pti->GetNames( pvardesc->memid, rgbstrNames, 1, (UINT FAR*)&cNames )))
			_com_issue_error( hr ) ;

		if (fConst)
		{
			if (!fIndent)
				WRITE1(_T("")) ;
			WRITE("const ") ; WRITE( str ) ; WRITE( " " ) ;
		}
		else
			WRITE1("") ;

		WRITEBSTR( rgbstrNames[0] ) ; WRITE( " = " ) ;

		if (pvardesc->lpvarValue->vt == VT_BSTR)
		{
			WRITE( "\"" ) ;
			WRITEBSTR( varValue.bstrVal ) ;
			WRITE( "\"" ) ;
		}
		else
			WRITEBSTR( varValue.bstrVal) ;

		if (fConst)
			WRITECR(";") ;

		VariantClear( &varValue ) ;

		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;
		pti->ReleaseVarDesc( pvardesc ) ;
	}
	CATCH(CException, pException)
	{
		VariantClear( &varValue ) ;
		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;
		if (pvardesc)
			pti->ReleaseVarDesc( pvardesc ) ;

		THROW_LAST();
	}
	END_CATCH

	return hr ;
}

IStream* CreateMemoryStream()
{
	LPSTREAM lpStream = NULL;

	// Create a stream object on a memory block.
	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE|GMEM_SHARE, 0);
	if (hGlobal != NULL)
	{
		HRESULT hr ;
		if (FAILED(hr = CreateStreamOnHGlobal(hGlobal, TRUE, &lpStream)))
		{
			TRACE0("CreateStreamOnHGlobal failed.\n");
			GlobalFree(hGlobal);
			return NULL;
//			_com_issue_error( hr ) ;
		}
	}
	else
	{
		TRACE0("Failed to allocate memory for stream.\n");
		AfxThrowMemoryException() ;
	}

	return lpStream;
}

HRESULT DeCompileForwardDeclareDispinterface( IStream* pstm, ITypeInfo* pti, UINT uiIndent /* = 0 */ )
{
	HRESULT     hr = S_OK ;
	TYPEATTR*   pattr = NULL ;
	BSTR            bstrName = NULL ;
	BSTR            bstrDoc = NULL ;
	BSTR            bstrHelp = NULL ;
	DWORD           dwHelpID ;
	ITypeInfo*      ptiImpl = NULL ;

	TRY
	{
		if (FAILED(hr = pti->GetTypeAttr( &pattr)))
			_com_issue_error( hr ) ;

		if (SUCCEEDED(pti->GetDocumentation( MEMBERID_NIL, &bstrName, &bstrDoc, &dwHelpID, &bstrHelp )))
		{
			CString str = (LPCTSTR)_bstr_t(bstrName);
			if(!str.IsEmpty())
				WRITE("\n    dispinterface " + str + ";");
		}

		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;
		bstrName = bstrDoc = bstrHelp = NULL ;

		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;

		pti->ReleaseTypeAttr( pattr ) ;
	}
	CATCH(CException, pException)
	{
		if (ptiImpl)
			ptiImpl->Release() ;

		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;
		if (pattr)
			pti->ReleaseTypeAttr( pattr ) ;
	}
	END_CATCH
	return hr ;
}

HRESULT DeCompileForwardDeclareInterface( IStream* pstm, ITypeInfo* pti, UINT uiIndent /* = 0 */ )
{
	HRESULT     hr = S_OK ;
	TYPEATTR*   pattr = NULL ;
	BSTR            bstrName = NULL ;
	BSTR            bstrDoc = NULL ;
	BSTR            bstrHelp = NULL ;
	DWORD           dwHelpID ;
	ITypeInfo*      ptiImpl = NULL ;

	TRY
	{
		if (FAILED(hr = pti->GetTypeAttr( &pattr)))
			_com_issue_error( hr ) ;

		if(SUCCEEDED(pti->GetDocumentation( MEMBERID_NIL, &bstrName, &bstrDoc, &dwHelpID, &bstrHelp )))
		{
			CString str = (LPCTSTR)_bstr_t(bstrName);
			if(!str.IsEmpty())
				WRITE("\n    interface " + str + ";");
		}

		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;
		bstrName = bstrDoc = bstrHelp = NULL ;
		
		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;

		pti->ReleaseTypeAttr( pattr ) ;
	}
	CATCH(CException, pException)
	{
		if (ptiImpl)
			ptiImpl->Release() ;

		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;
		if (pattr)
			pti->ReleaseTypeAttr( pattr ) ;

		THROW_LAST();
	}
	END_CATCH
	return hr ;
}

HRESULT DeCompileTypeLib(IStream* pstm, ITypeLib* ptlb, UINT uiIndent /* = 0 */ )
{
	USES_CONVERSION;
	HRESULT         hr = S_OK ;
	BSTR            bstrName = NULL ;
	BSTR            bstrDoc = NULL ;
	BSTR            bstrHelp = NULL ;
	DWORD           dwHelpID ;
	TLIBATTR*       plibattr = NULL ;
	ITypeInfo*      pti = NULL ;
	TYPEATTR*       pattr = NULL ;

	ASSERT(ptlb) ;

	TRY
	{
		TCHAR szGUID[64] ;
		CString str;
		hr = ptlb->GetLibAttr(&plibattr);
		if (FAILED(hr))
			_com_issue_error( hr ) ;

		// Write header with library filename
		WRITE1(_T("// Generated .IDL/C++ pseudo source equivalent of Win32 type library ..\\"));
		hr = ::QueryPathOfRegTypeLib(plibattr->guid, plibattr->wMajorVerNum, plibattr->wMinorVerNum, plibattr->lcid, &bstrName);
		if (SUCCEEDED(hr))
		{
			LPCTSTR lpszName = OLE2CT(bstrName);
			::SysFreeString(bstrName);
			bstrName = NULL;

			LPTSTR p = (LPTSTR) _tcsrchr(lpszName, '\\');
			if (p != NULL && *p && *(p+1))
				WRITECR( p+1 ) ;
			else
				WRITECR( p ) ;
		}
		else
		{
			// It's possible we're viewing a type lib that was never registered
			WRITECR(_T("<Unknown>") ) ;
		}

		WRITELINE( _T("[") ) ;

		StringFromGUID2T( plibattr->guid, szGUID, sizeof(szGUID) );
		WRITE1(_T("  uuid(")) ; WRITE(szGUID); WRITECR(_T("),"))  ;

		str.Format( _T("  version(%d.%d)"), plibattr->wMajorVerNum, plibattr->wMinorVerNum ) ;
		WRITE1(str) ;

		hr = ptlb->GetDocumentation( MEMBERID_NIL, &bstrName, &bstrDoc, &dwHelpID, &bstrHelp );
		if (FAILED(hr))
			_com_issue_error( hr ) ;

		if (bstrDoc && *bstrDoc)
		{
			WRITECR(_T(",")) ;
			WRITE1(_T("  helpstring(\"")) ;
			WRITEBSTR( bstrDoc ) ;
			WRITE(_T("\")")) ;
		}

		if (bstrHelp && *bstrHelp)
		{
			WRITECR(",") ;
			LPCTSTR lpszHelp = OLE2CT(bstrHelp);
			::SysFreeString(bstrHelp);
			bstrHelp = NULL;

			LPTSTR p = (LPTSTR) _tcsrchr(lpszHelp, '\\');
			if (p != NULL && *p && *(p+1))
				str.Format( _T("  helpfile(\"%s\"),"), p+1 ) ;
			else
				str.Format( _T("  helpfile(\"%s\"),"), lpszHelp ) ;

			WRITELINE( str ) ;
			str.Format( _T("  helpcontext(%#08.8x)"), dwHelpID ) ;
			WRITE1( str ) ;
		}

		WRITECR(_T("")) ;
		WRITELINE(_T("]") ) ;

		WRITE1( _T("library ") ) ;
		WRITEBSTR(bstrName) ;
		WRITECR(_T("")) ;

		WRITELINE( _T("{") ) ;

#ifdef _WRITE_IMPORT_LIBS
		// TODO:  Grovel through all typeinfos for any referenced types
		// that are defined in imported libs
		//uiInfos = ptlb->GetTypeInfoCount() ;
		//for (UINT n = 0 ; n < uiInfos ; n++)
		//{
		//}
		WRITELINE(_T("    // There most likely were \"importlib()\" statements in"));
		WRITELINE(_T("    // in the source.  While it appears possible to be able"));
		WRITELINE(_T("    // to identify them, it is non-trivial and is currently"));
		WRITELINE(_T("    // not supported."));
#endif
		
		UINT uiInfos = ptlb->GetTypeInfoCount() ;
		// Foward declares
		WRITE("    // Forward references and typedefs");
		for (UINT n = 0 ; n < uiInfos ; n++)
		{
			if (FAILED(hr = ptlb->GetTypeInfo( n, &pti )))
				_com_issue_error(hr) ;

			if (FAILED(hr = pti->GetTypeAttr( &pattr )))
				_com_issue_error(hr) ;

			switch(pattr->typekind)
			{
			case TKIND_INTERFACE:
				 hr = DeCompileForwardDeclareInterface( pstm, pti, uiIndent + 1 ) ;
			break ;

			case TKIND_DISPATCH:
				 hr = DeCompileForwardDeclareDispinterface( pstm, pti, uiIndent + 1 ) ;
			break ;
			}
			
			if(pti)
			{
				pti->ReleaseTypeAttr( pattr ) ;
				pti->Release() ;
				pti = NULL ;
				pattr = NULL;
			}
		}
		WRITE("\n\n");

		for (unsigned n = 0 ; n < uiInfos ; n++)
		{
			if (FAILED(hr = ptlb->GetTypeInfo( n, &pti )))
				_com_issue_error(hr) ;

			if (FAILED(hr = pti->GetTypeAttr( &pattr )))
				_com_issue_error(hr) ;

			switch(pattr->typekind)
			{
			case TKIND_ENUM:
			case TKIND_RECORD:
			case TKIND_UNION:
			case TKIND_ALIAS:
				 hr = DeCompileTypedef( pstm, pti, uiIndent + 1 ) ;
			break ;

			case TKIND_MODULE:
				 hr = DeCompileModule( pstm, pti, uiIndent + 1 ) ;
			break ;

			case TKIND_INTERFACE:
				 hr = DeCompileInterface( pstm, pti, uiIndent + 1 ) ;
			break ;

			case TKIND_DISPATCH:
				 hr = DeCompileDispinterface( pstm, pti, uiIndent + 1 ) ;
			break ;

			case TKIND_COCLASS:
				 hr = DeCompileCoClass( pstm, pti, uiIndent + 1 ) ;
			break ;
			}

			if (n != uiInfos - 1)
				WRITECR(_T("")) ;

			pti->ReleaseTypeAttr( pattr ) ;
			pti->Release() ;
			pti = NULL ;
			pattr = NULL ;
		}
		// Last line of the .IDL file
		WRITELINE( "};" ) ;

		SysFreeString( bstrName ) ;
		bstrName = NULL ;
		SysFreeString( bstrDoc ) ;
		bstrDoc = NULL ;
		SysFreeString( bstrHelp ) ;
		bstrHelp = NULL ;

		if (plibattr)
			ptlb->ReleaseTLibAttr( plibattr ) ;
	}
	CATCH(CException, pException)
	{
		if (pti)
		{
			if (pattr)
				pti->ReleaseTypeAttr( pattr ) ;
			pti->Release() ;
		}
		if (plibattr)
			ptlb->ReleaseTLibAttr( plibattr ) ;
		if (bstrName)
			SysFreeString( bstrName ) ;
		if (bstrDoc)
			SysFreeString( bstrDoc ) ;
		if (bstrHelp)
			SysFreeString( bstrHelp ) ;

		THROW_LAST() ;
	}
	END_CATCH

	return hr ;
}

HRESULT CTypeLib::IsTypeLib(
	const CString& sTypeLibFileName, LPTYPELIB& lpTypeLib
	)
{
	HRESULT hr = S_OK;

	try
	{
		USES_CONVERSION;
		LPCOLESTR lpszT = T2COLE(sTypeLibFileName);
		hr = ::LoadTypeLib( lpszT, &lpTypeLib );
		if(FAILED(hr))
			_com_issue_error(hr);
	}
	catch(...)
	{
	}

	return hr;
}

HRESULT CTypeLib::GetTypeLibAsString(
	const LPTYPELIB& lpTypeLib, CString& sText
	)
{
	sText.Empty();
	
	IStream* pStream = CreateMemoryStream();
	HRESULT hr = DeCompileTypeLib(pStream, lpTypeLib);
	if(FAILED(hr))
		_com_issue_error(hr);
	if(pStream != NULL)
	{
		STATSTG statstg;
		if(FAILED(hr = pStream->Stat(&statstg, STATFLAG_NONAME)))
			_com_issue_error(hr);
		
		// Seek to beginning
		LARGE_INTEGER li;
		LISet32(li, 0);
		if(FAILED(hr = pStream->Seek(li, STREAM_SEEK_SET, NULL)))
			_com_issue_error(hr);
		
		// Read into string
		LPTSTR lpszBuf = sText.GetBuffer(statstg.cbSize.LowPart+1);
		if(FAILED(hr = pStream->Read(lpszBuf, statstg.cbSize.LowPart, NULL)))
			_com_issue_error(hr);
		lpszBuf[statstg.cbSize.LowPart] = '\0';
		sText.ReleaseBuffer();
		pStream->Release();
	}

	sText.TrimRight();
	sText.TrimLeft();
	sText.Replace(_T(".ODL"), _T(".IDL"));
	sText.Replace(_T(".odl"), _T(".idl"));

	return hr;
}
