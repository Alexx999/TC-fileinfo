//==================================
// FILEINFO - ELF (Executable and Linkable Format)
// FILE: ELFDUMP.H
//==================================
CStringA DumpElfFile( const BYTE *base, ULONG_PTR size );

// Returns a multi-line string summarizing ELF-specific "version" fields
// (SONAME, ABI tag, build ID, interpreter, compiler comment). Empty if the
// file is not ELF or can't be read. Intended for the m_fi properties tab,
// as the ELF counterpart to Windows VERSIONINFO.
CStringA GetElfVersionInfo( LPCTSTR filename );
