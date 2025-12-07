#ifndef MM_H
#define MM_H

#include "common.h"
#include "bitops.h"

/* FIXED: Include mm64.h if MM64 is defined to import 64-bit constants */
#ifdef MM64
#include "mm64.h"
#endif

/* CPU Bus definition */
#ifndef MM64
/* 32-BIT SETTINGS (Legacy) */
#define PAGING_CPU_BUS_WIDTH 22 
#define PAGING_PAGESZ  256      
#define PAGING_MEMRAMSZ BIT(21)
#define PAGING_PAGE_ALIGNSZ(sz) (DIV_ROUND_UP(sz,PAGING_PAGESZ)*PAGING_PAGESZ)
#define PAGING_MEMSWPSZ BIT(29)
#define PAGING_SWPFPN_OFFSET 5  
#define PAGING_MAX_PGN  (DIV_ROUND_UP(BIT(PAGING_CPU_BUS_WIDTH),PAGING_PAGESZ))
#define PAGING_SBRK_INIT_SZ PAGING_PAGESZ

/* Helper Macros for 32-bit */
#define PAGING_OFFST_MASK  GENMASK(PAGING_ADDR_OFFST_HIBIT,PAGING_ADDR_OFFST_LOBIT)
#define PAGING_PGN_MASK  GENMASK(PAGING_ADDR_PGN_HIBIT,PAGING_ADDR_PGN_LOBIT)
#define PAGING_FPN_MASK  GENMASK(PAGING_ADDR_FPN_HIBIT,PAGING_ADDR_FPN_LOBIT)
#define PAGING_SWP_MASK  GENMASK(PAGING_SWP_HIBIT,PAGING_SWP_LOBIT)

/* Extract Macros */
#define PAGING_OFFST(x)  GETVAL(x,PAGING_OFFST_MASK,PAGING_ADDR_OFFST_LOBIT)
#define PAGING_PGN(x)  GETVAL(x,PAGING_PGN_MASK,PAGING_ADDR_PGN_LOBIT)
#define PAGING_FPN(x)  GETVAL(x,PAGING_PTE_FPN_MASK,PAGING_PTE_FPN_LOBIT)
#define PAGING_SWP(pte) ((pte&PAGING_PTE_SWPOFF_MASK) >> PAGING_SWPFPN_OFFSET)
#define PAGING_ADDR_FPN_LOBIT NBITS(PAGING_PAGESZ)
#define PAGING_ADDR_FPN_HIBIT (NBITS(PAGING_MEMRAMSZ) - 1)

#else 
/* 64-BIT SETTINGS MAPPING */
/* Map generic names to 64-bit specific definitions from mm64.h */
#define PAGING_PAGESZ PAGING64_PAGESZ
#define PAGING_PAGE_ALIGNSZ(sz) PAGING64_PAGE_ALIGNSZ(sz)
#define PAGING_MAX_PGN PAGING64_MAX_PGN

/* Map extraction macros used in mm-vm.c to 64-bit versions */
#define PAGING_PGN(x)  PAGING64_ADDR_PGN(x)
#define PAGING_OFFST(x) PAGING64_ADDR_OFFST(x)
/* Note: FPN extraction differs in 64-bit, usually handled inside specific functions, 
   but for consistency with mm-vm.c: */
#define PAGING_ADDR_FPN_LOBIT PAGING64_ADDR_PT_LOBIT // Frame aligns with Page Table shift
#endif

/* Common PTE BIT Definitions (Shared) */
#define PAGING_PTE_PRESENT_MASK BIT(31) 
#define PAGING_PTE_SWAPPED_MASK BIT(30)
#define PAGING_PTE_RESERVE_MASK BIT(29)
#define PAGING_PTE_DIRTY_MASK BIT(28)
#define PAGING_PTE_EMPTY01_MASK BIT(14)
#define PAGING_PTE_EMPTY02_MASK BIT(13)

/* ... (Keep the rest of the structs and prototypes) ... */

/* ... (Keep Value operators SETBIT, GETVAL, etc.) ... */

#ifndef MM64
/* 32-bit specific masks (Only define if NOT MM64 to avoid conflict) */
#define PAGING_ADDR_OFFST_LOBIT 0
#define PAGING_ADDR_OFFST_HIBIT (NBITS(PAGING_PAGESZ) - 1)
#define PAGING_ADDR_PGN_LOBIT NBITS(PAGING_PAGESZ)
#define PAGING_ADDR_PGN_HIBIT (PAGING_CPU_BUS_WIDTH - 1)
/* ... */
#endif

#endif
