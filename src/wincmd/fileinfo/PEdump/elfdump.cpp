//==================================
// FILEINFO - ELF (Executable and Linkable Format)
// FILE: ELFDUMP.CPP
//==================================

#include "stdafx.h"
#include "elfdump.h"
#include "elfdump_relocs.h"
#include <stdint.h>
#include "elf.h"

//-----------------------------------
// Endian-aware readers
//-----------------------------------

static inline uint16_t elf_u16( const BYTE *p, bool le )
{
    return le ? (uint16_t)( p[0] | ((uint16_t)p[1] << 8) )
              : (uint16_t)( ((uint16_t)p[0] << 8) | p[1] );
}
static inline uint32_t elf_u32( const BYTE *p, bool le )
{
    return le ? ( (uint32_t)p[0]        | ((uint32_t)p[1] << 8)
                | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24) )
              : ( ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
                | ((uint32_t)p[2] << 8)  |  (uint32_t)p[3] );
}
static inline uint64_t elf_u64( const BYTE *p, bool le )
{
    return le ? (  (uint64_t)elf_u32(p,     true)
                | ((uint64_t)elf_u32(p + 4, true) << 32) )
              : ( ((uint64_t)elf_u32(p,     false) << 32)
                |  (uint64_t)elf_u32(p + 4, false) );
}

//-----------------------------------
// Parse context
//-----------------------------------

struct ElfContext
{
    const BYTE *base;
    ULONG_PTR   size;
    bool        is_64;        // ELFCLASS64
    bool        is_le;        // ELFDATA2LSB

    // Parsed Ehdr fields (address/offset fields always widened to uint64_t)
    uint16_t    e_type;
    uint16_t    e_machine;
    uint32_t    e_version;
    uint64_t    e_entry;
    uint64_t    e_phoff;
    uint64_t    e_shoff;
    uint32_t    e_flags;
    uint16_t    e_ehsize;
    uint16_t    e_phentsize;
    uint16_t    e_shentsize;

    // Raw (pre-resolution) 16-bit Ehdr count/index fields.
    uint16_t    raw_phnum;
    uint16_t    raw_shnum;
    uint16_t    raw_shstrndx;

    // Resolved count/index fields (after section-0 spill lookup).
    uint64_t    e_phnum;
    uint64_t    e_shnum;
    uint32_t    e_shstrndx;
};

//-----------------------------------
// Bounds + entry-size helpers
//
// The file-reported entsize / entry count are attacker-controlled. The Parse*
// helpers always read fixed-size structures regardless of entsize, so every
// iteration must guarantee (1) entsize is at least the real structure size
// and (2) offset + count*entsize fits in the mapped region without wrapping.
//-----------------------------------

static inline uint64_t MinShdrSize( bool is_64 ) { return is_64 ? 64u : 40u; }
static inline uint64_t MinPhdrSize( bool is_64 ) { return is_64 ? 56u : 32u; }
static inline uint64_t MinSymSize ( bool is_64 ) { return is_64 ? 24u : 16u; }
static inline uint64_t MinDynSize ( bool is_64 ) { return is_64 ? 16u :  8u; }
static inline uint64_t MinRelSize ( bool is_64 ) { return is_64 ? 16u :  8u; }
static inline uint64_t MinRelaSize( bool is_64 ) { return is_64 ? 24u : 12u; }

// Does [offset, offset + needed) fit entirely in [0, size)? Overflow-safe.
static inline bool RangeFits( uint64_t offset, uint64_t needed, uint64_t size )
{
    return offset <= size && needed <= size - offset;
}

// Do `count` entries of `entsize` bytes starting at `offset` fit in `size`?
// Uses division rather than multiplication to avoid 64-bit wrap on crafted
// counts like 0xFFFFFFFFFFFFFFFFull.
static inline bool TableFits( uint64_t offset, uint64_t count,
                              uint64_t entsize, uint64_t size )
{
    if ( offset > size ) return false;
    if ( entsize == 0 )  return count == 0;
    uint64_t avail = size - offset;
    return count <= avail / entsize;
}

//-----------------------------------
// Name tables
//-----------------------------------

typedef struct { uint32_t value; const char *name; } ValueName;

#define ENT(name)  { (uint32_t)(name), #name }

static const ValueName g_ElfClass[] = {
    ENT(ELFCLASSNONE), ENT(ELFCLASS32), ENT(ELFCLASS64),
};
static const ValueName g_ElfData[] = {
    ENT(ELFDATANONE), ENT(ELFDATA2LSB), ENT(ELFDATA2MSB),
};
static const ValueName g_ElfOSABI[] = {
    ENT(ELFOSABI_NONE),     ENT(ELFOSABI_HPUX),     ENT(ELFOSABI_NETBSD),
    ENT(ELFOSABI_GNU),      ENT(ELFOSABI_SOLARIS),  ENT(ELFOSABI_AIX),
    ENT(ELFOSABI_IRIX),     ENT(ELFOSABI_FREEBSD),  ENT(ELFOSABI_TRU64),
    ENT(ELFOSABI_MODESTO),  ENT(ELFOSABI_OPENBSD),  ENT(ELFOSABI_ARM_AEABI),
    ENT(ELFOSABI_ARM),      ENT(ELFOSABI_STANDALONE),
};
static const ValueName g_ElfType[] = {
    ENT(ET_NONE), ENT(ET_REL), ENT(ET_EXEC), ENT(ET_DYN), ENT(ET_CORE),
};
static const ValueName g_ElfMachine[] = {
    ENT(EM_NONE),     ENT(EM_M32),        ENT(EM_SPARC),        ENT(EM_386),
    ENT(EM_68K),      ENT(EM_88K),        ENT(EM_IAMCU),        ENT(EM_860),
    ENT(EM_MIPS),     ENT(EM_S370),       ENT(EM_MIPS_RS3_LE),  ENT(EM_PARISC),
    ENT(EM_VPP500),   ENT(EM_SPARC32PLUS),ENT(EM_960),          ENT(EM_PPC),
    ENT(EM_PPC64),    ENT(EM_S390),       ENT(EM_SPU),          ENT(EM_V800),
    ENT(EM_FR20),     ENT(EM_RH32),       ENT(EM_RCE),          ENT(EM_ARM),
    ENT(EM_FAKE_ALPHA), ENT(EM_SH),       ENT(EM_SPARCV9),      ENT(EM_TRICORE),
    ENT(EM_ARC),      ENT(EM_H8_300),     ENT(EM_H8_300H),      ENT(EM_H8S),
    ENT(EM_H8_500),   ENT(EM_IA_64),     ENT(EM_MIPS_X),        ENT(EM_COLDFIRE),
    ENT(EM_68HC12),   ENT(EM_MMA),        ENT(EM_PCP),          ENT(EM_NCPU),
    ENT(EM_NDR1),     ENT(EM_STARCORE),   ENT(EM_ME16),         ENT(EM_ST100),
    ENT(EM_TINYJ),    ENT(EM_X86_64),     ENT(EM_PDSP),         ENT(EM_PDP10),
    ENT(EM_PDP11),    ENT(EM_FX66),       ENT(EM_ST9PLUS),      ENT(EM_ST7),
    ENT(EM_68HC16),   ENT(EM_68HC11),     ENT(EM_68HC08),       ENT(EM_68HC05),
    ENT(EM_SVX),      ENT(EM_ST19),       ENT(EM_VAX),          ENT(EM_CRIS),
    ENT(EM_JAVELIN),  ENT(EM_FIREPATH),   ENT(EM_ZSP),          ENT(EM_MMIX),
    ENT(EM_HUANY),    ENT(EM_PRISM),      ENT(EM_AVR),          ENT(EM_FR30),
    ENT(EM_D10V),     ENT(EM_D30V),       ENT(EM_V850),         ENT(EM_M32R),
    ENT(EM_MN10300),  ENT(EM_MN10200),    ENT(EM_PJ),           ENT(EM_OPENRISC),
    ENT(EM_ARC_COMPACT), ENT(EM_XTENSA),  ENT(EM_VIDEOCORE),    ENT(EM_TMM_GPP),
    ENT(EM_NS32K),    ENT(EM_TPC),        ENT(EM_SNP1K),        ENT(EM_ST200),
    ENT(EM_IP2K),     ENT(EM_MAX),        ENT(EM_CR),           ENT(EM_F2MC16),
    ENT(EM_MSP430),   ENT(EM_BLACKFIN),   ENT(EM_SE_C33),       ENT(EM_SEP),
    ENT(EM_ARCA),     ENT(EM_UNICORE),    ENT(EM_EXCESS),       ENT(EM_DXP),
    ENT(EM_ALTERA_NIOS2), ENT(EM_CRX),    ENT(EM_XGATE),        ENT(EM_C166),
    ENT(EM_M16C),     ENT(EM_DSPIC30F),   ENT(EM_CE),           ENT(EM_M32C),
    ENT(EM_TSK3000),  ENT(EM_RS08),       ENT(EM_SHARC),        ENT(EM_ECOG2),
    ENT(EM_SCORE7),   ENT(EM_DSP24),      ENT(EM_VIDEOCORE3),   ENT(EM_LATTICEMICO32),
    ENT(EM_SE_C17),   ENT(EM_TI_C6000),   ENT(EM_TI_C2000),     ENT(EM_TI_C5500),
    ENT(EM_TI_ARP32), ENT(EM_TI_PRU),     ENT(EM_MMDSP_PLUS),   ENT(EM_CYPRESS_M8C),
    ENT(EM_R32C),     ENT(EM_TRIMEDIA),   ENT(EM_QDSP6),        ENT(EM_8051),
    ENT(EM_STXP7X),   ENT(EM_NDS32),      ENT(EM_ECOG1X),       ENT(EM_MAXQ30),
    ENT(EM_XIMO16),   ENT(EM_MANIK),      ENT(EM_CRAYNV2),      ENT(EM_RX),
    ENT(EM_METAG),    ENT(EM_MCST_ELBRUS),ENT(EM_ECOG16),       ENT(EM_CR16),
    ENT(EM_ETPU),     ENT(EM_SLE9X),      ENT(EM_L10M),         ENT(EM_K10M),
    ENT(EM_AARCH64),  ENT(EM_AVR32),      ENT(EM_STM8),         ENT(EM_TILE64),
    ENT(EM_TILEPRO),  ENT(EM_MICROBLAZE), ENT(EM_CUDA),         ENT(EM_TILEGX),
    ENT(EM_CLOUDSHIELD), ENT(EM_COREA_1ST), ENT(EM_COREA_2ND),  ENT(EM_ARCV2),
    ENT(EM_OPEN8),    ENT(EM_RL78),       ENT(EM_VIDEOCORE5),   ENT(EM_78KOR),
    ENT(EM_56800EX),  ENT(EM_BA1),        ENT(EM_BA2),          ENT(EM_XCORE),
    ENT(EM_MCHP_PIC), ENT(EM_INTELGT),    ENT(EM_KM32),         ENT(EM_KMX32),
    ENT(EM_EMX16),    ENT(EM_EMX8),       ENT(EM_KVARC),        ENT(EM_CDP),
    ENT(EM_COGE),     ENT(EM_COOL),       ENT(EM_NORC),         ENT(EM_CSR_KALIMBA),
    ENT(EM_Z80),      ENT(EM_VISIUM),     ENT(EM_FT32),         ENT(EM_MOXIE),
    ENT(EM_AMDGPU),   ENT(EM_RISCV),      ENT(EM_BPF),          ENT(EM_CSKY),
    ENT(EM_LOONGARCH),ENT(EM_ALPHA),
};

