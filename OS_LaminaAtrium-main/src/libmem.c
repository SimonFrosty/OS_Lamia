/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

// #ifdef MM_PAGING
/*
 * System Library
 * Memory Module Library libmem.c 
 */

#include "syscall.h"
#include "common.h"
#include "string.h"
#include "mm.h"
#include "mm64.h"
#include "libmem.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

static pthread_mutex_t mmvm_lock = PTHREAD_MUTEX_INITIALIZER;

/*enlist_vm_freerg_list - add new rg to freerg_list
 *@mm: memory region
 *@rg_elmt: new region
 *
 */
int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct *rg_elmt)
{
  struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;

  if (rg_elmt->rg_start >= rg_elmt->rg_end)
    return -1;

  if (rg_node != NULL)
    rg_elmt->rg_next = rg_node;

  /* Enlist the new region */
  mm->mmap->vm_freerg_list = rg_elmt;

  return 0;
}

/*get_symrg_byid - get mem region by region ID
 *@mm: memory region
 *@rgid: region ID act as symbol index of variable
 *
 */
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
{
  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return NULL;

  return &mm->symrgtbl[rgid];
}

/*__alloc - allocate a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *@alloc_addr: address of allocated memory region
 *
 */
int __alloc(struct pcb_t *caller, int vmaid, int rgid, addr_t size, addr_t *alloc_addr)
{
  /*Allocate at the toproof */
  pthread_mutex_lock(&mmvm_lock);
  struct vm_rg_struct rgnode;
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);
  int inc_sz=0;

  if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
  {
    caller->krnl->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
    caller->krnl->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
 
    *alloc_addr = rgnode.rg_start;

    pthread_mutex_unlock(&mmvm_lock);
    return 0;
  }

  /* TODO get_free_vmrg_area FAILED handle the region management (Fig.6)*/

  /*Attempt to increate limit to get space */
#ifdef MM64
  inc_sz = (uint32_t)(size/(int)PAGING64_PAGESZ);
  inc_sz = inc_sz + 1;
#else
  inc_sz = PAGING_PAGE_ALIGNSZ(size);
#endif
  int old_sbrk;
  inc_sz = inc_sz + 1;

  old_sbrk = cur_vma->sbrk;

  /* TODO INCREASE THE LIMIT
   * SYSCALL 1 sys_memmap
   */
  struct sc_regs regs;
  regs.a1 = SYSMEM_INC_OP;
  regs.a2 = vmaid;
#ifdef MM64
  regs.a3 = size;
#else
  regs.a3 = PAGING_PAGE_ALIGNSZ(size);
#endif  
  syscall(caller->krnl, caller->pid, 17, &regs); /* SYSCALL 17 sys_memmap */

  /*Successful increase limit */
  caller->krnl->mm->symrgtbl[rgid].rg_start = old_sbrk;
  caller->krnl->mm->symrgtbl[rgid].rg_end = old_sbrk + size;

  *alloc_addr = old_sbrk;

  pthread_mutex_unlock(&mmvm_lock);
  return 0;

}

/*__free - remove a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __free(struct pcb_t *caller, int vmaid, int rgid)
{
  pthread_mutex_lock(&mmvm_lock);

  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  /* TODO: Manage the collect freed region to freerg_list */
  struct vm_rg_struct *rgnode = get_symrg_byid(caller->krnl->mm, rgid);

  if (rgnode->rg_start == 0 && rgnode->rg_end == 0)
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  /* Create a new node to store in the free list */
  struct vm_rg_struct *freerg_node = malloc(sizeof(struct vm_rg_struct));
  freerg_node->rg_start = rgnode->rg_start;
  freerg_node->rg_end = rgnode->rg_end;
  freerg_node->rg_next = NULL;

  rgnode->rg_start = 0;
  rgnode->rg_end = 0;
  rgnode->rg_next = NULL;

  /*enlist the obsoleted memory region */
  enlist_vm_freerg_list(caller->krnl->mm, freerg_node);

  pthread_mutex_unlock(&mmvm_lock);
  return 0;
}

/*liballoc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int liballoc(struct pcb_t *proc, addr_t size, uint32_t reg_index)
{
  addr_t  addr;

  /* Allocate the region using the internal __alloc logic */
  int val = __alloc(proc, 0, reg_index, size, &addr);
  printf("%s:%d\n", __func__, __LINE__);
  if (val == -1)
  {
    return -1;
  }
