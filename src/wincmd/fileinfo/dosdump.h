CStringA DumpMZHeader( PIMAGE_DOS_HEADER pdosHeader );
CStringA DumpNEHeader( PIMAGE_DOS_HEADER dosHeader );
//CStringA DumpLXHeader( PIMAGE_DOS_HEADER dosHeader );
CStringA DumpLXHeader( EXE_FILE *pEXE );
//CStringA DumpLEHeader( PIMAGE_DOS_HEADER pdosHeader );
CStringA DumpLEHeader( EXE_FILE *pEXE );