static const char *LookupName( const ValueName *table, size_t count, uint32_t value )
{
    for ( size_t i = 0; i < count; i++ )
        if ( table[i].value == value ) return table[i].name;
    return NULL;
}
#define LOOKUP(table, v) LookupName(table, sizeof(table) / sizeof((table)[0]), (v))

//-----------------------------------
// Ehdr parse and extended-numbering resolution
//-----------------------------------

static void ParseEhdr( ElfContext &ctx )
{
    const BYTE *b  = ctx.base;
    const bool  le = ctx.is_le;

    ctx.e_type    = elf_u16(b + 16, le);
    ctx.e_machine = elf_u16(b + 18, le);
    ctx.e_version = elf_u32(b + 20, le);

    if ( ctx.is_64 )
    {
        ctx.e_entry      = elf_u64(b + 24, le);
        ctx.e_phoff      = elf_u64(b + 32, le);
        ctx.e_shoff      = elf_u64(b + 40, le);
        ctx.e_flags      = elf_u32(b + 48, le);
        ctx.e_ehsize     = elf_u16(b + 52, le);
        ctx.e_phentsize  = elf_u16(b + 54, le);
        ctx.raw_phnum    = elf_u16(b + 56, le);
        ctx.e_shentsize  = elf_u16(b + 58, le);
        ctx.raw_shnum    = elf_u16(b + 60, le);
        ctx.raw_shstrndx = elf_u16(b + 62, le);
    }
    else
    {
        ctx.e_entry      = (uint64_t)elf_u32(b + 24, le);
        ctx.e_phoff      = (uint64_t)elf_u32(b + 28, le);
        ctx.e_shoff      = (uint64_t)elf_u32(b + 32, le);
        ctx.e_flags      = elf_u32(b + 36, le);
        ctx.e_ehsize     = elf_u16(b + 40, le);
        ctx.e_phentsize  = elf_u16(b + 42, le);
        ctx.raw_phnum    = elf_u16(b + 44, le);
        ctx.e_shentsize  = elf_u16(b + 46, le);
        ctx.raw_shnum    = elf_u16(b + 48, le);
        ctx.raw_shstrndx = elf_u16(b + 50, le);
    }

    ctx.e_phnum    = ctx.raw_phnum;
    ctx.e_shnum    = ctx.raw_shnum;
    ctx.e_shstrndx = ctx.raw_shstrndx;
}

// Per gABI: when a count/index field overflows its 16-bit Ehdr slot, the real
// value lives in a dedicated field of shdr[0] (normally all-zero SHT_NULL).
//   e_phnum == PN_XNUM         -> shdr[0].sh_info  (always 32-bit)
//   e_shnum == 0  (>=SHN_LORESERVE)-> shdr[0].sh_size (Word32 or Xword64)
//   e_shstrndx == SHN_XINDEX   -> shdr[0].sh_link  (always 32-bit)
static void ResolveExtendedNumbering( ElfContext &ctx )
{
    if ( ctx.e_shoff == 0 ) return;

    const uint64_t shdr_sz = MinShdrSize(ctx.is_64);
    if ( !RangeFits(ctx.e_shoff, shdr_sz, ctx.size) ) return;

    const BYTE *s0 = ctx.base + ctx.e_shoff;
    const bool  le = ctx.is_le;

    if ( ctx.raw_phnum == PN_XNUM )
    {
        // sh_info offset 44 (64-bit) / 28 (32-bit)
        ctx.e_phnum = ctx.is_64 ? elf_u32(s0 + 44, le) : elf_u32(s0 + 28, le);
    }
    if ( ctx.raw_shnum == 0 )
    {
        // sh_size offset 32 (64-bit, Xword) / 20 (32-bit, Word)
        ctx.e_shnum = ctx.is_64 ? elf_u64(s0 + 32, le) : (uint64_t)elf_u32(s0 + 20, le);
    }
    if ( ctx.raw_shstrndx == SHN_XINDEX )
    {
        // sh_link offset 40 (64-bit) / 24 (32-bit)
        ctx.e_shstrndx = ctx.is_64 ? elf_u32(s0 + 40, le) : elf_u32(s0 + 24, le);
    }
}

//-----------------------------------
// Dump helpers
//-----------------------------------

static CStringA FormatAddr( const ElfContext &ctx, uint64_t value )
{
    CStringA s;
    if ( ctx.is_64 )
        s.Format("0x%016llx", (unsigned long long)value);
    else
        s.Format("0x%08x", (uint32_t)value);
    return s;
}

//-----------------------------------
// Ehdr dumper
//-----------------------------------

static CStringA DumpEhdr( const ElfContext &ctx )
{
    CStringA str, line;
    const BYTE *b = ctx.base;

    const char *szClass   = LOOKUP(g_ElfClass,   b[EI_CLASS]);
    const char *szData    = LOOKUP(g_ElfData,    b[EI_DATA]);
    const char *szOSABI   = LOOKUP(g_ElfOSABI,   b[EI_OSABI]);
    const char *szType    = LOOKUP(g_ElfType,    ctx.e_type);
    const char *szMachine = LOOKUP(g_ElfMachine, ctx.e_machine);

    str += "ELF File Header:\r\n";

    line.Format("  Magic                 : %02x %02x %02x %02x (\"\\x7FELF\")\r\n",
                b[0], b[1], b[2], b[3]);
    str += line;

    line.Format("  Class                 : %s (%u)\r\n",
                szClass ? szClass : "?", b[EI_CLASS]);
    str += line;

    line.Format("  Data                  : %s (%u)\r\n",
                szData ? szData : "?", b[EI_DATA]);
    str += line;

    line.Format("  Version (ident)       : %u%s\r\n",
                b[EI_VERSION],
                (b[EI_VERSION] == EV_CURRENT) ? " (EV_CURRENT)" : "");
    str += line;

    line.Format("  OS/ABI                : %s (%u)\r\n",
                szOSABI ? szOSABI : "?", b[EI_OSABI]);
    str += line;

    line.Format("  ABI Version           : %u\r\n", b[EI_ABIVERSION]);
    str += line;

    line.Format("  Type                  : %s (0x%04x)\r\n",
                szType ? szType : "?", ctx.e_type);
    str += line;

    line.Format("  Machine               : %s (0x%04x)\r\n",
                szMachine ? szMachine : "?", ctx.e_machine);
    str += line;

    line.Format("  Version               : 0x%08x%s\r\n",
                ctx.e_version,
                (ctx.e_version == EV_CURRENT) ? " (EV_CURRENT)" : "");
    str += line;

    line.Format("  Entry point           : %s\r\n",
                (LPCSTR)FormatAddr(ctx, ctx.e_entry));
    str += line;

    line.Format("  Program header offset : %s (%llu bytes)\r\n",
                (LPCSTR)FormatAddr(ctx, ctx.e_phoff),
                (unsigned long long)ctx.e_phoff);
    str += line;

    line.Format("  Section header offset : %s (%llu bytes)\r\n",
                (LPCSTR)FormatAddr(ctx, ctx.e_shoff),
                (unsigned long long)ctx.e_shoff);
    str += line;

    line.Format("  Flags                 : 0x%08x\r\n", ctx.e_flags);
    str += line;

    line.Format("  Header size           : %u bytes\r\n", ctx.e_ehsize);
    str += line;

    line.Format("  PH entry size         : %u bytes\r\n", ctx.e_phentsize);
    str += line;

    if ( ctx.raw_phnum == PN_XNUM )
        line.Format("  PH count              : PN_XNUM (real: %llu from shdr[0].sh_info)\r\n",
                    (unsigned long long)ctx.e_phnum);
    else
        line.Format("  PH count              : %u\r\n", ctx.raw_phnum);
    str += line;

    line.Format("  SH entry size         : %u bytes\r\n", ctx.e_shentsize);
    str += line;

    if ( ctx.raw_shnum == 0 && ctx.e_shoff != 0 )
        line.Format("  SH count              : 0 (real: %llu from shdr[0].sh_size)\r\n",
                    (unsigned long long)ctx.e_shnum);
    else
        line.Format("  SH count              : %u\r\n", ctx.raw_shnum);
    str += line;

    if ( ctx.raw_shstrndx == SHN_XINDEX )
        line.Format("  SH string tbl index   : SHN_XINDEX (real: %u from shdr[0].sh_link)\r\n",
                    ctx.e_shstrndx);
    else
        line.Format("  SH string tbl index   : %u\r\n", ctx.raw_shstrndx);
    str += line;

    return str;
}

//-----------------------------------
// Shdr parsing + dump
//-----------------------------------

struct ShdrFields
{
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
};

