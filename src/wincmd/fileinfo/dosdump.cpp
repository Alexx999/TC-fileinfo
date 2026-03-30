#include "stdafx.h"
#include "pedump\common.h" 
#include "peexe\exefile.h"

//
// Dump the IMAGE_FILE_HEADER for a DOS EXE file
//
CString DumpMZHeader( PIMAGE_DOS_HEADER pdosHeader )
{
    CString str="", strTemp="";    int j;
   if (pdosHeader->e_res[0] == LZ)
      str += (CString) "\tCompressed Executable by LZEXE"+ rtfReturn+ rtfReturn;
   if (pdosHeader->e_res[1] == PK)
      str += (CString) "\tCompressed Executable by PKLITE"+ rtfReturn+ rtfReturn;

    str += (CString) ("Header Information :")+ rtfReturn+ rtfReturn;
    strTemp.Format("\t%s\t%04Xh ", "Signature :", pdosHeader->e_magic);
   str += strTemp + rtfReturn;
/*   switch(pdosHeader->e_magic)
   {
      case IMAGE_DOS_SIGNATURE: strTemp = "( DOS : normal 16-bit ) \r\n"; break;
//      case 0x20b: strTemp = "( PE32+ : 64-bit ready ) \r\n"; break;
//      case 0x107: strTemp = "( ROM image ) \r\n"; break;
   } 
   str += strTemp;/**/
    strTemp.Format("\t%s\t%04Xh ", "Bytes on last page of file :", pdosHeader->e_cblp);
   str += strTemp + rtfReturn;
    strTemp.Format("\t%s\t%04Xh ", "Total Pages in File :", pdosHeader->e_cp);
   str += strTemp + rtfReturn;
    strTemp.Format("\t%s\t%04Xh ", "Relocation Items :", pdosHeader->e_crlc);
   str += strTemp + rtfReturn;
    strTemp.Format("\t%s\t%04Xh ", "Size of header in paragraphs :", pdosHeader->e_cparhdr);
   str += strTemp + rtfReturn;
    strTemp.Format("\t%s\t%04Xh ", "Minimum Extra Paragraphs :", pdosHeader->e_minalloc);
   str += strTemp + rtfReturn;
    strTemp.Format("\t%s\t%04Xh ", "Maximum Extra Paragraphs :", pdosHeader->e_maxalloc);
   str += strTemp + rtfReturn;
    strTemp.Format("\t%s\t%04Xh ", "Initial Stack Segment :", pdosHeader->e_ss);
   str += strTemp + rtfReturn;
    strTemp.Format("\t%s\t%04Xh ", "Initial Stack Pointer :", pdosHeader->e_sp);
   str += strTemp + rtfReturn;
    strTemp.Format("\t%s\t%04Xh ", "Complemented Checksum :", pdosHeader->e_csum);
   str += strTemp + rtfReturn;
    strTemp.Format("\tInitial Instruction Pointer :\t%04Xh ", pdosHeader->e_ip);
   str += strTemp + rtfReturn;
    strTemp.Format("\tInitial Code Segment :\t%04Xh ", pdosHeader->e_cs);
   str += strTemp + rtfReturn;
    strTemp.Format("\tRelocation Table Offset :\t%04Xh ", pdosHeader->e_lfarlc);
    str += strTemp + rtfReturn;
    strTemp.Format("\tOverlay Number :\t%04Xh ", pdosHeader->e_ovno);
    str += strTemp + rtfReturn;
	
	str += rtfReturn;
    str += (CString) ("Extra Header Information :")+ rtfReturn;
/***************   OEM Text
   char *pStr = (char *) pdosHeader->e_res;
   strTemp = "";
   for(int j=0; j<32; j++)
      if ((pStr[j] <= 0x21) || (pStr[j] >= 0x7D)) continue;
      else strTemp += pStr[j];
   str += strTemp;str += "\r\n";
****************************/
   for (j=0; j<4; j++)
   {
      strTemp.Format("\tReserved WORD %d:\t%04Xh ", j, pdosHeader->e_res[j]);
      str += strTemp + rtfReturn;
   }
    strTemp.Format("\tOEM identifier :\t%04Xh ", pdosHeader->e_oemid);
    str += strTemp + rtfReturn;
    strTemp.Format("\tOEM information :\t%04Xh ", pdosHeader->e_oeminfo);
    str += strTemp + rtfReturn;
   for (j=0; j<10; j++)
   {
      strTemp.Format("\tReserved WORD %d:\t%04Xh ", j, pdosHeader->e_res2[j]);
      str += strTemp + rtfReturn;
   }
    strTemp.Format("\tNew Header Address :\t%08Xh ", pdosHeader->e_lfanew);
   str += strTemp + rtfReturn;

   ULONG size = pdosHeader->e_cp;
   size *= 512; size += pdosHeader->e_cblp;
    strTemp.Format("\t%s\t%lu B ( %lu KB )", "Memory Needed :", size, size >> 10);
   str += strTemp + rtfReturn;

   return str;
}

#define NENOTP     0x8000     /* Not a process */
#define NEIERR     0x2000     /* Errors in image */
#define NEBOUND     0x0800     /* Bound as family app */
#define NEAPPTYP     0x0700     /* Application type mask */
#define NENOTWINCOMPAT  0x0100     /* Not compatible with P.M. Windowing */
#define NEWINCOMPAT     0x0200     /* Compatible with P.M. Windowing */
#define NEWINAPI     0x0300     /* Uses P.M. Windowing API */
#define NEFLTP     0x0080     /* Floating-point instructions */
#define NEI386     0x0040     /* 386 instructions */
#define NEI286     0x0020     /* 286 instructions */
#define NEI086     0x0010     /* 8086 instructions */
#define NEPROT     0x0008     /* Runs in protected mode only */
#define NEPPLI     0x0004     /* Per-Process Library Initialization */
#define NEINST     0x0002     /* Instance data */
#define NESOLO     0x0001     /* Solo data */

