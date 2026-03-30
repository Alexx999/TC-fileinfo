//**********************************************************************
// 
//  This program retrieves the version resource from a VxD.
// 
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
//  TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
// 
//  Copyright (C) 1998 Microsoft Corporation. All rights reserved.
//  Author: Jonathan Russ (jruss)
// 
//**********************************************************************

#include <windows.h>
#include <stdio.h>

#define SWAPWORDS(X) ( (X<<16) | (X>>16) )

// the following structure must be byte-aligned.
#pragma pack( push, pre_vxd_ver, 1 )
typedef struct _VXD_VERSION_RESOURCE {
  char  cType;
  WORD  wID;
  char  cName;
  WORD  wOrdinal;
  WORD  wFlags;
  DWORD dwResSize;
  BYTE  bVerData;
} VXD_VERSION_RESOURCE, *PVXD_VERSION_RESOURCE;
#pragma pack( pop, pre_vxd_ver )


//**********************************************************************
// 
//  FUNCTION:     GetVxdVersion - This function retrieves the version
//                information from a VxD. It is analagous to the
//                GetFileVersionInfo() API.
// 
//  PARAMETERS:   szFile  - null-terminated string indicating the VxD
//                          file name.
// 
//                lpdwLen - pointer to DWORD containing the size of the
//                          buffer.
// 
//                lpData  - pointer to buffer to receive the version
//                          resource
// 
//  RETURN VALUE: TRUE if successful. Otherwise, FALSE and
//                GetLastError() will return one of the following values
//                indicating why the function failed:
// 
//                ERROR_BAD_FORMAT - the file is not a valid VxD.
// 
//                ERROR_RESOURCE_DATA_NOT_FOUND - the VxD does not
//                contain a version resource.
// 
//                ERROR_INSUFFICIENT_BUFFER - the specified buffer size
//                was too small. After the function fails with this
//                error code, the value pointed to by lpdwSize will be
//                the required buffer size.
// 
//**********************************************************************

BOOL GetVxdVersion( LPSTR szFile, LPDWORD lpdwLen, LPVOID lpData ) {

  HANDLE hFile        = NULL;
  HANDLE hFileMapping = NULL;
  void * pView        = NULL;
  DWORD  dwSize       = 0;
  BOOL   bResult      = FALSE;
  DWORD  dwError      = 0;

  PIMAGE_DOS_HEADER       pDosExeHdr = NULL;
  PIMAGE_NT_HEADERS       pNtExeHdr  = NULL;
  PIMAGE_VXD_HEADER       pLEHdr     = NULL;
  PVXD_VERSION_RESOURCE   pVerRes    = NULL;
  LPVOID                  pRawRes    = NULL;

  // Wrap in a try..finally block to ensure resources are freed.
  __try {

     // Open the file for shared read access.
     hFile = CreateFile( szFile, GENERIC_READ, FILE_SHARE_READ,
           NULL, OPEN_EXISTING, 0, NULL );
     if ( hFile == INVALID_HANDLE_VALUE )
        __leave;

     // Create a read-only file mapping object for the file.
     hFileMapping = CreateFileMapping( hFile, NULL,
           PAGE_READONLY, 0, 0, NULL);
     if ( !hFileMapping )
        __leave;

     // Map a view of the the file.
     pView = MapViewOfFile( hFileMapping, FILE_MAP_READ, 0, 0, 0 );
     if ( !pView )
        __leave;

     // The DOS header begins at byte 0.
     pDosExeHdr = (PIMAGE_DOS_HEADER) pView;

     // Check to make sure the file has a DOS EXE header.
     if ( pDosExeHdr->e_magic != IMAGE_DOS_SIGNATURE ) {
        SetLastError( ERROR_BAD_FORMAT );
        __leave;
     }

     // Find the beginning of the NT header at offset e_lfanew.
     pNtExeHdr = (PIMAGE_NT_HEADERS) ( (ULONG_PTR) pView
           + (ULONG_PTR) pDosExeHdr->e_lfanew );

     // Check to make sure the file is a VxD.
     if ( (DWORD) pNtExeHdr->Signature != IMAGE_VXD_SIGNATURE ) {
        SetLastError( ERROR_BAD_FORMAT );
        __leave;
     }

     // The LE header begins at the same place as the NT header.
     pLEHdr = (PIMAGE_VXD_HEADER) pNtExeHdr;

     // e32_winreslen contains the size of the VxD's version resource.
     if ( pLEHdr->e32_winreslen == 0 ) {
        *lpdwLen = 0;
        SetLastError( ERROR_RESOURCE_DATA_NOT_FOUND );
        __leave;
     }

     // e32_winresoff contains the offset of the resource in the VxD.
     pVerRes = (VXD_VERSION_RESOURCE *) ( (ULONG_PTR) pView
           + (ULONG_PTR) pLEHdr->e32_winresoff );
     dwSize = pVerRes->dwResSize;
     pRawRes = &(pVerRes->bVerData);

     // Make sure the supplied buffer is large enough for the resource.
     if ( ( lpData == NULL ) || ( *lpdwLen < dwSize ) ) {
        *lpdwLen = dwSize;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        __leave;
     }

     // Zero the passed buffer and copy the resource into it.
     ZeroMemory( lpData, *lpdwLen );
     CopyMemory( lpData, pRawRes, dwSize );
     *lpdwLen = dwSize;
     SetLastError(0);
     bResult = TRUE;

  } __finally {

     // Save the last error, in case the clean up code modifies it.
     dwError = GetLastError();

     // Clean up resources.
     if ( pView )
        UnmapViewOfFile( pView );

     if ( hFileMapping )
        CloseHandle( hFileMapping );

     if ( hFile != INVALID_HANDLE_VALUE )
        CloseHandle( hFile );

     // Reset the last error to the saved value.
     SetLastError( dwError );
  }

  return bResult;
}