static void ParseShdr( const ElfContext &ctx, uint64_t index, ShdrFields &s )
{
    const BYTE *p  = ctx.base + ctx.e_shoff + index * ctx.e_shentsize;
    const bool  le = ctx.is_le;

    s.sh_name = elf_u32(p + 0, le);
    s.sh_type = elf_u32(p + 4, le);

    if ( ctx.is_64 )
    {
        s.sh_flags     = elf_u64(p +  8, le);
        s.sh_addr      = elf_u64(p + 16, le);
        s.sh_offset    = elf_u64(p + 24, le);
        s.sh_size      = elf_u64(p + 32, le);
        s.sh_link      = elf_u32(p + 40, le);
        s.sh_info      = elf_u32(p + 44, le);
        s.sh_addralign = elf_u64(p + 48, le);
        s.sh_entsize   = elf_u64(p + 56, le);
    }
    else
    {
        s.sh_flags     = (uint64_t)elf_u32(p +  8, le);
        s.sh_addr      = (uint64_t)elf_u32(p + 12, le);
        s.sh_offset    = (uint64_t)elf_u32(p + 16, le);
        s.sh_size      = (uint64_t)elf_u32(p + 20, le);
        s.sh_link      = elf_u32(p + 24, le);
        s.sh_info      = elf_u32(p + 28, le);
        s.sh_addralign = (uint64_t)elf_u32(p + 32, le);
        s.sh_entsize   = (uint64_t)elf_u32(p + 36, le);
    }
}

static const char *GetString( const BYTE *strtab, uint64_t strtab_size, uint32_t offset )
{
    if ( !strtab || offset >= strtab_size ) return "";
    const BYTE *p   = strtab + offset;
    const BYTE *end = strtab + strtab_size;
    for ( const BYTE *q = p; q < end; q++ )
        if ( *q == 0 ) return (const char *)p;
    return "(unterminated)";
}

static const char *GetShTypeName( uint32_t type )
{
    switch ( type )
    {
    case SHT_NULL:           return "NULL";
    case SHT_PROGBITS:       return "PROGBITS";
    case SHT_SYMTAB:         return "SYMTAB";
    case SHT_STRTAB:         return "STRTAB";
    case SHT_RELA:           return "RELA";
    case SHT_HASH:           return "HASH";
    case SHT_DYNAMIC:        return "DYNAMIC";
    case SHT_NOTE:           return "NOTE";
    case SHT_NOBITS:         return "NOBITS";
    case SHT_REL:            return "REL";
    case SHT_SHLIB:          return "SHLIB";
    case SHT_DYNSYM:         return "DYNSYM";
    case SHT_INIT_ARRAY:     return "INIT_ARRAY";
    case SHT_FINI_ARRAY:     return "FINI_ARRAY";
    case SHT_PREINIT_ARRAY:  return "PREINIT_ARRAY";
    case SHT_GROUP:          return "GROUP";
    case SHT_SYMTAB_SHNDX:   return "SYMTAB_SHNDX";
    case SHT_RELR:           return "RELR";
    case SHT_GNU_HASH:       return "GNU_HASH";
    case SHT_GNU_verdef:     return "VERDEF";
    case SHT_GNU_verneed:    return "VERNEED";
    case SHT_GNU_versym:     return "VERSYM";
    case SHT_GNU_ATTRIBUTES: return "GNU_ATTRIB";
    case SHT_GNU_LIBLIST:    return "GNU_LIBLIST";
    case SHT_CHECKSUM:       return "CHECKSUM";
    default:
        if ( type >= SHT_LOOS   && type <= SHT_HIOS   ) return "LOOS..HIOS";
        if ( type >= SHT_LOPROC && type <= SHT_HIPROC ) return "LOPROC..HIPROC";
        if ( type >= SHT_LOUSER && type <= SHT_HIUSER ) return "LOUSER..HIUSER";
        return "?";
    }
}

static CStringA FormatShFlags( uint64_t flags )
{
    CStringA s;
    if ( flags & SHF_WRITE )            s += 'W';
    if ( flags & SHF_ALLOC )            s += 'A';
    if ( flags & SHF_EXECINSTR )        s += 'X';
    if ( flags & SHF_MERGE )            s += 'M';
    if ( flags & SHF_STRINGS )          s += 'S';
    if ( flags & SHF_INFO_LINK )        s += 'I';
    if ( flags & SHF_LINK_ORDER )       s += 'L';
    if ( flags & SHF_OS_NONCONFORMING ) s += 'O';
    if ( flags & SHF_GROUP )            s += 'G';
    if ( flags & SHF_TLS )              s += 'T';
    if ( flags & SHF_COMPRESSED )       s += 'C';
    return s;
}

static CStringA DumpShdrTable( const ElfContext &ctx )
{
    CStringA str, line;

    if ( ctx.e_shnum == 0 || ctx.e_shoff == 0 )
    {
        str += "\r\nSection Headers: (none)\r\n";
        return str;
    }

    // Belt-and-suspenders bounds check (DumpElfFile already validated these,
    // but keep the per-function guard so ParseShdr is safe in isolation).
    if ( ctx.e_shentsize < MinShdrSize(ctx.is_64)
         || !TableFits(ctx.e_shoff, ctx.e_shnum, ctx.e_shentsize, ctx.size) )
    {
        str += "\r\nSection Headers: (bad entsize or out of bounds)\r\n";
        return str;
    }

    // Resolve the section-name string table (.shstrtab).
    const BYTE *strtab      = NULL;
    uint64_t    strtab_size = 0;
    if ( ctx.e_shstrndx != 0 && ctx.e_shstrndx < ctx.e_shnum )
    {
        ShdrFields shstr;
        ParseShdr(ctx, ctx.e_shstrndx, shstr);
        if ( shstr.sh_type == SHT_STRTAB
             && RangeFits(shstr.sh_offset, shstr.sh_size, ctx.size) )
        {
            strtab      = ctx.base + shstr.sh_offset;
            strtab_size = shstr.sh_size;
        }
    }

    str += "\r\nSection Headers:\r\n";
    str += "  [Nr] Name                     Type          Flags        Address            Offset       Size          EntSize  Lk   Inf  Al\r\n";

    for ( uint64_t i = 0; i < ctx.e_shnum; i++ )
    {
        ShdrFields s;
        ParseShdr(ctx, i, s);

        const char *name  = GetString(strtab, strtab_size, s.sh_name);
        const char *type  = GetShTypeName(s.sh_type);
        CStringA    flags = FormatShFlags(s.sh_flags);

        line.Format("  [%3llu] %-24.24s %-13s %-12s %016llx   %010llx   %012llx  %06llx   %-4u %-4u %llu\r\n",
                    (unsigned long long)i,
                    (name && *name) ? name : "<null>",
                    type,
                    flags.IsEmpty() ? "" : (LPCSTR)flags,
                    (unsigned long long)s.sh_addr,
                    (unsigned long long)s.sh_offset,
                    (unsigned long long)s.sh_size,
                    (unsigned long long)s.sh_entsize,
                    s.sh_link,
                    s.sh_info,
                    (unsigned long long)s.sh_addralign);
        str += line;
    }

    str += "\r\nFlags: W=write  A=alloc  X=exec  M=merge  S=strings  I=info-link  L=link-order\r\n";
    str += "       O=os-nonconf  G=group  T=tls  C=compressed\r\n";

    return str;
}

//-----------------------------------
// Phdr parsing + dump
//-----------------------------------

struct PhdrFields
{
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
};

// Note: field order differs between classes.
//   Elf32_Phdr: type, offset, vaddr, paddr, filesz, memsz, flags, align
//   Elf64_Phdr: type, flags, offset, vaddr, paddr, filesz, memsz, align
static void ParsePhdr( const ElfContext &ctx, uint64_t index, PhdrFields &p )
{
    const BYTE *e  = ctx.base + ctx.e_phoff + index * ctx.e_phentsize;
    const bool  le = ctx.is_le;

    p.p_type = elf_u32(e + 0, le);

    if ( ctx.is_64 )
    {
        p.p_flags  = elf_u32(e +  4, le);
        p.p_offset = elf_u64(e +  8, le);
        p.p_vaddr  = elf_u64(e + 16, le);
        p.p_paddr  = elf_u64(e + 24, le);
        p.p_filesz = elf_u64(e + 32, le);
        p.p_memsz  = elf_u64(e + 40, le);
        p.p_align  = elf_u64(e + 48, le);
    }
    else
    {
        p.p_offset = (uint64_t)elf_u32(e +  4, le);
        p.p_vaddr  = (uint64_t)elf_u32(e +  8, le);
        p.p_paddr  = (uint64_t)elf_u32(e + 12, le);
        p.p_filesz = (uint64_t)elf_u32(e + 16, le);
        p.p_memsz  = (uint64_t)elf_u32(e + 20, le);
        p.p_flags  = elf_u32(e + 24, le);
        p.p_align  = (uint64_t)elf_u32(e + 28, le);
    }
}

static const char *GetPtTypeName( uint32_t type )
{
    switch ( type )
    {
    case PT_NULL:          return "NULL";
    case PT_LOAD:          return "LOAD";
    case PT_DYNAMIC:       return "DYNAMIC";
    case PT_INTERP:        return "INTERP";
    case PT_NOTE:          return "NOTE";
    case PT_SHLIB:         return "SHLIB";
    case PT_PHDR:          return "PHDR";
    case PT_TLS:           return "TLS";
    case PT_GNU_EH_FRAME:  return "GNU_EH_FRAME";
    case PT_GNU_STACK:     return "GNU_STACK";
    case PT_GNU_RELRO:     return "GNU_RELRO";
    case PT_GNU_PROPERTY:  return "GNU_PROPERTY";
    case PT_GNU_SFRAME:    return "GNU_SFRAME";
    case PT_SUNWBSS:       return "SUNWBSS";
    case PT_SUNWSTACK:     return "SUNWSTACK";
    default:
        if ( type >= PT_LOOS   && type <= PT_HIOS   ) return "LOOS..HIOS";
        if ( type >= PT_LOPROC && type <= PT_HIPROC ) return "LOPROC..HIPROC";
        return "?";
    }
}

static CStringA FormatPhdrFlags( uint32_t flags )
{
    CStringA s;
    s += (flags & PF_R) ? 'R' : ' ';
    s += (flags & PF_W) ? 'W' : ' ';
    s += (flags & PF_X) ? 'E' : ' ';
    return s;
}