#ifdef IODUMP
  /* TODO dump IO content (if needed) */
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
#endif

  /* By default using vmaid = 0 */
  return val;
}

/*libfree - PAGING-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */

int libfree(struct pcb_t *proc, uint32_t reg_index)
{
  /* Free the region using the internal __free logic */
  int val = __free(proc, 0, reg_index);
  if (val == -1)
  {
    return -1;
  }
printf("%s:%d\n",__func__,__LINE__);
#ifdef IODUMP
  /* TODO dump IO content (if needed) */
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
#endif
  return 0;//val;
}

/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
  /* Sanity: bounds */
  if (pgn < 0 || pgn >= PAGING_MAX_PGN) return -1;
  
  uint32_t pte = pte_get_entry(caller, pgn);

  if (!PAGING_PAGE_PRESENT(pte))
  { /* Page is not online, make it actively living */

    addr_t frame;
    addr_t tgt_swpfpn = PAGING_PTE_SWP(pte); // The frame where target is currently stored in SWAP
    addr_t vicpgn;
    addr_t swpfpn;
    uint32_t vicpte;
    addr_t vicfpn;

    /* Prepare for System Call */
    struct sc_regs regs;

    /* 1. Try to get a free frame in RAM */
    if (MEMPHY_get_freefp(caller->krnl->mram, &frame) < 0)
    {
      /* TODO Initialize the target frame storing our variable */
      //  addr_t tgtfpn 

      /* TODO: Play with your paging theory here */
      /* Find victim page */
      if (find_victim_page(caller->krnl->mm, &vicpgn) == -1)
      {
        return -1;
      }

      /* Get free frame in MEMSWP */
      if (MEMPHY_get_freefp(caller->krnl->active_mswp, &swpfpn) == -1)
      {
        return -1;
      }

      /* Get Victim's RAM frame */
      vicpte = pte_get_entry(caller, vicpgn);
      if (!PAGING_PAGE_PRESENT(vicpte)) {
        return -1; /* inconsistent: victim not in RAM */
      }
      vicfpn = PAGING_PTE_FPN(vicpte);

      /* Copy the victim's data from RAM to SWAP before we lose it! */
      __swap_cp_page(caller->krnl->mram, vicfpn, caller->krnl->active_mswp, swpfpn); 

      /* TODO: Implement swap frame from MEMRAM to MEMSWP and vice versa*/
      /* Set up registers for syscall */
      regs.a1 = SYSMEM_SWP_OP;
      regs.a2 = vicfpn;  // From RAM
      regs.a3 = swpfpn;  // To SWAP

      /* TODO copy victim frame to swap 
      * SWP(vicfpn <--> swpfpn)
      * SYSCALL 1 sys_memmap
      */
      /* Trigger System Call to Copy Data */
      syscall(caller->krnl, caller->pid, 17, &regs); // 17 is memmap syscall

      /* Update Victim's Page Table */
      pte_set_swap(caller, vicpgn, 0, swpfpn);

      /* Steal the victim's frame */
      frame = vicfpn;
    }

    /* Update page table */
    //pte_set_swap(...);
    /* Done in the if above (OK)*/

    /* 2. SWAP IN: Bring Target from SWAP to RAM */
    /* Note: The default SYSMEM_SWP_OP is typically RAM->SWAP (Swap Out). 
     * For Swap In (SWAP->RAM), we direct copy using the kernel function because 
     * no separate syscall opcode was defined for "Swap In".
     */
    if (tgt_swpfpn != 0) {
        __swap_cp_page(caller->krnl->active_mswp, tgt_swpfpn, caller->krnl->mram, frame);
        MEMPHY_put_freefp(caller->krnl->active_mswp, tgt_swpfpn);
    } else {
      /* Fresh page: clear old contents to avoid leaking stale data */
      int base = frame * PAGING_PAGESZ;
      memset(&caller->krnl->mram->storage[base], 0, PAGING_PAGESZ);
    }

    /* Update its online status of the target page */
    //pte_set_fpn(...);
    pte_set_fpn(caller, pgn,frame);

    enlist_pgn_node(&caller->krnl->mm->fifo_pgn, pgn);
  }

  *fpn = PAGING_FPN(pte_get_entry(caller,pgn));

  return 0;
}