//**********************************************************************
// 
//  FUNCTION:     GetVxdVersionInfoSize - This function is analagous to
//                the GetFileVersionInfoSize() API.
// 
//  PARAMETERS:   szFile  - null-terminated string indicating the VxD
//                          file name.
// 
//  RETURN VALUE: If successful, this function returns the size in bytes
//                of the resource information for the VxD. Otherwise,
//                the function returns zero and GetLastError() will
//                indicate why the function failed. If the function
//                fails with ERROR_BAD_FORMAT, then the file was not a
//                valid VxD.
// 
//**********************************************************************

DWORD GetVxdVersionInfoSize( LPSTR szFile ) {

  DWORD dwResult = 0;

  // Call GetVxdVersion() with NULL for the pointer to the buffer.
  if ( !GetVxdVersion( szFile, &dwResult, NULL ) ) {

     DWORD dwError = GetLastError();

     // GetVxdVersion() will fail with ERROR_INSUFFICIENT_BUFFER and
     // the required buffer size will be returned in dwResult.
     if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {
        SetLastError( 0 );
        return dwResult;
     }
  }

  // The following line is never executed.
  return 0;
}


//**********************************************************************
// 
//  FUNCTION:     GetVxdVersionInfo - This function is analagous to the
//                GetFileVersionInfo() API. It is basically a
//                pass-through to GetVxDVersion, except that the dwLen
//                parameter is passed by reference.
// 
//  PARAMETERS:   szFile  - null-terminated string indicating the VxD
//                          file name.
// 
//                dwLen   - DWORD containing the size of the buffer.
// 
//                lpData  - pointer to buffer to receive the version
//                          resource.
// 
//  RETURN VALUE: TRUE if successful. Otherwise, FALSE and
//                GetLastError() will indicate why the function failed.
// 
//**********************************************************************

BOOL GetVxdVersionInfo( LPSTR szFile, DWORD dwLen, LPVOID lpData ) {

  return GetVxdVersion( szFile, &dwLen, lpData );
}


//**********************************************************************
// 
//  FUNCTION:     main - This is the entry point for the program. This
//                function contains some sample code demonstrating how
//                to use the VxD version functions above.
// 
//  PARAMETERS:   argc - the number of command-line arguments.
// 
//                argv - an array of null-terminated strings containing
//                       the command-line arguments.
// 
//  RETURN VALUE: none
// 
//**********************************************************************