static CStringA DumpPhdrTable( const ElfContext &ctx )
{
    CStringA str, line;

    if ( ctx.e_phnum == 0 || ctx.e_phoff == 0 )
    {
        str += "\r\nProgram Headers: (none)\r\n";
        return str;
    }

    if ( ctx.e_phentsize < MinPhdrSize(ctx.is_64)
         || !TableFits(ctx.e_phoff, ctx.e_phnum, ctx.e_phentsize, ctx.size) )
    {
        str += "\r\nProgram Headers: (bad entsize or out of bounds)\r\n";
        return str;
    }

    str += "\r\nProgram Headers:\r\n";
    str += "  [Nr] Type           Flg Offset         VirtAddr           PhysAddr           FileSize       MemSize        Align\r\n";

    for ( uint64_t i = 0; i < ctx.e_phnum; i++ )
    {
        PhdrFields p;
        ParsePhdr(ctx, i, p);

        const char *type = GetPtTypeName(p.p_type);
        CStringA    flg  = FormatPhdrFlags(p.p_flags);

        line.Format("  [%3llu] %-14s %-3s %014llx %018llx %018llx %014llx %014llx 0x%llx\r\n",
                    (unsigned long long)i,
                    type,
                    (LPCSTR)flg,
                    (unsigned long long)p.p_offset,
                    (unsigned long long)p.p_vaddr,
                    (unsigned long long)p.p_paddr,
                    (unsigned long long)p.p_filesz,
                    (unsigned long long)p.p_memsz,
                    (unsigned long long)p.p_align);
        str += line;
    }

    return str;
}

//-----------------------------------
// Symbol-table parsing + dump
//-----------------------------------

struct SymFields
{
    uint32_t st_name;
    uint8_t  st_info;
    uint8_t  st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;
};

static void ParseSym( const ElfContext &ctx, const BYTE *tabBase, uint64_t entsize,
                      uint64_t index, SymFields &s )
{
    const BYTE *e  = tabBase + index * entsize;
    const bool  le = ctx.is_le;

    s.st_name = elf_u32(e + 0, le);

    if ( ctx.is_64 )
    {
        s.st_info  = e[4];
        s.st_other = e[5];
        s.st_shndx = elf_u16(e +  6, le);
        s.st_value = elf_u64(e +  8, le);
        s.st_size  = elf_u64(e + 16, le);
    }
    else
    {
        s.st_value = (uint64_t)elf_u32(e +  4, le);
        s.st_size  = (uint64_t)elf_u32(e +  8, le);
        s.st_info  = e[12];
        s.st_other = e[13];
        s.st_shndx = elf_u16(e + 14, le);
    }
}

static const char *GetStBindName( uint8_t bind )
{
    switch ( bind )
    {
    case STB_LOCAL:      return "LOCAL";
    case STB_GLOBAL:     return "GLOBAL";
    case STB_WEAK:       return "WEAK";
    case STB_GNU_UNIQUE: return "UNIQUE";
    default:
        if ( bind >= STB_LOPROC && bind <= STB_HIPROC ) return "PROC";
        if ( bind >= STB_LOOS   && bind <= STB_HIOS   ) return "OS";
        return "?";
    }
}

static const char *GetStTypeName( uint8_t type )
{
    switch ( type )
    {
    case STT_NOTYPE:    return "NOTYPE";
    case STT_OBJECT:    return "OBJECT";
    case STT_FUNC:      return "FUNC";
    case STT_SECTION:   return "SECTION";
    case STT_FILE:      return "FILE";
    case STT_COMMON:    return "COMMON";
    case STT_TLS:       return "TLS";
    case STT_GNU_IFUNC: return "IFUNC";
    default:
        if ( type >= STT_LOPROC && type <= STT_HIPROC ) return "PROC";
        if ( type >= STT_LOOS   && type <= STT_HIOS   ) return "OS";
        return "?";
    }
}

static const char *GetStVisibilityName( uint8_t vis )
{
    switch ( vis )
    {
    case STV_DEFAULT:   return "DEFAULT";
    case STV_INTERNAL:  return "INTERNAL";
    case STV_HIDDEN:    return "HIDDEN";
    case STV_PROTECTED: return "PROTECTED";
    default:            return "?";
    }
}

static CStringA FormatShndx( uint32_t shndx )
{
    CStringA s;
    switch ( shndx )
    {
    case SHN_UNDEF:  s = "UND";     break;
    case SHN_ABS:    s = "ABS";     break;
    case SHN_COMMON: s = "COM";     break;
    case SHN_XINDEX: s = "XINDEX";  break;
    default:         s.Format("%u", shndx); break;
    }
    return s;
}

// Locate the SHT_SYMTAB_SHNDX section (if any) whose sh_link references the
// given symbol-table section index. Returns 0 if none found.
static uint64_t FindShndxTable( const ElfContext &ctx, uint64_t symtabIndex )
{
    for ( uint64_t i = 0; i < ctx.e_shnum; i++ )
    {
        ShdrFields s;
        ParseShdr(ctx, i, s);
        if ( s.sh_type == SHT_SYMTAB_SHNDX && s.sh_link == symtabIndex )
            return i;
    }
    return 0;
}

static CStringA DumpOneSymbolTable( const ElfContext &ctx, uint64_t secIndex,
                                    const ShdrFields &sec )
{
    CStringA str, line;

    // Name of this symtab section (for the header line).
    const BYTE *shstr      = NULL;
    uint64_t    shstr_size = 0;
    if ( ctx.e_shstrndx != 0 && ctx.e_shstrndx < ctx.e_shnum )
    {
        ShdrFields shstrSec;
        ParseShdr(ctx, ctx.e_shstrndx, shstrSec);
        if ( RangeFits(shstrSec.sh_offset, shstrSec.sh_size, ctx.size) )
        {
            shstr      = ctx.base + shstrSec.sh_offset;
            shstr_size = shstrSec.sh_size;
        }
    }
    const char *secName = GetString(shstr, shstr_size, sec.sh_name);

    if ( sec.sh_entsize < MinSymSize(ctx.is_64)
         || !RangeFits(sec.sh_offset, sec.sh_size, ctx.size) )
    {
        line.Format("\r\nSymbol Table '%s': (bad entsize or out-of-bounds)\r\n", secName);
        str += line;
        return str;
    }

    // Linked string table.
    const BYTE *strtab      = NULL;
    uint64_t    strtab_size = 0;
    if ( sec.sh_link != 0 && sec.sh_link < ctx.e_shnum )
    {
        ShdrFields strSec;
        ParseShdr(ctx, sec.sh_link, strSec);
        if ( strSec.sh_type == SHT_STRTAB
             && RangeFits(strSec.sh_offset, strSec.sh_size, ctx.size) )
        {
            strtab      = ctx.base + strSec.sh_offset;
            strtab_size = strSec.sh_size;
        }
    }

    // Extended section-index table (SHT_SYMTAB_SHNDX) that pairs with this symtab.
    const BYTE *shndxTable      = NULL;
    uint64_t    shndxTable_size = 0;
    uint64_t    shndxIdx        = FindShndxTable(ctx, secIndex);
    if ( shndxIdx != 0 )
    {
        ShdrFields shndxSec;
        ParseShdr(ctx, shndxIdx, shndxSec);
        if ( RangeFits(shndxSec.sh_offset, shndxSec.sh_size, ctx.size) )
        {
            shndxTable      = ctx.base + shndxSec.sh_offset;
            shndxTable_size = shndxSec.sh_size;
        }
    }

    uint64_t nSyms = sec.sh_size / sec.sh_entsize;
    const BYTE *tabBase = ctx.base + sec.sh_offset;

    line.Format("\r\nSymbol Table '%s' (section %llu): %llu entries\r\n",
                secName, (unsigned long long)secIndex, (unsigned long long)nSyms);
    str += line;
    str += "   [Nr]  Value              Size         Type     Bind     Vis        Ndx     Name\r\n";

    for ( uint64_t i = 0; i < nSyms; i++ )
    {
        SymFields sym;
        ParseSym(ctx, tabBase, sec.sh_entsize, i, sym);

        uint8_t  bind    = (uint8_t)(sym.st_info >> 4);
        uint8_t  type    = (uint8_t)(sym.st_info & 0xF);
        uint8_t  vis     = (uint8_t)(sym.st_other & 0x3);

        // Resolve SHN_XINDEX via the parallel SYMTAB_SHNDX table.
        uint32_t shndx   = sym.st_shndx;
        if ( shndx == SHN_XINDEX && shndxTable != NULL
             && i * 4 + 4 <= shndxTable_size )
        {
            shndx = elf_u32(shndxTable + i * 4, ctx.is_le);
        }

        const char *szType = GetStTypeName(type);
        const char *szBind = GetStBindName(bind);
        const char *szVis  = GetStVisibilityName(vis);
        CStringA    szNdx  = FormatShndx(shndx);
        const char *name   = GetString(strtab, strtab_size, sym.st_name);

        line.Format("   [%4llu] %016llx   %012llx %-8s %-8s %-10s %-7s %s\r\n",
                    (unsigned long long)i,
                    (unsigned long long)sym.st_value,
                    (unsigned long long)sym.st_size,
                    szType,
                    szBind,
                    szVis,
                    (LPCSTR)szNdx,
                    name ? name : "");
        str += line;
    }

    return str;
}

static CStringA DumpSymbolTables( const ElfContext &ctx )
{
    CStringA str;

    if ( ctx.e_shnum == 0 || ctx.e_shoff == 0 ) return str;

    for ( uint64_t i = 0; i < ctx.e_shnum; i++ )
    {
        ShdrFields sec;
        ParseShdr(ctx, i, sec);
        if ( sec.sh_type == SHT_SYMTAB || sec.sh_type == SHT_DYNSYM )
            str += DumpOneSymbolTable(ctx, i, sec);
    }

    return str;
}

//-----------------------------------
// Dynamic section dump
//-----------------------------------