#define    NE_UNKNOWN  0  // Unknown (any "new-format" OS)
#define    NE_OS2      1  // Microsoft/IBM OS/2
#define    NE_WINDOWS  2  // Microsoft Windows
#define    NE_DOS4     3  // Microsoft MS-DOS 4.x
#define    NE_DEV386   4  // Microsoft Windows 386
#define    NE_BOSS     5  // BOSS (Borland Operating System Services)
#define    NE_PL286  129  // PharLap 286|DOS-Extender, OS/2
#define    NE_PL386  130  // PharLap 286|DOS-Extender, Windows

// DOS/4GW  'Linear Executable' LE and LX.

CString DumpNEHeader( PIMAGE_DOS_HEADER dosHeader )
{
    CString str="", strTemp="";
    PIMAGE_OS2_HEADER pNEHeader;
    DWORD base = (DWORD)dosHeader;

    pNEHeader = MakePtr( PIMAGE_OS2_HEADER, dosHeader, dosHeader->e_lfanew );
    if ( pNEHeader->ne_magic != IMAGE_OS2_SIGNATURE )
      return (CString) "Not a New Executable (NE) File"+ rtfReturn;

   str += (CString) ("File Characteristics : ")+ rtfReturn;
   if (pNEHeader->ne_flags & NESOLO) 
      str += (CString) ("\t\tFile is a dynamic-link library (DLL) (SINGLEDATA)")+ rtfReturn;
   if (pNEHeader->ne_flags & NEINST) 
      str += (CString) ("\t\tFile is a Windows application (MULTIPLEDATA)")+ rtfReturn;
   if (!(pNEHeader->ne_flags & 3))
      str += (CString) ("\t\tNOAUTODATA")+ rtfReturn;
   if (pNEHeader->ne_flags & NEPPLI) str += (CString) ("\t\tFile uses Per-process library initialization")+ rtfReturn; // Per-process library initialization
   if (pNEHeader->ne_flags & NEPROT) str += (CString) ("\t\tFile uses Protected mode only")+ rtfReturn;
   if (pNEHeader->ne_flags & NEI086) str += (CString) ("\t\tFile uses 8086 instructions")+ rtfReturn;
   if (pNEHeader->ne_flags & NEI286) str += (CString) ("\t\tFile uses 286 instructions")+ rtfReturn;
   if (pNEHeader->ne_flags & NEI386) str += (CString) ("\t\tFile uses 386 instructions")+ rtfReturn;
   if (pNEHeader->ne_flags & NEFLTP) str += (CString) ("\t\tFile uses Floating-point instructions")+ rtfReturn;
   if ((pNEHeader->ne_flags & NEAPPTYP) == 0) str += (CString) ("\t\tFull screen (not aware of Windows/P.M. API)")+ rtfReturn;
   if ((pNEHeader->ne_flags & NEAPPTYP) ==  NEWINAPI) str += (CString) ("\t\tFile uses P.M. Windowing API")+ rtfReturn;
   if ((pNEHeader->ne_flags & NEAPPTYP) ==  NEWINCOMPAT) str += (CString) ("\t\tFile is compatible with P.M. Windowing")+ rtfReturn;
   if ((pNEHeader->ne_flags & NEAPPTYP) ==  NENOTWINCOMPAT) str += (CString) ("\t\tFile is not compatible with P.M. Windowing")+ rtfReturn;
   if (pNEHeader->ne_flags & (1 << 11)) str += (CString) ("\t\tBound as family app")+ rtfReturn;
   // (pNEHeader->ne_flags & (1 << 12)) str += (CString) ("\t\tUnused"+ rtfReturn);
   if (pNEHeader->ne_flags & NEIERR) str += (CString) ("\t\tFile has errors in image")+ rtfReturn;
   // (pNEHeader->ne_flags & (1 << 14)) str += (CString) ("\t\tUnused"+ rtfReturn);
   if (pNEHeader->ne_flags & (1 << 15)) str += (CString) ("\t\tFile is a Library Module ")+ rtfReturn;
   if (pNEHeader->ne_flags & NENOTP) str += (CString) ("\t\tFile is not a process ")+ rtfReturn;

    str += (CString) ("Header Information :")+ rtfReturn+ rtfReturn;

   char szModuleName[300];
   lstrcpyn(szModuleName, 
      (LPSTR)pNEHeader + pNEHeader->ne_restab +1,
        min((BYTE)*((LPSTR)pNEHeader + pNEHeader->ne_restab) + 1,300));

    strTemp.Format("\tDescription :\t%s  ", szModuleName);
   str += strTemp + rtfReturn;

    strTemp.Format("\tSignature :\t%04Xh  ( 16bit for Windows 3.x ) ", pNEHeader->ne_magic);
   str += strTemp + rtfReturn;
    
   str += ("\tTarget Operating system :\t");
//   strTemp.Format("Operating System :");
//   str += strTemp;
   switch(pNEHeader->ne_exetyp)
   {
      case NE_UNKNOWN : strTemp = "Unknown (any \"new-format\" OS) "; break;
      case NE_OS2     : strTemp = "Microsoft/IBM OS/2 "; break;
      case NE_WINDOWS : strTemp = "Microsoft Windows  "; break;
      case NE_DOS4    : strTemp = "Microsoft MS-DOS 4.x  "; break;
      case NE_DEV386  : strTemp = "Microsoft Windows 386 "; break;
      case NE_BOSS    : strTemp = "BOSS (Borland Operating System Services) "; break;
      case NE_PL286   : strTemp = "PharLap 286|DOS-Extender, OS/2  "; break;
      case NE_PL386   : strTemp = "PharLap 286|DOS-Extender, Windows  "; break;
      default :   strTemp.Format("Unknown \"new-format\" OS ( %u )", pNEHeader->ne_exetyp);
   }   
   str += strTemp + rtfReturn;
   UINT major = pNEHeader->ne_expver >> 8;
   UINT minor = pNEHeader->ne_expver - (major << 8);
    strTemp.Format("\tExpected Windows Version :\t%u.%02u ", major, minor);
   str += strTemp + rtfReturn;
    strTemp.Format("\tLinker Version :\t%u.%02u ", pNEHeader->ne_ver, pNEHeader->ne_rev);
   str += strTemp + rtfReturn;
    strTemp.Format("\tChecksum of whole file :\t%08Xh ", pNEHeader->ne_crc);
   str += strTemp + rtfReturn;
    strTemp.Format("\tAutomatic Data Segment :\t%x ", pNEHeader->ne_autodata);
   str += strTemp + rtfReturn;
    strTemp.Format("\tInitial heap allocation :\t%04Xh ", pNEHeader->ne_heap);
   str += strTemp + rtfReturn;
    strTemp.Format("\tInitial stack allocation :\t%04Xh ", pNEHeader->ne_stack);
   str += strTemp + rtfReturn;
    strTemp.Format("\tInitial CS:IP setting :\t%04Xh:%04Xh ", ( pNEHeader->ne_csip >> 16) & 0xFFFF, pNEHeader->ne_csip & 0xFFFF);
   str += strTemp + rtfReturn;
    strTemp.Format("\tInitial SS:SP setting :\t%04Xh:%04Xh ", ( pNEHeader->ne_sssp >> 16) & 0xFFFF, pNEHeader->ne_sssp & 0xFFFF);
   str += strTemp + rtfReturn;
    strTemp.Format("\tSegment Table : \toffset - %08Xh ", pNEHeader->ne_segtab);
   str += strTemp + rtfReturn;
    strTemp.Format("\t\tentries - %04Xh ", pNEHeader->ne_cseg);
   str += strTemp + rtfReturn;
    strTemp.Format("\tResource Table : \toffset - %08Xh ", pNEHeader->ne_rsrctab);
   str += strTemp + rtfReturn;
    strTemp.Format("\t\tentries - %04Xh ", pNEHeader->ne_cres);
   str += strTemp + rtfReturn;
    strTemp.Format("\tResident Name Table : \toffset - %08Xh ", pNEHeader->ne_restab);
   str += strTemp + rtfReturn;
    strTemp.Format("\tModule Reference Table : \toffset - %08Xh ", pNEHeader->ne_modtab);
   str += strTemp + rtfReturn;
    strTemp.Format("\t\tentries - %04Xh ", pNEHeader->ne_cmod);
   str += strTemp + rtfReturn;
    strTemp.Format("\tImported Names Table : \toffset - %08Xh ", pNEHeader->ne_imptab);
   str += strTemp + rtfReturn;
    strTemp.Format("\tEntries Table : \toffset - %08Xh ", pNEHeader->ne_enttab);
   str += strTemp + rtfReturn;
    strTemp.Format("\t\tlength - %04Xh ", pNEHeader->ne_cbenttab);
   str += strTemp + rtfReturn;
    strTemp.Format("\tNon-resident Names Table : \toffset - %08Xh ", pNEHeader->ne_nrestab);
   str += strTemp + rtfReturn;
    strTemp.Format("\t\tlength - %04Xh ", pNEHeader->ne_cbnrestab);
   str += strTemp + rtfReturn;
    strTemp.Format("\tCount of movable entries :\t%x ", pNEHeader->ne_cres);
   str += strTemp + rtfReturn;
    strTemp.Format("\tSegment alignment shift count :\t%x ", pNEHeader->ne_align);
   str += strTemp + rtfReturn;
    strTemp.Format("\tAlignment :\t%u bytes", 1 << pNEHeader->ne_align );
   str += strTemp + rtfReturn;

   str += rtfReturn;
   strTemp.Format(" \tFlag word :\t%x ", pNEHeader->ne_flags);
   str += strTemp;str += rtfReturn;

   strTemp.Format("\tOther .EXE flags :\t%x ", pNEHeader->ne_flagsothers);
   str += strTemp + rtfReturn;
   if (pNEHeader->ne_flagsothers & (1)) //Windows 2.x application that runs in version 3.x protected mode
      str += (CString) ("\t\tsupports long filenames") + rtfReturn;
   if (pNEHeader->ne_flagsothers & (1 << 1)) //Windows 2.x application that runs in version 3.x protected mode
      str += (CString) ("\t\t2.X protected mode" )+ rtfReturn;
   if (pNEHeader->ne_flagsothers & (1 << 2)) //Windows 2.x application that supports proportional fonts.
      str += (CString) ("\t\t2.X proportional font") + rtfReturn;
   if (pNEHeader->ne_flagsothers & (1 << 3)) //contains a fast-load area.
      str += (CString) ("\t\tfast-load area" )+ rtfReturn;

    strTemp.Format("\toffset to return thunks :\t%x ", pNEHeader->ne_pretthunks);
   str += strTemp + rtfReturn;
    strTemp.Format("\toffset to segment ref. bytes :\t%x ", pNEHeader->ne_psegrefbytes);
   str += strTemp + rtfReturn;
    strTemp.Format("\tMinimum code swap area size :\t%x ", pNEHeader->ne_swaparea);
   str += strTemp + rtfReturn;

   return str;
}

