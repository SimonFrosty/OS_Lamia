/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  struct vm_area_struct *pvma = mm->mmap;

  while (pvma != NULL)
  {
    if (pvma->vm_id == vmaid)
      return pvma;

    pvma = pvma->vm_next;
  }

  return NULL;
}

int __mm_swap_page(struct pcb_t *caller, addr_t vicfpn , addr_t swpfpn)
{
    __swap_cp_page(caller->krnl->mram, vicfpn, caller->krnl->active_mswp, swpfpn);
    return 0;
}

/* get_vm_area_node_at_brk - get vm area for a number of pages
 * @caller: caller
 * @vmaid: ID vm area to alloc memory region
 * @size: size of the region requested
 * @alignedsz: size aligned to page size (amount to increase limit)
 */
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, addr_t size, addr_t alignedsz)
{
  struct vm_rg_struct *newrg;
  /* Retrieve current vma to obtain sbrk */
  /* Note: Accessing mm via caller->krnl->mm based on assignment requirement */
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);

  if (cur_vma == NULL)
    return NULL;

  newrg = malloc(sizeof(struct vm_rg_struct));

  /* Update the newrg boundary based on current sbrk */
  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end = cur_vma->sbrk + size;

  return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma start
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, addr_t vmastart, addr_t vmaend)
{
  /* Access the list of VMAs via the kernel structure */
  struct vm_area_struct *vma = caller->krnl->mm->mmap;

  /* Invalid address range check */
  if (vmastart >= vmaend)
  {
    return -1;
  }

  /* Iterate through all existing VM Areas */
  while (vma != NULL)
  {
    /* We only check for overlap with OTHER areas.
     * (e.g. We want to ensure the Heap (vmaid=0) doesn't grow into the Stack (vmaid=1))
     */
    if (vma->vm_id != vmaid) 
    {
        /* Check if the requested range overlaps with this existing VMA */
        if (OVERLAP(vmastart, vmaend, vma->vm_start, vma->vm_end))
        {
            return -1; // Overlap detected!
        }
    }
    
    vma = vma->vm_next;
  }

  return 0;
}

/* inc_vma_limit - increase vm area limits to reserve space for new variable
 * @caller: caller
 * @vmaid: ID vm area to alloc memory region
 * @inc_sz: increment size
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, addr_t inc_sz)
{
  /* Page-align the increment and compute page count */
  addr_t inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
  int incnumpage = inc_amt / PAGING_PAGESZ;

  /* Get the current VMA via Kernel Structure */
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);
  if (cur_vma == NULL)
    return -1;

  addr_t mapstart = cur_vma->vm_end;           /* map immediately after current end */
  addr_t mapend   = mapstart + inc_amt;        /* aligned end */

  /* Validate overlap against other VMAs for the new span */
  if (validate_overlap_vm_area(caller, vmaid, mapstart, mapend) < 0)
    return -1; /* Overlap detected */

  /* Map the new physical frames for the aligned span */
  if (vm_map_ram(caller, mapstart, mapend, mapstart, incnumpage, NULL) < 0)
    return -1; /* Map failed (OOM) */
  
  /* SUCCESS: advance limits; sbrk grows by requested (possibly unaligned) size */
  cur_vma->vm_end = mapend;
  cur_vma->sbrk += inc_sz;

  return 0;
}

/* VM prototypes */
/*
 * pgalloc - Paging-based ALLOC wrapper
 * @proc: process executing the instruction
 * @size: size of memory to allocate
 * @reg_index: register index to store the allocated address (and serve as Region ID)
 */
int pgalloc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  addr_t addr;

  /* By default using vmaid = 0 */
  return __alloc(proc, 0, reg_index, size, &addr);
}

/*
 * pgfree_data - Paging-based FREE wrapper
 * @proc: process executing the instruction
 * @reg_index: register index containing the region ID to free
 */
int pgfree_data(struct pcb_t *proc, uint32_t reg_index)
{
  return __free(proc, 0, reg_index);
}

// #endif