static const char *GetDtTagName( uint64_t tag )
{
    switch ( tag )
    {
    case DT_NULL:            return "NULL";
    case DT_NEEDED:          return "NEEDED";
    case DT_PLTRELSZ:        return "PLTRELSZ";
    case DT_PLTGOT:          return "PLTGOT";
    case DT_HASH:            return "HASH";
    case DT_STRTAB:          return "STRTAB";
    case DT_SYMTAB:          return "SYMTAB";
    case DT_RELA:            return "RELA";
    case DT_RELASZ:          return "RELASZ";
    case DT_RELAENT:         return "RELAENT";
    case DT_STRSZ:           return "STRSZ";
    case DT_SYMENT:          return "SYMENT";
    case DT_INIT:            return "INIT";
    case DT_FINI:            return "FINI";
    case DT_SONAME:          return "SONAME";
    case DT_RPATH:           return "RPATH";
    case DT_SYMBOLIC:        return "SYMBOLIC";
    case DT_REL:             return "REL";
    case DT_RELSZ:           return "RELSZ";
    case DT_RELENT:          return "RELENT";
    case DT_PLTREL:          return "PLTREL";
    case DT_DEBUG:           return "DEBUG";
    case DT_TEXTREL:         return "TEXTREL";
    case DT_JMPREL:          return "JMPREL";
    case DT_BIND_NOW:        return "BIND_NOW";
    case DT_INIT_ARRAY:      return "INIT_ARRAY";
    case DT_FINI_ARRAY:      return "FINI_ARRAY";
    case DT_INIT_ARRAYSZ:    return "INIT_ARRAYSZ";
    case DT_FINI_ARRAYSZ:    return "FINI_ARRAYSZ";
    case DT_RUNPATH:         return "RUNPATH";
    case DT_FLAGS:           return "FLAGS";
    case DT_PREINIT_ARRAY:   return "PREINIT_ARRAY";
    case DT_PREINIT_ARRAYSZ: return "PREINIT_ARRAYSZ";
    case DT_SYMTAB_SHNDX:    return "SYMTAB_SHNDX";
    case DT_RELRSZ:          return "RELRSZ";
    case DT_RELR:            return "RELR";
    case DT_RELRENT:         return "RELRENT";
    case DT_GNU_HASH:        return "GNU_HASH";
    case DT_TLSDESC_PLT:     return "TLSDESC_PLT";
    case DT_TLSDESC_GOT:     return "TLSDESC_GOT";
    case DT_GNU_CONFLICT:    return "GNU_CONFLICT";
    case DT_GNU_LIBLIST:     return "GNU_LIBLIST";
    case DT_CONFIG:          return "CONFIG";
    case DT_DEPAUDIT:        return "DEPAUDIT";
    case DT_AUDIT:           return "AUDIT";
    case DT_SYMINFO:         return "SYMINFO";
    case DT_VERSYM:          return "VERSYM";
    case DT_RELACOUNT:       return "RELACOUNT";
    case DT_RELCOUNT:        return "RELCOUNT";
    case DT_FLAGS_1:         return "FLAGS_1";
    case DT_VERDEF:          return "VERDEF";
    case DT_VERDEFNUM:       return "VERDEFNUM";
    case DT_VERNEED:         return "VERNEED";
    case DT_VERNEEDNUM:      return "VERNEEDNUM";
    case DT_AUXILIARY:       return "AUXILIARY";
    case DT_FILTER:          return "FILTER";
    case DT_GNU_PRELINKED:   return "GNU_PRELINKED";
    case DT_CHECKSUM:        return "CHECKSUM";
    case DT_MOVEENT:         return "MOVEENT";
    case DT_MOVESZ:          return "MOVESZ";
    case DT_FEATURE_1:       return "FEATURE_1";
    case DT_POSFLAG_1:       return "POSFLAG_1";
    case DT_SYMINSZ:         return "SYMINSZ";
    case DT_SYMINENT:        return "SYMINENT";
    default:                 return NULL;
    }
}

static bool IsDtStringValued( uint64_t tag )
{
    return tag == DT_NEEDED || tag == DT_SONAME
        || tag == DT_RPATH  || tag == DT_RUNPATH
        || tag == DT_CONFIG || tag == DT_AUDIT || tag == DT_DEPAUDIT;
}

// DT_FLAGS bits
static CStringA FormatDtFlags( uint64_t v )
{
    CStringA s;
    const struct { uint64_t bit; const char *name; } tbl[] = {
        { DF_ORIGIN,     "ORIGIN"     },
        { DF_SYMBOLIC,   "SYMBOLIC"   },
        { DF_TEXTREL,    "TEXTREL"    },
        { DF_BIND_NOW,   "BIND_NOW"   },
        { DF_STATIC_TLS, "STATIC_TLS" },
    };
    for ( size_t i = 0; i < sizeof(tbl)/sizeof(tbl[0]); i++ )
    {
        if ( v & tbl[i].bit )
        {
            if ( !s.IsEmpty() ) s += " | ";
            s += tbl[i].name;
        }
    }
    return s;
}

// DT_FLAGS_1 bits
static CStringA FormatDtFlags1( uint64_t v )
{
    CStringA s;
    const struct { uint64_t bit; const char *name; } tbl[] = {
        { DF_1_NOW,         "NOW"         },
        { DF_1_GLOBAL,      "GLOBAL"      },
        { DF_1_GROUP,       "GROUP"       },
        { DF_1_NODELETE,    "NODELETE"    },
        { DF_1_LOADFLTR,    "LOADFLTR"    },
        { DF_1_INITFIRST,   "INITFIRST"   },
        { DF_1_NOOPEN,      "NOOPEN"      },
        { DF_1_ORIGIN,      "ORIGIN"      },
        { DF_1_DIRECT,      "DIRECT"      },
        { DF_1_TRANS,       "TRANS"       },
        { DF_1_INTERPOSE,   "INTERPOSE"   },
        { DF_1_NODEFLIB,    "NODEFLIB"    },
        { DF_1_NODUMP,      "NODUMP"      },
        { DF_1_CONFALT,     "CONFALT"     },
        { DF_1_ENDFILTEE,   "ENDFILTEE"   },
        { 0x00008000ULL,    "DISPRELDNE"  },
        { 0x00010000ULL,    "DISPRELPND"  },
        { 0x00020000ULL,    "NODIRECT"    },
        { 0x00040000ULL,    "IGNMULDEF"   },
        { 0x00080000ULL,    "NOKSYMS"     },
        { 0x00100000ULL,    "NOHDR"       },
        { 0x00200000ULL,    "EDITED"      },
        { 0x00400000ULL,    "NORELOC"     },
        { 0x00800000ULL,    "SYMINTPOSE"  },
        { 0x01000000ULL,    "GLOBAUDIT"   },
        { 0x02000000ULL,    "SINGLETON"   },
        { 0x04000000ULL,    "STUB"        },
        { 0x08000000ULL,    "PIE"         },
    };
    for ( size_t i = 0; i < sizeof(tbl)/sizeof(tbl[0]); i++ )
    {
        if ( v & tbl[i].bit )
        {
            if ( !s.IsEmpty() ) s += " | ";
            s += tbl[i].name;
        }
    }
    return s;
}

static CStringA DumpDynamicSection( const ElfContext &ctx )
{
    CStringA str, line;

    if ( ctx.e_shnum == 0 || ctx.e_shoff == 0 ) return str;

    // Find .dynamic section.
    ShdrFields dynSec;
    uint64_t dynIdx = 0;
    bool found = false;
    for ( uint64_t i = 0; i < ctx.e_shnum; i++ )
    {
        ShdrFields s;
        ParseShdr(ctx, i, s);
        if ( s.sh_type == SHT_DYNAMIC )
        {
            dynSec = s;
            dynIdx = i;
            found  = true;
            break;
        }
    }
    if ( !found ) return str;

    uint64_t entsize = MinDynSize(ctx.is_64);
    if ( dynSec.sh_entsize != 0 ) entsize = dynSec.sh_entsize;

    if ( entsize < MinDynSize(ctx.is_64)
         || !RangeFits(dynSec.sh_offset, dynSec.sh_size, ctx.size) )
    {
        str += "\r\nDynamic Section: (bad entsize or out-of-bounds)\r\n";
        return str;
    }

    // String table for .dynamic (sh_link points to .dynstr).
    const BYTE *strtab      = NULL;
    uint64_t    strtab_size = 0;
    if ( dynSec.sh_link != 0 && dynSec.sh_link < ctx.e_shnum )
    {
        ShdrFields strSec;
        ParseShdr(ctx, dynSec.sh_link, strSec);
        if ( strSec.sh_type == SHT_STRTAB
             && RangeFits(strSec.sh_offset, strSec.sh_size, ctx.size) )
        {
            strtab      = ctx.base + strSec.sh_offset;
            strtab_size = strSec.sh_size;
        }
    }

    uint64_t nEntries = dynSec.sh_size / entsize;

    line.Format("\r\nDynamic Section (section %llu): %llu entries\r\n",
                (unsigned long long)dynIdx, (unsigned long long)nEntries);
    str += line;
    str += "   [Nr]  Tag                  Name/Value\r\n";

    const BYTE *entries = ctx.base + dynSec.sh_offset;
    const bool  le      = ctx.is_le;

    for ( uint64_t i = 0; i < nEntries; i++ )
    {
        const BYTE *e = entries + i * entsize;
        uint64_t d_tag, d_val;
        if ( ctx.is_64 )
        {
            d_tag = elf_u64(e + 0, le);
            d_val = elf_u64(e + 8, le);
        }
        else
        {
            d_tag = (uint64_t)elf_u32(e + 0, le);
            d_val = (uint64_t)elf_u32(e + 4, le);
        }

        const char *tagName = GetDtTagName(d_tag);
        CStringA    tagStr;
        if ( tagName )
            tagStr.Format("DT_%s", tagName);
        else
            tagStr.Format("0x%llx", (unsigned long long)d_tag);

        CStringA rhs;
        if ( IsDtStringValued(d_tag) )
        {
            const char *sval = GetString(strtab, strtab_size, (uint32_t)d_val);
            rhs.Format("%s", sval ? sval : "");
        }
        else if ( d_tag == DT_FLAGS )
        {
            CStringA flags = FormatDtFlags(d_val);
            rhs.Format("0x%llx (%s)", (unsigned long long)d_val,
                       flags.IsEmpty() ? "none" : (LPCSTR)flags);
        }
        else if ( d_tag == DT_FLAGS_1 )
        {
            CStringA flags = FormatDtFlags1(d_val);
            rhs.Format("0x%llx (%s)", (unsigned long long)d_val,
                       flags.IsEmpty() ? "none" : (LPCSTR)flags);
        }
        else
        {
            rhs.Format("0x%llx (%llu)",
                       (unsigned long long)d_val,
                       (unsigned long long)d_val);
        }

        line.Format("   [%4llu] %-20s %s\r\n",
                    (unsigned long long)i, (LPCSTR)tagStr, (LPCSTR)rhs);
        str += line;

        if ( d_tag == DT_NULL ) break;
    }

    return str;
}