/*pg_getval - read value at given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; /* invalid page access */

  /* Calculate physical address */
  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  /* TODO 
   *  MEMPHY_read(caller->krnl->mram, phyaddr, data);
   *  MEMPHY READ 
   *  SYSCALL 17 sys_memmap with SYSMEM_IO_READ
   */

  /* Perform Physical Read via System Call */
  struct sc_regs regs;
  regs.a1 = SYSMEM_IO_READ;
  regs.a2 = phyaddr;
  regs.a3 = 0; // Placeholder for result

  /* Execute System Call */
  syscall(caller->krnl, caller->pid, 17, &regs);

  /* Retrieve result from register a3 */
  *data = (BYTE)regs.a3;

  return 0;
}

/*pg_setval - write value to given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; /* invalid page access */

  /* Calculate physical address */
  int phyaddr = (fpn * PAGING_PAGESZ) + off;

  /* TODO 
   *  MEMPHY_write(caller->krnl->mram, phyaddr, value);
   *  MEMPHY WRITE with SYSMEM_IO_WRITE 
   * SYSCALL 17 sys_memmap
   */
  /* Perform Physical Write via System Call */
  struct sc_regs regs;
  regs.a1 = SYSMEM_IO_WRITE;
  regs.a2 = phyaddr;
  regs.a3 = (addr_t)value; // Data to write

  /* Execute System Call */
  syscall(caller->krnl, caller->pid, 17, &regs);

  return 0;
}

/*__read - read value in region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __read(struct pcb_t *caller, int vmaid, int rgid, addr_t offset, BYTE *data)
{
  pthread_mutex_lock(&mmvm_lock);
  struct vm_rg_struct *currg = get_symrg_byid(caller->krnl->mm, rgid);

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);

  /* TODO Invalid memory identify */
  /* Valid memory identify check */
  if (currg == NULL || cur_vma == NULL) {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  /* Calculate Virtual Address */
  int vaddr = currg->rg_start + offset;

  /* Boundary Check: Ensure read is within the allocated region */
  if (vaddr >= currg->rg_end) {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }
  
  /* Execute Page Read */
  int ret = pg_getval(caller->krnl->mm, vaddr, data, caller);
  pthread_mutex_unlock(&mmvm_lock);
  return ret;
}

/*libread - PAGING-based read a region memory */
int libread(
    struct pcb_t *proc, // Process executing the instruction
    uint32_t source,    // Index of source register
    addr_t offset,    // Source address = [source] + [offset]
    uint32_t* destination)
{
  BYTE data;
  int val = __read(proc, 0, source, offset, &data);

  // *destination = data;
  if (val == 0) {
      /* destination points to the register index (e.g., 0, 1, ... 9)
       * We need to update the process's register with the read data.
       */
      uint32_t reg_idx = *destination;
      proc->regs[reg_idx] = (addr_t)data;
  }
#ifdef IODUMP
  /* TODO dump IO content (if needed) */
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
#endif

  return val;
}

/*__write - write a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __write(struct pcb_t *caller, int vmaid, int rgid, addr_t offset, BYTE value)
{
  pthread_mutex_lock(&mmvm_lock);
  struct vm_rg_struct *currg = get_symrg_byid(caller->krnl->mm, rgid);

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);

  if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  pg_setval(caller->krnl->mm, currg->rg_start + offset, value, caller);

  pthread_mutex_unlock(&mmvm_lock);
  return 0;
}

/*libwrite - PAGING-based write a region memory */
int libwrite(
    struct pcb_t *proc,   // Process executing the instruction
    BYTE data,            // Data to be wrttien into memory
    uint32_t destination, // Index of destination register
    addr_t offset)
{
  int val = __write(proc, 0, destination, offset, data);
  if (val == -1)
  {
    return -1;
  }
#ifdef IODUMP
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
  MEMPHY_dump(proc->krnl->mram);
#endif

  return val;
}