#define    LXAPPTYP 0x38000
#define    LXVDX    0x38000     /* */
#define    LXVDDM     0x28000     /* Virtual Device Driver module    101000000000000000*/
#define    LXPDDM     0x20000     /* Physical Device Driver module   100000000000000000*/
#define    LXPMLM     0x18000     /* Protected Memory Library module  11000000000000000 */
CString DumpCommonLEHeader( PIMAGE_VXD_HEADER pLXHeader )
{
    CString str="", strTemp="";
   str += (CString) ("File Characteristics : " )+ rtfReturn;
   if (pLXHeader->e32_mflags & NEPPLI) str += (CString) ("\t\tFile uses Per-process library initialization" )+ rtfReturn; // Per-process library initialization
   if ((pLXHeader->e32_mflags  & NEAPPTYP) ==  NEWINAPI) str += (CString) ("\tFile uses P.M. Windowing API" )+ rtfReturn;
   if ((pLXHeader->e32_mflags  & NEAPPTYP) ==  NEWINCOMPAT) str += (CString) ("\t\tFile is compatible with P.M. Windowing" )+ rtfReturn;
   if ((pLXHeader->e32_mflags  & NEAPPTYP) ==  NENOTWINCOMPAT) str += (CString) ("\t\tFile is not compatible with P.M. Windowing" )+ rtfReturn;
   if (pLXHeader->e32_mflags  & NEIERR) str += (CString) ("\t\tFile contains errors in image" )+ rtfReturn;
   if (pLXHeader->e32_mflags & NENOTP) str += (CString) ("\t\tFile is not a process " )+ rtfReturn;
/*   if ((pLXHeader->e32_mflags  & LXAPPTYP) ==  LXPMLM) str += ("\t\tFile is a Protected Memory Library module\r\n");
   if ((pLXHeader->e32_mflags  & LXAPPTYP) ==  LXPDDM) str += ("\t\tFile is a Physical Device Driver module\r\n");
   if ((pLXHeader->e32_mflags  & LXAPPTYP) ==  LXVDDM) str += ("\t\tFile is a Virtual Device Driver module\r\n");
*/
   switch(pLXHeader->e32_mflags  & LXAPPTYP)
   {
      case LXPMLM : str += (CString) ("\t\tFile is a Protected Memory Library module" )+ rtfReturn; break;
      case LXPDDM : str += (CString) ("\t\tFile is a Physical Device Driver module" )+ rtfReturn;break;
      case LXVDDM : str += (CString) ("\t\tFile is a Virtual Device Driver module" )+ rtfReturn;break;
      case LXVDX  : str += (CString) ("\t\tFile is a Virtual Device Driver module" )+ rtfReturn;break;
   }
    str += ("HEADER INFORMATION :");
	str += rtfReturn;str += rtfReturn;

    strTemp.Format("\tSignature :\t%04Xh ", pLXHeader->e32_magic);
   str += strTemp + rtfReturn;

    strTemp.Format("\tCPU type :\t%04Xh ", pLXHeader->e32_cpu);
   str += strTemp + rtfReturn;
   switch(pLXHeader->e32_cpu)
   {
      case 0: str += "\t\tUndefined "; break; 
      case 1: str += "\t\t80286 and newer or compatible "; break;
      case 2: str += "\t\t80386 and newer or compatible "; break;
      case 3: str += "\t\t80486 and newer or compatible "; break;
      case 4: str += "\t\tPentium class CPU or Suprerior "; break;
      case 0x20 : str += "\t\tIntel i860 (N10) or compatible "; break;
      case 0x21 : str += "\t\tIntel \"N11\" or compatible"; break;
        case 0x40 : str += "\t\tMIPS Mark I ( R2000, R3000) or compatible"; break;
        case 0x41 : str += "\t\tMIPS Mark II ( R6000 ) or compatible"; break;
        case 0x42 : str += "\t\tMIPS Mark III ( R4000 ) or compatible"; break;
      default: str += "\t\t ?????? "; break;
   }
   str += rtfReturn;
   str += ("\tTarget Operating system :\t");
   switch(pLXHeader->e32_os)
   {
      case NE_UNKNOWN : str += "Unknown (any \"new-format\" OS) "; break;
      case NE_OS2     : str += "Microsoft/IBM OS/2 "; break;
      case NE_WINDOWS : str += "Microsoft Windows  "; break;
      case NE_DOS4    : str += "Microsoft MS-DOS 4.x  "; break;
      case NE_DEV386  : str += "Microsoft Windows 386  "; break;
      default : strTemp.Format("Unknown \"new-format\" OS ( %u )", pLXHeader->e32_os);         
		  str += strTemp;
   }   
   
	str += rtfReturn;
    strTemp.Format("\tVersion Number :\t%04Xh ", pLXHeader->e32_ver);
   str += strTemp + rtfReturn;
//   strTemp.Format("\tRevision Number :\t%04X ", pLXHeader->ne_rev);
//   str += strTemp + rtfReturn;
   strTemp.Format("\tFlag word :\t%08xh ", pLXHeader->e32_mflags);
   str += strTemp + rtfReturn;
   strTemp.Format("\tNumber of Module pages :\t%u ", pLXHeader->e32_mpages);
   str += strTemp + rtfReturn;
   strTemp.Format("\tObject for EIP :\t%u ", pLXHeader->e32_mpages);
   str += strTemp + rtfReturn;
   strTemp.Format("\tExtended instruction pointer (EIP) :\t%u ", pLXHeader->e32_eip);
   str += strTemp + rtfReturn;
   strTemp.Format("\tObject for ESP :\t%u ", pLXHeader->e32_stackobj);
   str += strTemp + rtfReturn;
   strTemp.Format("\tExtended stack pointer (ESP) :\t%u ", pLXHeader->e32_esp);
   str += strTemp + rtfReturn;
   strTemp.Format("\tPage size :\t%u ", pLXHeader->e32_pagesize);
   str += strTemp + rtfReturn;
   strTemp.Format("\tLast Page size :\t%u ", pLXHeader->e32_lastpagesize);
   str += strTemp + rtfReturn;

   strTemp.Format("\tFixup section size :\t%u ", pLXHeader->e32_fixupsize);
   str += strTemp + rtfReturn;
   strTemp.Format("\tFixup section checksum :\t%08xh ", pLXHeader->e32_fixupsum);
   str += strTemp + rtfReturn;
   strTemp.Format("\tLoader section size :\t%u ", pLXHeader->e32_ldrsize);
   str += strTemp + rtfReturn;
   strTemp.Format("\tLoader section checksum :\t%08xh ", pLXHeader->e32_ldrsum);
   str += strTemp + rtfReturn;
   strTemp.Format("\tObject table offset :\t%08xh ", pLXHeader->e32_objtab);
   str += strTemp + rtfReturn;
   strTemp.Format("\tNumber of objects in module :\t%u ", pLXHeader->e32_objcnt);
   str += strTemp + rtfReturn;
   strTemp.Format("\tObject page map offset :\t%08xh ", pLXHeader->e32_objmap);
   str += strTemp + rtfReturn;
   strTemp.Format("\tObject iterated data map offset :\t%08xh ", pLXHeader->e32_itermap);
   str += strTemp + rtfReturn;
   strTemp.Format("\tResource Table Offset :\t%08xh ", pLXHeader->e32_rsrctab);
   str += strTemp + rtfReturn;
   strTemp.Format("\tNumber of resource entries :\t%u ", pLXHeader->e32_rsrccnt);
   str += strTemp + rtfReturn;
   strTemp.Format("\tResident name table Offset :\t%08xh ", pLXHeader->e32_restab);
   str += strTemp + rtfReturn;
   strTemp.Format("\tEntry Table Offset :\t%08xh ", pLXHeader->e32_enttab);
   str += strTemp + rtfReturn;
   strTemp.Format("\tModule Directive Table Offset :\t%08xh ", pLXHeader->e32_dirtab);
   str += strTemp + rtfReturn;
   strTemp.Format("\tNumber of module directives :\t%u ", pLXHeader->e32_dircnt);
   str += strTemp + rtfReturn;
   strTemp.Format("\tFixup Page Table Offset :\t%08xh ", pLXHeader->e32_fpagetab);
   str += strTemp + rtfReturn;
   strTemp.Format("\tFixup Record Table Offset :\t%08xh ", pLXHeader->e32_frectab);
   str += strTemp + rtfReturn;
   strTemp.Format("\tImport Module Name Table Offset :\t%08xh ", pLXHeader->e32_impmod);
   str += strTemp + rtfReturn;
   strTemp.Format("\t    Number of entries in IMNT :\t%u ", pLXHeader->e32_impmodcnt);
   str += strTemp + rtfReturn;
   strTemp.Format("\tImport Proc Name Table Offset :\t%08xh ", pLXHeader->e32_impproc);
   str += strTemp + rtfReturn;
   strTemp.Format("\tPer-Page Checksum Table Offset :\t%08xh ", pLXHeader->e32_pagesum);
   str += strTemp + rtfReturn;
   strTemp.Format("\tEnumerated Data Pages Offset :\t%08xh ", pLXHeader->e32_datapage);
   str += strTemp + rtfReturn;
   strTemp.Format("\tNumber of preload pages :\t%u ", pLXHeader->e32_preload);
   str += strTemp + rtfReturn;
   strTemp.Format("\tNon-resident Names Table Offset :\t%08xh ", pLXHeader->e32_nrestab);
   str += strTemp + rtfReturn;
   strTemp.Format("\t     Size of NRNT :\t%u ", pLXHeader->e32_cbnrestab);
   str += strTemp + rtfReturn;
   strTemp.Format("\t     NRNT Checksum :\t%08xh ", pLXHeader->e32_nressum);
   str += strTemp + rtfReturn;
   strTemp.Format("\tObject # for automatic data object :\t%u ", pLXHeader->e32_autodata);
   str += strTemp + rtfReturn;
   strTemp.Format("\tDebugging information Offset :\t%08xh ", pLXHeader->e32_debuginfo);
   str += strTemp + rtfReturn;
   strTemp.Format("\tLength of the debugging info :\t%u bytes ", pLXHeader->e32_debuglen);
   str += strTemp + rtfReturn;
   strTemp.Format("\tPages in preload section :\t%u ", pLXHeader->e32_instpreload);
   str += strTemp + rtfReturn;
   strTemp.Format("\tPages in demand-load section :\t%u ", pLXHeader->e32_instdemand);
   str += strTemp + rtfReturn;
   strTemp.Format("\tSize of heap - for 16-bit apps :\t%u ", pLXHeader->e32_heapsize);
   str += strTemp + rtfReturn;
   
   return str;
}