//-----------------------------------
// Relocation section dump
//-----------------------------------

// Extract symbol index / type from r_info.
//
//   ELF32 (all arches):   sym = info >> 8,    type = info & 0xff
//   ELF64 (standard):     sym = info >> 32,   type = info & 0xffffffff
//   ELF64 MIPS (quirky):  r_info is a 5-field STRUCT rather than a packed 64-bit
//                         integer. The on-disk byte layout is:
//                           bytes 0-3 : r_sym  (32-bit, file-endian)
//                           byte  4   : r_ssym
//                           byte  5   : r_type3
//                           byte  6   : r_type2
//                           byte  7   : r_type  (primary type)
//                         Reading r_info as a native uint64 therefore puts
//                         r_sym in the low 32 bits on LE hosts and the high
//                         32 bits on BE hosts — the standard sym>>32 / type&0xff
//                         formulas are wrong in both cases. We bypass it by
//                         reading the fields directly from the raw bytes.
static inline bool IsMips64( const ElfContext &ctx )
{
    return ctx.is_64
        && ( ctx.e_machine == EM_MIPS || ctx.e_machine == EM_MIPS_RS3_LE );
}

static inline uint32_t RelSym( const ElfContext &ctx, uint64_t info,
                               const BYTE *raw_info )
{
    if ( !ctx.is_64 )    return (uint32_t)(info >> 8);
    if ( IsMips64(ctx) ) return elf_u32(raw_info, ctx.is_le);
    return (uint32_t)(info >> 32);
}
static inline uint32_t RelType( const ElfContext &ctx, uint64_t info,
                                const BYTE *raw_info )
{
    if ( !ctx.is_64 )    return (uint32_t)(info & 0xFFu);
    if ( IsMips64(ctx) ) return (uint32_t)raw_info[7];  // primary type = byte 7
    return (uint32_t)(info & 0xFFFFFFFFull);
}

// Load symbol N out of a symbol-table section. Returns true on success.
static bool GetSymByIndex( const ElfContext &ctx, const ShdrFields &symSec,
                           uint32_t symIdx, SymFields &out )
{
    // ParseSym always reads a full symbol record (16 / 24 bytes); any caller
    // that hands us a symSec with a smaller entsize would trigger an OOB read.
    if ( symSec.sh_entsize < MinSymSize(ctx.is_64) ) return false;
    if ( !RangeFits(symSec.sh_offset, symSec.sh_size, ctx.size) ) return false;
    uint64_t nSyms = symSec.sh_size / symSec.sh_entsize;
    if ( symIdx >= nSyms ) return false;
    const BYTE *tabBase = ctx.base + symSec.sh_offset;
    ParseSym(ctx, tabBase, symSec.sh_entsize, symIdx, out);
    return true;
}

static CStringA DumpOneRelocSection( const ElfContext &ctx, uint64_t secIndex,
                                     const ShdrFields &sec )
{
    CStringA str, line;
    const bool isRela = (sec.sh_type == SHT_RELA);

    // Name.
    const BYTE *shstr      = NULL;
    uint64_t    shstr_size = 0;
    if ( ctx.e_shstrndx != 0 && ctx.e_shstrndx < ctx.e_shnum )
    {
        ShdrFields shstrSec;
        ParseShdr(ctx, ctx.e_shstrndx, shstrSec);
        if ( RangeFits(shstrSec.sh_offset, shstrSec.sh_size, ctx.size) )
        {
            shstr      = ctx.base + shstrSec.sh_offset;
            shstr_size = shstrSec.sh_size;
        }
    }
    const char *secName = GetString(shstr, shstr_size, sec.sh_name);

    const uint64_t minEnt = isRela ? MinRelaSize(ctx.is_64)
                                   : MinRelSize (ctx.is_64);
    uint64_t entsize = sec.sh_entsize != 0 ? sec.sh_entsize : minEnt;

    if ( entsize < minEnt
         || !RangeFits(sec.sh_offset, sec.sh_size, ctx.size) )
    {
        line.Format("\r\nRelocation section '%s': (bad entsize or out-of-bounds)\r\n", secName);
        str += line;
        return str;
    }

    // Linked symbol table -> and its string table. GetSymByIndex() will
    // dereference full Sym structures, so reject the linked symtab if its
    // entsize is too small — otherwise a crafted reloc could fabricate a
    // symtab reference that triggers OOB in ParseSym.
    bool haveSyms = false;
    ShdrFields symSec;
    const BYTE *strtab      = NULL;
    uint64_t    strtab_size = 0;
    if ( sec.sh_link != 0 && sec.sh_link < ctx.e_shnum )
    {
        ParseShdr(ctx, sec.sh_link, symSec);
        if ( (symSec.sh_type == SHT_SYMTAB || symSec.sh_type == SHT_DYNSYM)
             && symSec.sh_entsize >= MinSymSize(ctx.is_64)
             && RangeFits(symSec.sh_offset, symSec.sh_size, ctx.size) )
        {
            haveSyms = true;
            if ( symSec.sh_link != 0 && symSec.sh_link < ctx.e_shnum )
            {
                ShdrFields strSec;
                ParseShdr(ctx, symSec.sh_link, strSec);
                if ( strSec.sh_type == SHT_STRTAB
                     && RangeFits(strSec.sh_offset, strSec.sh_size, ctx.size) )
                {
                    strtab      = ctx.base + strSec.sh_offset;
                    strtab_size = strSec.sh_size;
                }
            }
        }
    }

    uint64_t nEntries = sec.sh_size / entsize;
    line.Format("\r\nRelocation section '%s' (section %llu): %llu entries\r\n",
                secName, (unsigned long long)secIndex, (unsigned long long)nEntries);
    str += line;

    if ( isRela )
        str += "    Offset             Info               Type                          SymValue          SymName + Addend\r\n";
    else
        str += "    Offset             Info               Type                          SymValue          SymName\r\n";

    const BYTE *entries = ctx.base + sec.sh_offset;
    const bool  le      = ctx.is_le;

    for ( uint64_t i = 0; i < nEntries; i++ )
    {
        const BYTE *e = entries + i * entsize;
        uint64_t r_offset, r_info;
        int64_t  r_addend = 0;
        const BYTE *r_info_raw = NULL;  // MIPS64 needs byte-level access.

        if ( ctx.is_64 )
        {
            r_offset   = elf_u64(e + 0, le);
            r_info     = elf_u64(e + 8, le);
            r_info_raw = e + 8;
            if ( isRela ) r_addend = (int64_t)elf_u64(e + 16, le);
        }
        else
        {
            r_offset   = (uint64_t)elf_u32(e + 0, le);
            r_info     = (uint64_t)elf_u32(e + 4, le);
            r_info_raw = e + 4;
            if ( isRela ) r_addend = (int32_t)elf_u32(e + 8, le);
        }

        uint32_t symIdx  = RelSym (ctx, r_info, r_info_raw);
        uint32_t relType = RelType(ctx, r_info, r_info_raw);

        const char *typeName = GetElfRelocationTypeName(ctx.e_machine, relType);
        CStringA    typeStr;
        if ( typeName )
            typeStr = typeName;
        else
            typeStr.Format("0x%x", relType);

        CStringA    symName;
        uint64_t    symValue = 0;
        // Symbol index 0 is the undefined symbol — relocations that reference
        // it (typically R_*_RELATIVE) carry no symbolic meaning; show nothing
        // instead of "<noname>".
        if ( haveSyms && symIdx != 0 )
        {
            SymFields sym;
            if ( GetSymByIndex(ctx, symSec, symIdx, sym) )
            {
                symValue = sym.st_value;
                const char *nm = GetString(strtab, strtab_size, sym.st_name);
                if ( nm && *nm )
                    symName = nm;
                else if ( (sym.st_info & 0xF) == STT_SECTION )
                    symName.Format("<section %u>", sym.st_shndx);
                else
                    symName = "<noname>";
            }
        }

        if ( isRela )
        {
            line.Format("    %016llx   %016llx   %-29s %016llx  %s %c %llx\r\n",
                        (unsigned long long)r_offset,
                        (unsigned long long)r_info,
                        (LPCSTR)typeStr,
                        (unsigned long long)symValue,
                        symName.IsEmpty() ? "" : (LPCSTR)symName,
                        (r_addend >= 0) ? '+' : '-',
                        (unsigned long long)(r_addend >= 0 ? r_addend : -r_addend));
        }
        else
        {
            line.Format("    %016llx   %016llx   %-29s %016llx  %s\r\n",
                        (unsigned long long)r_offset,
                        (unsigned long long)r_info,
                        (LPCSTR)typeStr,
                        (unsigned long long)symValue,
                        symName.IsEmpty() ? "" : (LPCSTR)symName);
        }
        str += line;
    }

    return str;
}

static CStringA DumpRelocationSections( const ElfContext &ctx )
{
    CStringA str;

    if ( ctx.e_shnum == 0 || ctx.e_shoff == 0 ) return str;

    for ( uint64_t i = 0; i < ctx.e_shnum; i++ )
    {
        ShdrFields s;
        ParseShdr(ctx, i, s);
        if ( s.sh_type == SHT_REL || s.sh_type == SHT_RELA )
            str += DumpOneRelocSection(ctx, i, s);
    }

    return str;
}

//-----------------------------------
// Note section dump
//-----------------------------------

// Round up to 4-byte boundary. Per ELF gABI, note name and desc fields are
// each padded to 4 bytes regardless of ELFCLASS (the de-facto behavior on
// Linux; some historical readers pad to 8 on ELF64, but mainstream tooling
// treats notes as 4-byte-padded everywhere).
static inline uint64_t RoundUp4( uint64_t v ) { return (v + 3) & ~3ULL; }

static const char *GetGnuNoteTypeName( uint32_t type )
{
    switch ( type )
    {
    case NT_GNU_ABI_TAG:         return "NT_GNU_ABI_TAG";
    case NT_GNU_HWCAP:           return "NT_GNU_HWCAP";
    case NT_GNU_BUILD_ID:        return "NT_GNU_BUILD_ID";
    case NT_GNU_GOLD_VERSION:    return "NT_GNU_GOLD_VERSION";
    case NT_GNU_PROPERTY_TYPE_0: return "NT_GNU_PROPERTY_TYPE_0";
    default:                     return NULL;
    }
}

