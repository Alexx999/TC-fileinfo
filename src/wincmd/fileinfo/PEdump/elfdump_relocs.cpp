//==================================
// FILEINFO - ELF arch-specific relocation names
// FILE: ELFDUMP_RELOCS.CPP
//
// Thin shim over LLVM ELFRelocs/*.def files. Each arch function expands the
// matching .def via an ELF_RELOC(name, value) X-macro into a case-label switch.
//==================================

#include "stdafx.h"
#include "elfdump_relocs.h"
#include <stdint.h>
#include "elf.h"

static const char *Reloc_AArch64( uint32_t t )
{
    switch ( t ) {
    #define ELF_RELOC(name, value) case value: return #name;
    #include "ELFRelocs/AArch64.def"
    #undef  ELF_RELOC
    default: return NULL;
    }
}

static const char *Reloc_ARM( uint32_t t )
{
    switch ( t ) {
    #define ELF_RELOC(name, value) case value: return #name;
    #include "ELFRelocs/ARM.def"
    #undef  ELF_RELOC
    default: return NULL;
    }
}

static const char *Reloc_AVR( uint32_t t )
{
    switch ( t ) {
    #define ELF_RELOC(name, value) case value: return #name;
    #include "ELFRelocs/AVR.def"
    #undef  ELF_RELOC
    default: return NULL;
    }
}

static const char *Reloc_BPF( uint32_t t )
{
    switch ( t ) {
    #define ELF_RELOC(name, value) case value: return #name;
    #include "ELFRelocs/BPF.def"
    #undef  ELF_RELOC
    default: return NULL;
    }
}

static const char *Reloc_CSKY( uint32_t t )
{
    switch ( t ) {
    #define ELF_RELOC(name, value) case value: return #name;
    #include "ELFRelocs/CSKY.def"
    #undef  ELF_RELOC
    default: return NULL;
    }
}

static const char *Reloc_Hexagon( uint32_t t )
{
    switch ( t ) {
    #define ELF_RELOC(name, value) case value: return #name;
    #include "ELFRelocs/Hexagon.def"
    #undef  ELF_RELOC
    default: return NULL;
    }
}

static const char *Reloc_LoongArch( uint32_t t )
{
    switch ( t ) {
    #define ELF_RELOC(name, value) case value: return #name;
    #include "ELFRelocs/LoongArch.def"
    #undef  ELF_RELOC
    default: return NULL;
    }
}

static const char *Reloc_M68k( uint32_t t )
{
    switch ( t ) {
    #define ELF_RELOC(name, value) case value: return #name;
    #include "ELFRelocs/M68k.def"
    #undef  ELF_RELOC
    default: return NULL;
    }
}

static const char *Reloc_MSP430( uint32_t t )
{
    switch ( t ) {
    #define ELF_RELOC(name, value) case value: return #name;
    #include "ELFRelocs/MSP430.def"
    #undef  ELF_RELOC
    default: return NULL;
    }
}

static const char *Reloc_Mips( uint32_t t )
{
    switch ( t ) {
    #define ELF_RELOC(name, value) case value: return #name;
    #include "ELFRelocs/Mips.def"
    #undef  ELF_RELOC
    default: return NULL;
    }
}

static const char *Reloc_PowerPC( uint32_t t )
{
    switch ( t ) {
    #define ELF_RELOC(name, value) case value: return #name;
    #include "ELFRelocs/PowerPC.def"
    #undef  ELF_RELOC
    default: return NULL;
    }
}

static const char *Reloc_PowerPC64( uint32_t t )
{
    switch ( t ) {
    #define ELF_RELOC(name, value) case value: return #name;
    #include "ELFRelocs/PowerPC64.def"
    #undef  ELF_RELOC
    default: return NULL;
    }
}

static const char *Reloc_RISCV( uint32_t t )
{
    switch ( t ) {
    #define ELF_RELOC(name, value) case value: return #name;
    #include "ELFRelocs/RISCV.def"
    #undef  ELF_RELOC
    default: return NULL;
    }
}

static const char *Reloc_Sparc( uint32_t t )
{
    switch ( t ) {
    #define ELF_RELOC(name, value) case value: return #name;
    #include "ELFRelocs/Sparc.def"
    #undef  ELF_RELOC
    default: return NULL;
    }
}

static const char *Reloc_SystemZ( uint32_t t )
{
    switch ( t ) {
    #define ELF_RELOC(name, value) case value: return #name;
    #include "ELFRelocs/SystemZ.def"
    #undef  ELF_RELOC
    default: return NULL;
    }
}

static const char *Reloc_Xtensa( uint32_t t )
{
    switch ( t ) {
    #define ELF_RELOC(name, value) case value: return #name;
    #include "ELFRelocs/Xtensa.def"
    #undef  ELF_RELOC
    default: return NULL;
    }
}

static const char *Reloc_i386( uint32_t t )
{
    switch ( t ) {
    #define ELF_RELOC(name, value) case value: return #name;
    #include "ELFRelocs/i386.def"
    #undef  ELF_RELOC
    default: return NULL;
    }
}

static const char *Reloc_x86_64( uint32_t t )
{
    switch ( t ) {
    #define ELF_RELOC(name, value) case value: return #name;
    #include "ELFRelocs/x86_64.def"
    #undef  ELF_RELOC
    default: return NULL;
    }
}

const char *GetElfRelocationTypeName( unsigned int machine, unsigned int type )
{
    switch ( machine )
    {
    case EM_AARCH64:        return Reloc_AArch64(type);
    case EM_ARM:            return Reloc_ARM(type);
    case EM_AVR:            return Reloc_AVR(type);
    case EM_BPF:            return Reloc_BPF(type);
    case EM_CSKY:           return Reloc_CSKY(type);
    case EM_QDSP6:          return Reloc_Hexagon(type);  // Hexagon uses QDSP6 EM_ value
    case EM_LOONGARCH:      return Reloc_LoongArch(type);
    case EM_68K:            return Reloc_M68k(type);
    case EM_MSP430:         return Reloc_MSP430(type);
    case EM_MIPS:
    case EM_MIPS_RS3_LE:    return Reloc_Mips(type);
    case EM_PPC:            return Reloc_PowerPC(type);
    case EM_PPC64:          return Reloc_PowerPC64(type);
    case EM_RISCV:          return Reloc_RISCV(type);
    case EM_SPARC:
    case EM_SPARC32PLUS:
    case EM_SPARCV9:        return Reloc_Sparc(type);
    case EM_S390:           return Reloc_SystemZ(type);
    case EM_XTENSA:         return Reloc_Xtensa(type);
    case EM_386:
    case EM_IAMCU:          return Reloc_i386(type);
    case EM_X86_64:         return Reloc_x86_64(type);
    default:                return NULL;
    }
}