CString DumpLXHeader( EXE_FILE *pEXE )
{
    CString str="", strTemp="";
	PIMAGE_DOS_HEADER pdosHeader = pEXE->GetdosHeader();
    PIMAGE_VXD_HEADER pLXHeader;
    pLXHeader = MakePtr( PIMAGE_VXD_HEADER, pdosHeader, pdosHeader->e_lfanew );
    if ( pLXHeader->e32_magic != IMAGE_LX_SIGNATURE )
      return (CString) "Not a Linear eXecutable (LX) File"+ rtfReturn;

   str += DumpCommonLEHeader( pLXHeader );

   return str;
}

typedef struct _VxD_Desc_Block {      
	DWORD     Next                ; // VMM RESERVED FIELD
	WORD	  SDK_Version         ; // VMM RESERVED FIELD
	WORD      Req_Device_Number   ; // Required device number
	BYTE      Dev_Major_Version   ; // Major device number
	BYTE      Dev_Minor_Version   ; // Minor device number
	WORD      Flags               ; // Flags for init calls complete
	BYTE      Name                ; // Device name
	DWORD     Init_Order          ; // Initialization Order
	DWORD     Control_Proc        ; // Offset of control procedure
	DWORD     V86_API_Proc        ; // Offset of API procedure (or 0)
	DWORD     PM_API_Proc         ; // Offset of API procedure (or 0)
	DWORD     V86_API_CSIP        ; // CS:IP of API entry point
	DWORD     PM_API_CSIP         ; // CS:IP of API entry point
	DWORD     Reference_Data      ; // Reference data from real mode
	DWORD     Service_Table_Ptr   ; // Pointer to service table
	DWORD     Service_Table_Size  ; // Number of services
} VxD_Desc_Block, *PVxD_Desc_Block;