static CStringA DumpGnuAbiTag( const BYTE *desc, uint64_t descsz, bool le )
{
    CStringA s;
    if ( descsz < 16 ) { s.Format("<malformed, %llu bytes>", (unsigned long long)descsz); return s; }
    uint32_t os    = elf_u32(desc + 0,  le);
    uint32_t major = elf_u32(desc + 4,  le);
    uint32_t minor = elf_u32(desc + 8,  le);
    uint32_t patch = elf_u32(desc + 12, le);
    const char *osName =
        os == 0 ? "Linux" :
        os == 1 ? "Hurd"  :
        os == 2 ? "Solaris" :
        os == 3 ? "FreeBSD"  :
        os == 4 ? "NetBSD"   : "?";
    s.Format("OS=%s (%u), ABI=%u.%u.%u", osName, os, major, minor, patch);
    return s;
}

static CStringA DumpBuildId( const BYTE *desc, uint64_t descsz )
{
    CStringA s;
    for ( uint64_t i = 0; i < descsz; i++ )
    {
        CStringA t; t.Format("%02x", desc[i]);
        s += t;
    }
    return s;
}

static const char *GetGnuPropertyName( uint32_t pr_type, uint32_t machine )
{
    // Machine-specific properties (0xc0000000+).
    if ( machine == EM_AARCH64 )
    {
        switch ( pr_type )
        {
        case GNU_PROPERTY_AARCH64_FEATURE_1_AND: return "AARCH64_FEATURE_1_AND";
        default: break;
        }
    }
    if ( machine == EM_X86_64 || machine == EM_386 || machine == EM_IAMCU )
    {
        switch ( pr_type )
        {
        case GNU_PROPERTY_X86_FEATURE_1_AND:    return "X86_FEATURE_1_AND";
        case GNU_PROPERTY_X86_ISA_1_NEEDED:     return "X86_ISA_1_NEEDED";
        case GNU_PROPERTY_X86_ISA_1_USED:       return "X86_ISA_1_USED";
        default: break;
        }
    }
    switch ( pr_type )
    {
    case GNU_PROPERTY_STACK_SIZE:          return "STACK_SIZE";
    case GNU_PROPERTY_NO_COPY_ON_PROTECTED: return "NO_COPY_ON_PROTECTED";
    case GNU_PROPERTY_1_NEEDED:            return "1_NEEDED";
    default:                               return NULL;
    }
}

static CStringA DumpGnuProperty( const BYTE *desc, uint64_t descsz, bool le,
                                 uint32_t machine )
{
    CStringA str, line;
    uint64_t off = 0;
    while ( off + 8 <= descsz )
    {
        uint32_t pr_type   = elf_u32(desc + off + 0, le);
        uint32_t pr_datasz = elf_u32(desc + off + 4, le);
        if ( off + 8 + pr_datasz > descsz ) break;

        const char *nm = GetGnuPropertyName(pr_type, machine);
        if ( nm )
            line.Format("      [%s (0x%x), size=%u] ", nm, pr_type, pr_datasz);
        else
            line.Format("      [type=0x%x, size=%u] ", pr_type, pr_datasz);
        str += line;

        // Print raw bytes of data.
        const BYTE *d = desc + off + 8;
        for ( uint32_t k = 0; k < pr_datasz; k++ )
        {
            line.Format("%02x ", d[k]);
            str += line;
        }
        str += "\r\n";

        // Each property is padded to 8 bytes when descsz is 8-aligned.
        uint64_t step = 8 + pr_datasz;
        step = (step + 7) & ~7ULL;
        off += step;
    }
    return str;
}

static CStringA DumpOneNoteSection( const ElfContext &ctx, uint64_t secIndex,
                                    const ShdrFields &sec )
{
    CStringA str, line;

    const BYTE *shstr      = NULL;
    uint64_t    shstr_size = 0;
    if ( ctx.e_shstrndx != 0 && ctx.e_shstrndx < ctx.e_shnum )
    {
        ShdrFields shstrSec;
        ParseShdr(ctx, ctx.e_shstrndx, shstrSec);
        if ( RangeFits(shstrSec.sh_offset, shstrSec.sh_size, ctx.size) )
        {
            shstr      = ctx.base + shstrSec.sh_offset;
            shstr_size = shstrSec.sh_size;
        }
    }
    const char *secName = GetString(shstr, shstr_size, sec.sh_name);

    if ( !RangeFits(sec.sh_offset, sec.sh_size, ctx.size) )
    {
        line.Format("\r\nNote section '%s': (out of bounds)\r\n", secName);
        str += line;
        return str;
    }

    line.Format("\r\nNote section '%s' (section %llu, %llu bytes):\r\n",
                secName, (unsigned long long)secIndex, (unsigned long long)sec.sh_size);
    str += line;

    const BYTE *base = ctx.base + sec.sh_offset;
    const bool  le   = ctx.is_le;
    uint64_t off = 0;

    while ( off + 12 <= sec.sh_size )
    {
        uint32_t namesz = elf_u32(base + off + 0, le);
        uint32_t descsz = elf_u32(base + off + 4, le);
        uint32_t type   = elf_u32(base + off + 8, le);
        off += 12;

        // Overflow-safe check: namesz/descsz are 32-bit, RoundUp4 widens to
        // uint64_t, but (noff + padded_name + padded_desc) can still exceed
        // uint64_t if attacker-controlled. Compute each step with RangeFits.
        uint64_t nameEnd, descEnd;
        uint64_t paddedName = RoundUp4((uint64_t)namesz);
        uint64_t paddedDesc = RoundUp4((uint64_t)descsz);
        if ( !RangeFits(off, paddedName, sec.sh_size) ) break;
        nameEnd = off + paddedName;
        if ( !RangeFits(nameEnd, paddedDesc, sec.sh_size) ) break;
        descEnd = nameEnd + paddedDesc;
        (void)descEnd;

        const BYTE *name = base + off;
        const BYTE *desc = name + RoundUp4(namesz);

        // Extract a NUL-trimmed name string (note name is typically NUL-terminated).
        CStringA nameStr;
        if ( namesz > 0 )
        {
            uint32_t realLen = namesz;
            while ( realLen > 0 && name[realLen - 1] == 0 ) realLen--;
            nameStr.Append((const char *)name, (int)realLen);
        }

        bool isGnu = (nameStr == "GNU");
        const char *typeName = isGnu ? GetGnuNoteTypeName(type) : NULL;

        line.Format("  Owner: %-16s Type: 0x%-8x (%s) Size: %u\r\n",
                    nameStr.IsEmpty() ? "<empty>" : (LPCSTR)nameStr,
                    type,
                    typeName ? typeName : "?",
                    descsz);
        str += line;

        if ( isGnu && type == NT_GNU_BUILD_ID )
        {
            CStringA id = DumpBuildId(desc, descsz);
            line.Format("    Build ID: %s\r\n", (LPCSTR)id);
            str += line;
        }
        else if ( isGnu && type == NT_GNU_ABI_TAG )
        {
            CStringA info = DumpGnuAbiTag(desc, descsz, le);
            line.Format("    %s\r\n", (LPCSTR)info);
            str += line;
        }
        else if ( isGnu && type == NT_GNU_PROPERTY_TYPE_0 )
        {
            str += "    Properties:\r\n";
            str += DumpGnuProperty(desc, descsz, le, ctx.e_machine);
        }

        off += RoundUp4(namesz) + RoundUp4(descsz);
    }

    return str;
}

static CStringA DumpNoteSections( const ElfContext &ctx )
{
    CStringA str;

    if ( ctx.e_shnum == 0 || ctx.e_shoff == 0 ) return str;

    for ( uint64_t i = 0; i < ctx.e_shnum; i++ )
    {
        ShdrFields s;
        ParseShdr(ctx, i, s);
        if ( s.sh_type == SHT_NOTE )
            str += DumpOneNoteSection(ctx, i, s);
    }

    return str;
}

//-----------------------------------
// Version-info summary for the m_fi (file properties) tab
//-----------------------------------