void main( int argc, char *argv[] ) {

  void *   lpData;
  DWORD    dwResSize;
  BOOL     bSuccess;
  UINT     uFfiLen;
  char     szLangCp[9];
  LPDWORD  lpdwLangCp;
  UINT     uTranLen;
  char *   szVer;
  UINT     uVerLen;
  int      iIndex;
  int      iString;
  char     szSfi[255];
  VS_FIXEDFILEINFO *pFfi;

  char *VerStrings[] = { "CompanyName",
                         "FileDescription",
                         "FileVersion",
                         "InternalName",
                         "LegalCopyright",
                         "OriginalFilename",
                         "ProductName",
                         "ProductVersion",
                         "Comments",
                         "LegalTrademarks",
                         "PrivateBuild",
                         "SpecialBuild" };

  if ( argc < 2 ) {
     printf( "usage: VxdVer <filename>" );
     return;
  }

  dwResSize = GetVxdVersionInfoSize( argv[1] );
  if ( !dwResSize ) {
     printf( "GetVxdVersionInfoSize() failed with error %d.\n",
           GetLastError() );
     return;
  }

  lpData = VirtualAlloc( NULL, dwResSize, MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE );

  if ( lpData ) {

     __try {

        bSuccess = GetVxdVersionInfo( argv[1], dwResSize, lpData );
        if ( !bSuccess ) {
           printf( "GetVxdVersionInfo() failed with error %d.\n",
                 GetLastError() );
           __leave;
        }

        bSuccess = VerQueryValue( lpData, "\\", (void **) &pFfi,
              &uFfiLen );
        if ( !bSuccess ) {
           printf( "VerQueryValue() failed with error %d.\n",
                 GetLastError() );
           __leave;
        }

        printf("VS_FIXEDFILEINFO Structure\n");
        printf("--------------------------\n");
        printf("dwSignature = %lu\n", pFfi->dwSignature);
        printf("dwStrucVersion = %lu\n", pFfi->dwStrucVersion);
        printf("dwFileVersionMS = %lu\n", pFfi->dwFileVersionMS);
        printf("dwFileVersionLS = %lu\n", pFfi->dwFileVersionLS);
        printf("dwProductVersionMS = %lu\n", pFfi->dwProductVersionMS);
        printf("dwProductVersionLS = %lu\n", pFfi->dwProductVersionLS);
        printf("dwFileFlagsMask = %lu\n", pFfi->dwFileFlagsMask);
        printf("dwFileFlags = %lu\n", pFfi->dwFileFlags);
        printf("dwFileOS = %lu\n", pFfi->dwFileOS);
        printf("dwFileType = %lu\n", pFfi->dwFileType);
        printf("dwFileSubtype = %lu\n", pFfi->dwFileSubtype);
        printf("dwFileDateMS = %lu\n", pFfi->dwFileDateMS);
        printf("dwFileDateLS = %lu\n", pFfi->dwFileDateLS);

        bSuccess = VerQueryValue( lpData, "\\VarFileInfo\\Translation",
              (void **) &lpdwLangCp, &uTranLen );
        if ( !bSuccess ) {
           printf( "VerQueryValue() failed with error %d.\n",
                 GetLastError() );
           __leave;
        }

        for ( iIndex = 0; iIndex < (int)( uTranLen / sizeof(void*) );
              iIndex++ ) {

           int iItems = sizeof(VerStrings)/sizeof(char *);

           // Flip the words to display lang first.
           wsprintf( szLangCp, "%08X",
                 SWAPWORDS( *(lpdwLangCp + iIndex) ) );
           printf( "\nlang-codepage = %s\n", szLangCp );
           printf( "------------------------\n" );

           // Cycle through possible string values.
           for ( iString = 0; iString < iItems; iString++ ) {
              wsprintf( szSfi, "\\StringFileInfo\\%s\\%s", szLangCp,
                    VerStrings[iString] );
              bSuccess = VerQueryValue( lpData, szSfi,
                    (void **) &szVer, &uVerLen );
              if ( bSuccess )
                 printf( "%s = %s\n", VerStrings[iString], szVer );
           }
        }

     } __finally {
        VirtualFree( lpData, dwResSize, MEM_RELEASE );
     }
  }
}