/*free_pcb_memphy - collect all memphy of pcb
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 */
int free_pcb_memph(struct pcb_t *caller)
{
  pthread_mutex_lock(&mmvm_lock);
  int pagenum, fpn;
  uint32_t pte;

  for (pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
  {
    pte = caller->krnl->mm->pgd[pagenum];

    if (PAGING_PAGE_PRESENT(pte))
    {
      fpn = PAGING_FPN(pte);
      MEMPHY_put_freefp(caller->krnl->mram, fpn);
    }
    else
    {
      // fpn = PAGING_SWP(pte);
      // MEMPHY_put_freefp(caller->krnl->active_mswp, fpn);

      // Only free from swap if it is actually swapped out!
        if (pte & PAGING_PTE_SWAPPED_MASK) 
        {
            fpn = PAGING_SWP(pte);
            MEMPHY_put_freefp(caller->krnl->active_mswp, fpn);
        }
    }
  }

  pthread_mutex_unlock(&mmvm_lock);
  return 0;
}


/*find_victim_page - find victim page
 *@caller: caller
 *@pgn: return page number
 *
 */
int find_victim_page(struct mm_struct *mm, addr_t *retpgn)
{
  struct pgn_t *pg = mm->fifo_pgn;

  /* TODO: Implement the theorical mechanism to find the victim page */
  if (!pg)
  {
    return -1;
  }

  /* Case 1: List has only one element */
  if (pg->pg_next == NULL) {
      *retpgn = pg->pgn;
      free(pg);
      mm->fifo_pgn = NULL;
      return 0;
  }

  /* Case 2: List has multiple elements. Traverse to the end (Oldest page) */
  struct pgn_t *prev = NULL;
  while (pg->pg_next) {
    prev = pg;
    pg = pg->pg_next;
  }

  /* pg is now the last element (victim) */
  *retpgn = pg->pgn;
  
  /* Unlink the victim from the list */
  prev->pg_next = NULL;
  free(pg);

  return 0;
}

/*get_free_vmrg_area - get a free vm region
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@size: allocated size
 *
 */
int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);

  /* Safety Check */
  if (cur_vma == NULL) return -1;

  struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;
  struct vm_rg_struct *prev = NULL;

  if (rgit == NULL)
    return -1;

  /* Probe unintialized newrg */
  newrg->rg_start = newrg->rg_end = -1;

  /* Traverse on list of free vm region to find a fit space */
  while (rgit != NULL)
  {
    if (rgit->rg_start + size <= rgit->rg_end)
    { /* Current region has enough space */
      newrg->rg_start = rgit->rg_start;
      newrg->rg_end = rgit->rg_start + size;

      /* Update left space in chosen region */
      if (rgit->rg_start + size < rgit->rg_end)
      {
        /* Case 1: Partial Fit (Region is larger than needed) 
         * Logic: Shrink the free region from the bottom up 
         */
        rgit->rg_start = rgit->rg_start + size;
      }
      else
      { /*Use up all space, remove current node */
        if (prev != NULL) {
            prev->rg_next = rgit->rg_next;
        } else {
            cur_vma->vm_freerg_list = rgit->rg_next;
        }
        free(rgit);
      }
      break;
    }
    else
    {
      prev = rgit;
      rgit = rgit->rg_next; // Traverse next rg
    }
  }

  if (newrg->rg_start == -1) // new region not found
    return -1;

  return 0;
}

// int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
// {
//   struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);

//   struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;

//   if (rgit == NULL)
//     return -1;

//   /* Probe unintialized newrg */
//   newrg->rg_start = newrg->rg_end = -1;

//   /* Traverse on list of free vm region to find a fit space */
//   while (rgit != NULL)
//   {
//     if (rgit->rg_start + size <= rgit->rg_end)
//     { /* Current region has enough space */
//       newrg->rg_start = rgit->rg_start;
//       newrg->rg_end = rgit->rg_start + size;

//       /* Update left space in chosen region */
//       if (rgit->rg_start + size < rgit->rg_end)
//       {
//         rgit->rg_start = rgit->rg_start + size;
//       }
//       else
//       { /*Use up all space, remove current node */
//         /*Clone next rg node */
//         struct vm_rg_struct *nextrg = rgit->rg_next;

//         /*Cloning */
//         if (nextrg != NULL)
//         {
//           rgit->rg_start = nextrg->rg_start;
//           rgit->rg_end = nextrg->rg_end;

//           rgit->rg_next = nextrg->rg_next;

//           free(nextrg);
//         }
//         else
//         {                                /*End of free list */
//           rgit->rg_start = rgit->rg_end; // dummy, size 0 region
//           rgit->rg_next = NULL;
//         }
//       }
//       break;
//     }
//     else
//     {
//       rgit = rgit->rg_next; // Traverse next rg
//     }
//   }

//   if (newrg->rg_start == -1) // new region not found
//     return -1;

//   return 0;
// }

// #endif