static void CollectElfVersionInfo( const ElfContext &ctx, CStringA &str )
{
    CStringA line;

    // Ehdr one-liner.
    const char *szType    = LOOKUP(g_ElfType,    ctx.e_type);
    const char *szMachine = LOOKUP(g_ElfMachine, ctx.e_machine);
    const BYTE *ei        = ctx.base;
    const char *szOSABI   = LOOKUP(g_ElfOSABI,   ei[EI_OSABI]);
    line.Format("ELF File Information:\r\n"
                "  Format\t: %s %s  %s  OS/ABI=%s\r\n",
                ctx.is_64 ? "ELF64" : "ELF32",
                ctx.is_le ? "LE"    : "BE",
                szType    ? szType    : "?",
                szOSABI   ? szOSABI   : "?");
    str += line;
    line.Format("  Machine\t: %s\r\n", szMachine ? szMachine : "?");
    str += line;

    if ( ctx.e_shoff == 0 || ctx.e_shnum == 0 ) return;

    // Section name table.
    const BYTE *shstr      = NULL;
    uint64_t    shstr_size = 0;
    if ( ctx.e_shstrndx != 0 && ctx.e_shstrndx < ctx.e_shnum )
    {
        ShdrFields shstrSec;
        ParseShdr(ctx, ctx.e_shstrndx, shstrSec);
        if ( RangeFits(shstrSec.sh_offset, shstrSec.sh_size, ctx.size) )
        {
            shstr      = ctx.base + shstrSec.sh_offset;
            shstr_size = shstrSec.sh_size;
        }
    }

    for ( uint64_t i = 0; i < ctx.e_shnum; i++ )
    {
        ShdrFields s;
        ParseShdr(ctx, i, s);

        const char *secName = GetString(shstr, shstr_size, s.sh_name);

        if ( s.sh_type == SHT_DYNAMIC )
        {
            // Pick up the linked string table for DT_NEEDED / DT_SONAME / DT_R(UN)PATH.
            const BYTE *strtab      = NULL;
            uint64_t    strtab_size = 0;
            if ( s.sh_link != 0 && s.sh_link < ctx.e_shnum )
            {
                ShdrFields strSec;
                ParseShdr(ctx, s.sh_link, strSec);
                if ( strSec.sh_type == SHT_STRTAB
                     && RangeFits(strSec.sh_offset, strSec.sh_size, ctx.size) )
                {
                    strtab      = ctx.base + strSec.sh_offset;
                    strtab_size = strSec.sh_size;
                }
            }

            uint64_t entsize = s.sh_entsize != 0 ? s.sh_entsize
                                                 : MinDynSize(ctx.is_64);
            if ( entsize < MinDynSize(ctx.is_64) ) continue;
            if ( !RangeFits(s.sh_offset, s.sh_size, ctx.size) ) continue;

            uint64_t n = s.sh_size / entsize;
            const BYTE *entries = ctx.base + s.sh_offset;

            for ( uint64_t j = 0; j < n; j++ )
            {
                const BYTE *e = entries + j * entsize;
                uint64_t d_tag, d_val;
                if ( ctx.is_64 )
                {
                    d_tag = elf_u64(e + 0, ctx.is_le);
                    d_val = elf_u64(e + 8, ctx.is_le);
                }
                else
                {
                    d_tag = (uint64_t)elf_u32(e + 0, ctx.is_le);
                    d_val = (uint64_t)elf_u32(e + 4, ctx.is_le);
                }
                if ( d_tag == DT_NULL ) break;

                if ( d_tag == DT_SONAME || d_tag == DT_RPATH || d_tag == DT_RUNPATH )
                {
                    const char *v = GetString(strtab, strtab_size, (uint32_t)d_val);
                    const char *tagName = GetDtTagName(d_tag);
                    line.Format("  DT_%s\t: %s\r\n",
                                tagName ? tagName : "?",
                                (v && *v) ? v : "");
                    str += line;
                }
            }
        }
        else if ( s.sh_type == SHT_NOTE )
        {
            if ( !RangeFits(s.sh_offset, s.sh_size, ctx.size) ) continue;
            const BYTE *nbase = ctx.base + s.sh_offset;
            uint64_t noff = 0;
            while ( noff + 12 <= s.sh_size )
            {
                uint32_t namesz = elf_u32(nbase + noff + 0, ctx.is_le);
                uint32_t descsz = elf_u32(nbase + noff + 4, ctx.is_le);
                uint32_t ntype  = elf_u32(nbase + noff + 8, ctx.is_le);
                noff += 12;

                uint64_t paddedName = RoundUp4((uint64_t)namesz);
                uint64_t paddedDesc = RoundUp4((uint64_t)descsz);
                if ( !RangeFits(noff, paddedName, s.sh_size) ) break;
                uint64_t nameEnd = noff + paddedName;
                if ( !RangeFits(nameEnd, paddedDesc, s.sh_size) ) break;

                const BYTE *nname = nbase + noff;
                const BYTE *ndesc = nname + RoundUp4(namesz);

                CStringA nameStr;
                if ( namesz > 0 )
                {
                    uint32_t realLen = namesz;
                    while ( realLen > 0 && nname[realLen - 1] == 0 ) realLen--;
                    nameStr.Append((const char *)nname, (int)realLen);
                }

                if ( nameStr == "GNU" && ntype == NT_GNU_BUILD_ID )
                {
                    CStringA id = DumpBuildId(ndesc, descsz);
                    line.Format("  Build ID\t: %s\r\n", (LPCSTR)id);
                    str += line;
                }
                else if ( nameStr == "GNU" && ntype == NT_GNU_ABI_TAG )
                {
                    CStringA abi = DumpGnuAbiTag(ndesc, descsz, ctx.is_le);
                    line.Format("  ABI Tag\t: %s\r\n", (LPCSTR)abi);
                    str += line;
                }

                noff += RoundUp4(namesz) + RoundUp4(descsz);
            }
        }
        else if ( s.sh_type == SHT_PROGBITS && secName && *secName
                  && s.sh_size > 0
                  && RangeFits(s.sh_offset, s.sh_size, ctx.size) )
        {
            if ( strcmp(secName, ".interp") == 0 )
            {
                // .interp contains a single NUL-terminated interpreter path.
                const char *interp     = (const char *)(ctx.base + s.sh_offset);
                size_t      interp_max = (size_t)s.sh_size;
                size_t      interp_len = strnlen(interp, interp_max);
                CStringA    interp_str;
                interp_str.Append(interp, (int)interp_len);
                line.Format("  Interpreter\t: %s\r\n", (LPCSTR)interp_str);
                str += line;
            }
            else if ( strcmp(secName, ".comment") == 0 )
            {
                // .comment is a sequence of NUL-terminated strings (compiler/linker banners).
                const char *p         = (const char *)(ctx.base + s.sh_offset);
                size_t      remaining = (size_t)s.sh_size;
                while ( remaining > 0 )
                {
                    size_t len = strnlen(p, remaining);
                    if ( len == 0 ) { p++; remaining--; continue; }
                    CStringA comment;
                    comment.Append(p, (int)len);
                    line.Format("  Comment\t: %s\r\n", (LPCSTR)comment);
                    str += line;
                    if ( len >= remaining ) break;
                    p         += len + 1;
                    remaining -= len + 1;
                }
            }
        }
    }
}

CStringA GetElfVersionInfo( LPCTSTR filename )
{
    CStringA result;
    if ( !filename ) return result;

    HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if ( hFile == INVALID_HANDLE_VALUE ) return result;

    LARGE_INTEGER sz;
    if ( !GetFileSizeEx(hFile, &sz) || sz.QuadPart < EI_NIDENT )
    {
        CloseHandle(hFile);
        return result;
    }

    HANDLE hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if ( !hMap ) { CloseHandle(hFile); return result; }

    const BYTE *base = (const BYTE *)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if ( !base ) { CloseHandle(hMap); CloseHandle(hFile); return result; }

    do
    {
        if ( base[EI_MAG0] != ELFMAG0 || base[EI_MAG1] != ELFMAG1
             || base[EI_MAG2] != ELFMAG2 || base[EI_MAG3] != ELFMAG3 ) break;

        BYTE ei_class = base[EI_CLASS];
        BYTE ei_data  = base[EI_DATA];
        if ( (ei_class != ELFCLASS32 && ei_class != ELFCLASS64)
             || (ei_data != ELFDATA2LSB && ei_data != ELFDATA2MSB) ) break;

        ElfContext ctx;
        ctx.base  = base;
        ctx.size  = (ULONG_PTR)sz.QuadPart;
        ctx.is_64 = (ei_class == ELFCLASS64);
        ctx.is_le = (ei_data == ELFDATA2LSB);

        ULONG_PTR min_ehdr = ctx.is_64 ? 64u : 52u;
        if ( ctx.size < min_ehdr ) break;

        ParseEhdr(ctx);
        ResolveExtendedNumbering(ctx);

        // Same entsize guards as the full dumper: CollectElfVersionInfo walks
        // sections via ParseShdr, which assumes a real-sized entsize.
        if ( ctx.e_shnum > 0 && ctx.e_shoff != 0 )
        {
            if ( ctx.e_shentsize < MinShdrSize(ctx.is_64)
                 || !TableFits(ctx.e_shoff, ctx.e_shnum, ctx.e_shentsize, ctx.size) )
            {
                ctx.e_shnum = 0;
                ctx.e_shoff = 0;
            }
        }

        CollectElfVersionInfo(ctx, result);
    }
    while ( 0 );

    UnmapViewOfFile(base);
    CloseHandle(hMap);
    CloseHandle(hFile);
    return result;
}

//-----------------------------------
// Main entry point
//-----------------------------------

CStringA DumpElfFile( const BYTE *base, ULONG_PTR size )
{
    CStringA str;

    if ( !base || size < EI_NIDENT
         || base[EI_MAG0] != ELFMAG0 || base[EI_MAG1] != ELFMAG1
         || base[EI_MAG2] != ELFMAG2 || base[EI_MAG3] != ELFMAG3 )
    {
        str = "Not a valid ELF file.";
        return str;
    }

    BYTE ei_class = base[EI_CLASS];
    BYTE ei_data  = base[EI_DATA];

    if ( ei_class != ELFCLASS32 && ei_class != ELFCLASS64 )
    {
        str.Format("Unsupported ELF class: %u\r\n", (unsigned)ei_class);
        return str;
    }
    if ( ei_data != ELFDATA2LSB && ei_data != ELFDATA2MSB )
    {
        str.Format("Unsupported ELF data encoding: %u\r\n", (unsigned)ei_data);
        return str;
    }

    ElfContext ctx;
    ctx.base  = base;
    ctx.size  = size;
    ctx.is_64 = (ei_class == ELFCLASS64);
    ctx.is_le = (ei_data == ELFDATA2LSB);

    const ULONG_PTR min_ehdr = ctx.is_64 ? 64u : 52u;
    if ( size < min_ehdr )
    {
        str = "ELF header truncated.";
        return str;
    }

    ParseEhdr(ctx);
    ResolveExtendedNumbering(ctx);

    str += DumpEhdr(ctx);

    // Section-header and program-header tables: reject up front if the
    // file-reported entsize is smaller than the real structure size, or if
    // (offset + count*entsize) overflows or exceeds the mapped region. Every
    // downstream dumper walks these tables via ParseShdr / ParsePhdr, which
    // read full structures regardless of entsize, so a bad table here is
    // exploitable across every section-dependent dump.
    if ( ctx.e_shnum > 0 && ctx.e_shoff != 0 )
    {
        if ( ctx.e_shentsize < MinShdrSize(ctx.is_64)
             || !TableFits(ctx.e_shoff, ctx.e_shnum, ctx.e_shentsize, ctx.size) )
        {
            str += "\r\nSection header table invalid (entsize or bounds); "
                   "section-dependent dumps suppressed.\r\n";
            ctx.e_shnum = 0;
            ctx.e_shoff = 0;
        }
    }
    if ( ctx.e_phnum > 0 && ctx.e_phoff != 0 )
    {
        if ( ctx.e_phentsize < MinPhdrSize(ctx.is_64)
             || !TableFits(ctx.e_phoff, ctx.e_phnum, ctx.e_phentsize, ctx.size) )
        {
            str += "\r\nProgram header table invalid (entsize or bounds); "
                   "program header dump suppressed.\r\n";
            ctx.e_phnum = 0;
            ctx.e_phoff = 0;
        }
    }

    str += DumpShdrTable(ctx);
    str += DumpPhdrTable(ctx);
    str += DumpDynamicSection(ctx);
    str += DumpSymbolTables(ctx);
    str += DumpRelocationSections(ctx);
    str += DumpNoteSections(ctx);

    return str;
}