/***
dd DWORD
db byte
dw WORD
***/
#pragma pack( 1 )

typedef struct _Entry_Point {      
	BYTE   nb;
	BYTE   type; // data size
	WORD   object;
	BYTE   flags; // data size
	union { WORD o16; DWORD   o32; } offset;
}  Entry_Point, *PEntry_Point;

typedef struct _Entry_Data {
	BYTE   flags; // data size
	union { WORD o16; DWORD   o32; } offset;
}  Entry_Data, *PEntry_Data;

typedef struct LE_Object_Table_Define
{
	DWORD     Virtual_Segment_Size;
	DWORD     Relocation_Base_Address;
    DWORD     FLAGS;
    DWORD     Page_MAP_Index;
    DWORD     Page_MAP_Entries;
    DWORD     Reserved;
}LE_Object_Table, *PLE_Object_Table;

typedef struct LE_Object_Page_Table_Define
{
    DWORD   offset;
	WORD	size;
	WORD    flags;
}LE_Object_Page_Table, *PLE_Object_Page_Table;

typedef struct LE_ResourceTable_Entrie
{
    WORD	type;
	WORD	name;
	DWORD   size;
	WORD	object;
	DWORD   offset;
}LE_ResourceTable_Entrie, *PLE_ResourceTable_Entrie;

