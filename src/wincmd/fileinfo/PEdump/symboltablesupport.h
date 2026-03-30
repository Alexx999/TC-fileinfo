#include <afx.h>

class COFFSymbolTable;

BOOL LookupSymbolName(DWORD index, PSTR buffer, UINT length);
CStringA DumpSymbolTable( COFFSymbolTable * pSymTab );
CStringA DumpMiscDebugInfo( PIMAGE_DEBUG_MISC PMiscDebugInfo );
CStringA DumpCVDebugInfo( PDWORD pCVHeader );
CStringA DumpLineNumbers(PIMAGE_LINENUMBER pln, DWORD count);