#define LEREAD     0x00000001    /* Readable */
#define LEWRIT     0x00000002    /* Writable */
#define LEEXEC     0x00000004    /* Executable */
#define LERESO     0x00000008    /* Resource */
#define LEDISC     0x00000010    /* Discardable */
#define LESHAR     0x00000020    /* Shared */
#define LEPREL     0x00000040    /* Preloaded */
#define LEINVA     0x00000080    /* Invalid */

#define LETMSK     0x00000300     /* Type mask */
#define LENORM     0x00000000     /* normal */
#define LEZFIL     0x00000100     /* Zero-filled */
#define LERESI     0x00000200     /* Resident */
#define LERSCO     0x00000300     /* Resident/contiguous */

#define LERESI     0x00000200    /* "RESIDENT/LONG_LOCABLE" */
#define LEALIA     0x00001000    /* 16:16_ALIAS" */
#define LEBIG      0x00002000    /* "BIG" (USE32) */
#define LECONF     0x00004000    /* Conforming */
#define LEIOPL     0x00008000    /* Object_i/o_privilage_level */

CString DumpLEHeader( EXE_FILE *pEXE )
{
	CString str="", strTemp="";
	PIMAGE_DOS_HEADER pdosHeader = pEXE->GetdosHeader();
    PIMAGE_VXD_HEADER pLXHeader;
    pLXHeader = MakePtr( PIMAGE_VXD_HEADER, pdosHeader, pdosHeader->e_lfanew );
    if ( pLXHeader->e32_magic != IMAGE_LE_SIGNATURE )
      return (CString) "Not a Linear Executable (LE) File"+ rtfReturn;

	str += DumpCommonLEHeader( pLXHeader );


    strTemp.Format("\tDevice ID for VxD :\t%04X ", pLXHeader->e32_devid);
	str += strTemp + rtfReturn;
	UINT major = pLXHeader->e32_ddkver >> 8;
	UINT minor = pLXHeader->e32_ddkver - (major << 8);

    strTemp.Format("\tReserved :\t%12s ", pLXHeader->e32_res3);
	str += strTemp + rtfReturn;
    strTemp.Format("\tVersionInfo Resource offset:\t%08xh ", pLXHeader->e32_winresoff);
	str += strTemp + rtfReturn;
    strTemp.Format("\tVersionInfo Resource length:\t%u ", pLXHeader->e32_winreslen);
	str += strTemp + rtfReturn;
	
	str += rtfReturn;str += "IMPORT TABLE :\t";
	if (pLXHeader->e32_impmodcnt)
	{
		PBYTE PEXPTBL = MakePtr( PBYTE, pLXHeader, pLXHeader->e32_impmod);
		if (pEXE->IsValidPtr((ULONG_PTR) PEXPTBL)) {
			BYTE size = *PEXPTBL;
			while (size)
			{
				char *name = new char[size+1];
				char *tpname = (char *) (PEXPTBL + 1);
				strncpy(name, tpname, size);
				name[size]='\0';
				str += rtfReturn;
				strTemp.Format("\t\t%s ", name);
				str += strTemp + rtfReturn;
				delete [] name;
				PEXPTBL += size + 1;
				size = *PEXPTBL;
			}
		} else str += (CString) "error in import table"+ rtfReturn;
	}	else str += (CString) "none"+ rtfReturn;

	str += rtfReturn;
	str += "IMPORT proc TABLE :\t";
	if (pLXHeader->e32_impmodcnt)
	{
		PBYTE PEXPTBL = MakePtr( PBYTE, pLXHeader, pLXHeader->e32_impproc);
		if (pEXE->IsValidPtr((ULONG_PTR)PEXPTBL)) {
			BYTE size = *PEXPTBL;
			while (size)
			{
				char *name = new char[size+1];
				char *tpname = (char *) (PEXPTBL + 1);
				strncpy(name, tpname, size);
				name[size]='\0';
				str += rtfReturn;
				strTemp.Format("\t\t%s ", name);
				str += strTemp + rtfReturn;
				delete [] name;
				PEXPTBL += size + 1;
				size = *PEXPTBL;			
			} 
		} else str += (CString) "error in import proc table"+ rtfReturn;
	}	else str += (CString) "none"+ rtfReturn;

	str += rtfReturn;
	str += "EXPORT TABLE : (Resident)\t";
	if (pLXHeader->e32_restab)
	{
/*
           +-----+-----+-----+-----+     +-----+-----+-----+
       00h | LEN |    ASCII STRING  . . .      | ORDINAL # |
           +-----+-----+-----+-----+     +-----+-----+-----+
*/
		PBYTE PEXPTBL = MakePtr( PBYTE, pLXHeader, pLXHeader->e32_restab);
		if (pEXE->IsValidPtr((ULONG_PTR)PEXPTBL)) {
			BYTE size = *PEXPTBL;
//			size &= 0x01111111; // A voir
			if (size)
			{
				char *name = new char[size+1];
				char *tpname = (char *) (PEXPTBL + 1);
				strncpy(name, tpname, size);
				name[size]='\0';
				WORD ord = * (WORD *)(PEXPTBL + size +1);
				str += rtfReturn;
				strTemp.Format("\tOrd %u :\t%s ", ord, name);
				str += strTemp + rtfReturn;
				delete [] name;
			} else str += (CString) "none"+ rtfReturn;
		} else str += (CString) "error in export table"+ rtfReturn;
	} else str += (CString) "none"+ rtfReturn;
	str += rtfReturn;
	str += "EXPORT TABLE : (Non-Resident)\t";
	if (pLXHeader->e32_nrestab)
	{
		PBYTE PEXPTBL = MakePtr( PBYTE, pdosHeader, pLXHeader->e32_nrestab);
		if (pEXE->IsValidPtr((ULONG_PTR)PEXPTBL)) {
			BYTE size = *PEXPTBL; str += rtfReturn;
//			size &= 0x01111111;
			while (size)
			{
				char *name = new char[size+1];
				char *tpname = (char *) (PEXPTBL + 1);
				strncpy(name, tpname, size);
				name[size]='\0';
				WORD ord = * (WORD *)(PEXPTBL + size +1);
				strTemp.Format("\tOrd %u :\t%s ", ord, name);
				str += strTemp + rtfReturn;
				delete [] name;
				PEXPTBL += size + 3;
				size = *PEXPTBL;			
			}
		} else str += (CString) "error in export table (NR)"+ rtfReturn;
	}	else str += (CString) "none"+ rtfReturn;
	if (pLXHeader->e32_objtab)
	{ 
/*
           +-----+-----+-----+-----+-----+-----+-----+-----+
       00h |     VIRTUAL SIZE      |    RELOC BASE ADDR    |
           +-----+-----+-----+-----+-----+-----+-----+-----+
       08h |     OBJECT FLAGS      |    PAGE TABLE INDEX   |
           +-----+-----+-----+-----+-----+-----+-----+-----+
       10h |  # PAGE TABLE ENTRIES |       RESERVED        |
           +-----+-----+-----+-----+-----+-----+-----+-----+
*/
		str += rtfReturn;
			strTemp.Format("OBJECT TABLE : \t%d objects", pLXHeader->e32_objcnt);
			str += strTemp; 
			PLE_Object_Table PObjTbl = MakePtr( PLE_Object_Table, pLXHeader, pLXHeader->e32_objtab);
			if (pEXE->IsValidPtr((ULONG_PTR)PObjTbl)) {
				for (UINT i=0; i < pLXHeader->e32_objcnt; i++)
				{
					str += rtfReturn;
					strTemp.Format("%d:%s\tVirtual size :\t%08xh ", i, rtfReturn, PObjTbl->Virtual_Segment_Size);
					str += strTemp + rtfReturn;
					strTemp.Format("\tRelocation Address :\t%08xh ", PObjTbl->Relocation_Base_Address);
					str += strTemp + rtfReturn;
					strTemp.Format("\tPage Table Index :\t%lu ", PObjTbl->Page_MAP_Index);
					str += strTemp + rtfReturn;
					strTemp.Format("\tPage Table Count :\t%lu ", PObjTbl->Page_MAP_Entries);
					str += strTemp + rtfReturn;
					strTemp.Format("\tObject Flags :\t%08xh ", PObjTbl->FLAGS);
					str += strTemp + rtfReturn;
					if (PObjTbl->FLAGS & LEREAD) str += (CString) "\t\tReadable Object"+ rtfReturn; 
					if (PObjTbl->FLAGS & LEWRIT) str += (CString) "\t\tWritable Object"+ rtfReturn; 
					if (PObjTbl->FLAGS & LEEXEC) str += (CString) "\t\tExecutable Object"+ rtfReturn; 
					if (PObjTbl->FLAGS & LERESO) str += (CString) "\t\tResource Object"+ rtfReturn; 
					if (PObjTbl->FLAGS & LEDISC) str += (CString) "\t\tDiscardable Object"+ rtfReturn; 
					if (PObjTbl->FLAGS & LESHAR) str += (CString) "\t\tObject is Shared"+ rtfReturn; 
					if (PObjTbl->FLAGS & LEPREL) str += (CString) "\t\tObject has Preload Pages"+ rtfReturn; 
					if (PObjTbl->FLAGS & LEINVA) str += (CString) "\t\tObject has Invalid Pages"+ rtfReturn; 
					if ((PObjTbl->FLAGS & LETMSK) ==  LENORM) str += (CString) "\t\tObject is nonpermanent"+ rtfReturn;
					if ((PObjTbl->FLAGS & LETMSK) ==  LEZFIL) str += (CString) "\t\tObject has Zero Filled Pages"+ rtfReturn;
					if ((PObjTbl->FLAGS & LETMSK) ==  LERESI) str += (CString) "\t\tObject is Resident (valid for VDDs, PDDs only)"+ rtfReturn;
					if ((PObjTbl->FLAGS & LETMSK) ==  LERSCO) str += (CString) "\t\tObject is Resident & Contiguous (VDDs, PDDs only)"+ rtfReturn;
					if (PObjTbl->FLAGS & LERESI) str += (CString) "\t\tObject is Resident & 'long-lockable' (VDDs, PDDs only)"+ rtfReturn; 
					if (PObjTbl->FLAGS & LEALIA) str += (CString) "\t\t16:16 Alias Required (80x86 Specific)"+ rtfReturn; 
					if (PObjTbl->FLAGS & LEBIG) str += (CString) "\t\tBig/Default Bit Setting (80x86 Specific)"+ rtfReturn; 
					if (PObjTbl->FLAGS & LECONF) str += (CString) "\t\tObject is conforming for code (80x86 Specific)"+ rtfReturn;  //("\t\tConforming\r\n"); 
					if (PObjTbl->FLAGS & LEIOPL) str += (CString) "\t\tObject I/O privilege level (80x86 Specific)"+ rtfReturn;  // Only used for 16:16 Alias Objects
					PObjTbl++;
				}
			} else str += (CString) "error in object table"+ rtfReturn;
	}
/*
	if (pLXHeader->e32_objmap)
	{
		strTemp.Format("\r\nOBJECT PAGE TABLE : \t%d objects", pLXHeader->e32_mpages);
		str += strTemp; 
		PLE_Object_Page_Table PObjPT = MakePtr( PLE_Object_Page_Table, pLXHeader, pLXHeader->e32_objmap);
		for (int i=0; i < pLXHeader->e32_mpages-1; i++)
		{  
			strTemp.Format("\r\n%d:\r\n\tEXE offset :\t[%08xh] \r\n", i, pLXHeader->e32_datapage + pLXHeader->e32_pagesize * i);
			str += strTemp;
			strTemp.Format("\tsize :\t%xh \r\n", pLXHeader->e32_pagesize);	//PObjPT->size);
			str += strTemp;
//			strTemp.Format("\tflags :\t%08xh \r\n", PObjPT->flags);
//			str += strTemp;
/*
     FLAGS =  DW  Attributes  specifying  characteristics  of
     this logical page.
         The bit definitions for this word field follow,

             00h = Legal Physical Page  in the module (Offset
             from Preload Page Section).
             01h  = Iterated Data Page (Offset from  Iterated
             Data Pages Section).
             02h = Invalid Page (zero).
             03h = Zero Filled Page (zero).
             04h = Range of Pages.
*//*			
		}
		strTemp.Format("\r\n%d:\r\n\tEXE offset :\t[%08xh] \r\n", i, pLXHeader->e32_datapage + pLXHeader->e32_pagesize * i);
		str += strTemp;
		strTemp.Format("\tsize :\t%xh \r\n", pLXHeader->e32_lastpagesize);	//PObjPT->size);
		str += strTemp;
	}
*/
	str += rtfReturn;
	str += "RESOURCE TABLE :\t";
	if (pLXHeader->e32_rsrccnt)
	{
/*
           +-----+-----+-----+-----+
       00h |  TYPE ID  |  NAME ID  |
           +-----+-----+-----+-----+
       04h |     RESOURCE SIZE     |
           +-----+-----+-----+-----+-----+-----+
       08h |   OBJECT  |        OFFSET         |
           +-----+-----+-----+-----+-----+-----+
*/	
		PLE_ResourceTable_Entrie PResTblEnt = MakePtr( PLE_ResourceTable_Entrie, pLXHeader, pLXHeader->e32_rsrctab);
		if (pEXE->IsValidPtr((ULONG_PTR) PResTblEnt)) {
			for (UINT i=0; i < pLXHeader->e32_rsrccnt; i++)
			{
				strTemp.Format("%d objects", pLXHeader->e32_rsrccnt);
				str += strTemp + rtfReturn;
				strTemp.Format("\tName :\t%d ", PResTblEnt->name);
				str += strTemp + rtfReturn;
				strTemp.Format("\tType :\t%d ", PResTblEnt->type);
				str += strTemp + rtfReturn;
				strTemp.Format("\tObject :\t%d ", PResTblEnt->object);
				str += strTemp + rtfReturn;
				strTemp.Format("\tOffset :\t%d ", PResTblEnt->offset);
				str += strTemp + rtfReturn;
				strTemp.Format("\tSize :\t%d ", PResTblEnt->size);
				str += strTemp + rtfReturn;
				PResTblEnt++;
			}
		} else str += (CString) "error in resource table"+ rtfReturn;
	} else str += (CString) "none"+ rtfReturn;

	PEntry_Point ep = MakePtr( PEntry_Point, pLXHeader, pLXHeader->e32_enttab );
	if (pEXE->IsValidPtr((ULONG_PTR)ep))
	{
		str += rtfReturn;
		str += "ENTRY TABLE : ";
//		if (ep->flags & 2) str+= "\t32-bits entry\r\n";
//		else str+= "\t16-bits entry\r\n";

		switch(ep->type)
		{
			case 0 : str+= (CString) "\tunused entry"+ rtfReturn; break;
			case 1 : str+= (CString) "\t16-bits entry"+ rtfReturn; break;
			case 2 : str+= (CString) "\t286 Call Gate Entry"+ rtfReturn; break;
			case 3 : str+= (CString) "\t32-bits entry"+ rtfReturn; break;
			case 4 : str+= (CString) "\tForwarder Entry"+ rtfReturn; break;
			case 8 : str+= (CString) "\tParameter Typing Information Present"+ rtfReturn; break;
		}

		strTemp.Format("\tNumber of entries :\t%d ", ep->nb);
		str += strTemp + rtfReturn;
		strTemp.Format("\tFlags :\t0x%02Xh ",    ep->flags);
		str += strTemp + rtfReturn;
		strTemp.Format("\tObject :\t0x%04Xh ",    ep->object);
		str += strTemp + rtfReturn;
		if (ep->nb)
	    {
			if (ep->flags & 2)
			{
				strTemp.Format("\tOffset :\t0x%08Xh ",    ep->offset.o32);
	            str += strTemp + rtfReturn;
//		        PEntry_Data ed = MakePtr( PEntry_Data, pLXHeader, ep->offset.o32);
				PVxD_Desc_Block pVXDBlock = MakePtr( PVxD_Desc_Block, 0, ep->offset.o32);
			}
			else 
			{
				strTemp.Format("\tOffset :\t0x%04Xh ",    ep->offset.o16);
				str += strTemp + rtfReturn;
				PEntry_Data ed = MakePtr( PEntry_Data, pLXHeader, ep->offset.o16);
				PVxD_Desc_Block pVXDBlock = MakePtr( PVxD_Desc_Block, pLXHeader, ed->offset.o16);
			}
		}
   } else str += (CString) "error in entry table"+ rtfReturn;
   return str;
}

#pragma pack
