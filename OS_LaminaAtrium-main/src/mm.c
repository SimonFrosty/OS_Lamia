/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */
 
 /* NOTICE this moudle is deprecated in LamiaAtrium release
  *        the structure is maintained for future 64bit-32bit
  *        backward compatible feature or PAE feature 
  */
#include "common.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

#if !defined(MM64)
/*
 * PAGING based Memory Management
 * Memory management unit mm/mm.c
 */

/*
 * init_pte - Initialize PTE entry
 */
int init_pte(addr_t *pte,
             int pre,    // present
             addr_t fpn,    // FPN
             int drt,    // dirty
             int swp,    // swap
             int swptyp, // swap type
             addr_t swpoff) // swap offset
{
  if (pre != 0) {
    if (swp == 0) { // Non swap ~ page online
      if (fpn == 0)
        return -1;  // Invalid setting

      /* Valid setting with FPN */
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
    }
    else
    { // page swapped
      CLRBIT(*pte, PAGING_PTE_PRESENT_MASK); // swapped pages are not resident
      SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
      SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
    }
  }

  return 0;
}

/*
 * get_pd_from_pagenum - Parse address to 5 page directory level
 * @pgn   : pagenumer
 * @pgd   : page global directory
 * @p4d   : page level directory
 * @pud   : page upper directory
 * @pmd   : page middle directory
 * @pt    : page table 
 */
int get_pd_from_address(addr_t addr, addr_t* pgd, addr_t* p4d, addr_t* pud, addr_t* pmd, addr_t* pt)
{
  // printf("[ERROR] %s: This feature 32 bit mode is deprecated\n", __func__);

  if (pgd) *pgd = PAGING_PGN(addr);

  /* The other levels are not used in this scheme, set to 0 for safety */
  if (p4d) *p4d = 0;
  if (pud) *pud = 0;
  if (pmd) *pmd = 0;
  if (pt)  *pt = 0;

  return 0;
}

/*
 * get_pd_from_pagenum - Parse page number to 5 page directory level
 * @pgn   : pagenumer
 * @pgd   : page global directory
 * @p4d   : page level directory
 * @pud   : page upper directory
 * @pmd   : page middle directory
 * @pt    : page table 
 */
int get_pd_from_pagenum(addr_t pgn, addr_t* pgd, addr_t* p4d, addr_t* pud, addr_t* pmd, addr_t* pt)
{
  // printf("[ERROR] %s: This feature 32 bit mode is deprecated\n", __func__);

  if (pgd) *pgd = pgn;

  /* The other levels are not used */
  if (p4d) *p4d = 0;
  if (pud) *pud = 0;
  if (pmd) *pmd = 0;
  if (pt)  *pt = 0;

  return 0;
}

/*
 * pte_set_swap - Set PTE entry for swapped page
 * @pte    : target page table entry (PTE)
 * @swptyp : swap type
 * @swpoff : swap offset
 */
int pte_set_swap(struct pcb_t *caller, addr_t pgn, int swptyp, addr_t swpoff)
{
  struct krnl_t *krnl = caller->krnl;
  if (pgn >= PAGING_MAX_PGN)
    return -1;

  addr_t *pte = &krnl->mm->pgd[pgn];
  
  /* 1. Mark as NOT PRESENT in RAM (Trigger Page Fault on access) */
  CLRBIT(*pte, PAGING_PTE_PRESENT_MASK);
  
  /* Set Swapped bit (page is in Swap) */
  SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  /* Set Swap Type and Offset */
  SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
  SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

  /* Clear stale frame number when moved to swap */
//   SETVAL(*pte, 0, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);

  return 0;
}

/*
 * pte_set_fpn - Set PTE entry for on-line page
 * @pte   : target page table entry (PTE)
 * @fpn   : frame page number (FPN)
 */
int pte_set_fpn(struct pcb_t *caller, addr_t pgn, addr_t fpn)
{
  struct krnl_t *krnl = caller->krnl;
    if (pgn >= PAGING_MAX_PGN)
        return -1;

    addr_t *pte = &krnl->mm->pgd[pgn];

  /* Set Present bit */
  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  
  /* Clear Swapped bit (page is in RAM) */
  CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  /* Set Frame Page Number */
  SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);

  return 0;
}


/* Get PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry
 **/
uint32_t pte_get_entry(struct pcb_t *caller, addr_t pgn)
{
  struct krnl_t *krnl = caller->krnl;
  
  /* Check bounds if necessary, otherwise just return the entry */
  if (pgn >= PAGING_MAX_PGN) 
      return 0; // Invalid PGN

  return krnl->mm->pgd[pgn];
}

/* Set PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry
 **/
int pte_set_entry(struct pcb_t *caller, addr_t pgn, uint32_t pte_val)
{
  struct krnl_t *krnl = caller->krnl;
  
  if (pgn >= PAGING_MAX_PGN) 
      return -1;

  krnl->mm->pgd[pgn] = pte_val;
  
  return 0;
}

/*
 * vmap_pgd_memset - set a range of page entries to a specific value (usually 0)
 * @caller: process call
 * @addr: start address which is aligned to pagesz
 * @pgnum: num of mapping page
 */
int vmap_pgd_memset(struct pcb_t *caller, addr_t addr, int pgnum)
{
    struct krnl_t *krnl = caller->krnl;
    int pgn = PAGING_PGN(addr);
    int i;

    /* Loop through the page range and clear/set the Page Table Entries */
    for (i = 0; i < pgnum; i++) {
        /* In 32-bit, pgd is a flat array. We simply clear the entry.
         * This effectively marks the page as Not Present.
         */
        krnl->mm->pgd[pgn + i] = 0;
    }

    return 0;
}

/*
 * vmap_page_range - map a range of page at aligned address
 */
addr_t vmap_page_range(struct pcb_t *caller, 
                       addr_t addr, 
                       int pgnum, 
                       struct framephy_struct *frames, 
                       struct vm_rg_struct *ret_rg)
{
    struct krnl_t *krnl = caller->krnl;
    struct framephy_struct *fpit = frames;
    int pgn = PAGING_PGN(addr);
    int i;

    /* Update the region info if the pointer is valid */
    if (ret_rg != NULL) {
        ret_rg->rg_start = addr;
        ret_rg->rg_end = addr + (pgnum * PAGING_PAGESZ);
    }

    /* Map the list of frames to the virtual pages */
    for (i = 0; i < pgnum; i++) {
        if (fpit == NULL) {
            /* Ran out of frames before finishing the mapping? 
             * This shouldn't happen if alloc_pages_range worked correctly. */
            break;
        }

        int fpn = fpit->fpn;

        /* Update Page Table: Set Present bit and Frame Number 
         * Note: pte_set_fpn takes the caller (pcb_t*) and handles the 
         * kernel access internally, as defined in previous steps.
         */
        pte_set_fpn(caller, pgn + i, fpn);

        /* Add this page to the FIFO queue for future victim selection 
         * accessing the FIFO list via the kernel structure.
         */
        enlist_pgn_node(&krnl->mm->fifo_pgn, pgn + i);

        /* Move to the next frame in the list */
        fpit = fpit->fp_next;
    }

    return 0;
}

/*
 * alloc_pages_range - allocate req_pgnum of frame in ram
 * @caller    : caller
 * @req_pgnum : request page num
 * @frm_lst   : frame list
 */
addr_t alloc_pages_range(struct pcb_t *caller, int req_pgnum, struct framephy_struct **frm_lst)
{
    int pgit; 
    addr_t fpn;
    struct framephy_struct *newfp_str;
    struct krnl_t *krnl = caller->krnl;

    /* Initialize frame list to NULL */
    *frm_lst = NULL;
    struct framephy_struct *tail = NULL;

    for (pgit = 0; pgit < req_pgnum; pgit++)
    {
        newfp_str = malloc(sizeof(struct framephy_struct));
        
        /* Try to get a free frame from RAM via Kernel */
        if (MEMPHY_get_freefp(krnl->mram, &fpn) == 0)
        {
            newfp_str->fpn = fpn;
        }
        else
        { 
            /* RAM FULL: Perform Page Replacement (Swapping) */
            addr_t vicpgn;
            addr_t swpfpn;
            
            /* 1. Find victim page using FIFO via Kernel MM */
            if (find_victim_page(krnl->mm, &vicpgn) < 0) {
                free(newfp_str);
                return (addr_t)-3000; // OOM: No victim found
            }

            /* 2. Get a free frame from SWAP device via Kernel */
            if (MEMPHY_get_freefp(krnl->active_mswp, &swpfpn) < 0) {
                free(newfp_str);
                return (addr_t)-3000; // OOM: Swap is full
            }

            /* 3. Get the victim's current frame from PTE */
            // Note: In 32-bit mode, pgd is the flat page table
            uint32_t vicpte = krnl->mm->pgd[vicpgn];
            addr_t vicfpn = PAGING_PTE_FPN(vicpte);

            /* 4. Backing Store: Copy data from RAM (Victim) to SWAP */
            __swap_cp_page(krnl->mram, vicfpn, krnl->active_mswp, swpfpn);

            /* 5. Update Victim's PTE: Mark as swapped */
            // Note: pte_set_swap handles kernel access internally if using my previous code,
            // or we pass caller which contains the kernel pointer.
            pte_set_swap(caller, vicpgn, 0, swpfpn);

            /* 6. Steal the victim's RAM frame for the new allocation */
            newfp_str->fpn = vicfpn;
        }

        /* Link the new frame into the return list */
        newfp_str->owner = krnl->mm;
        newfp_str->fp_next = NULL;

        if (*frm_lst == NULL) {
            *frm_lst = newfp_str;
            tail = newfp_str;
        } else {
            tail->fp_next = newfp_str;
            tail = newfp_str;
        }
    }

    return 0;
}

/*
 * vm_map_ram - do the mapping all vm are to ram storage device
 * @caller    : caller
 * @astart    : vm area start
 * @aend      : vm area end
 * @mapstart  : start mapping point
 * @incpgnum  : number of mapped page
 * @ret_rg    : returned region
 */
addr_t vm_map_ram(struct pcb_t *caller, addr_t astart, addr_t aend, addr_t mapstart, int incpgnum, struct vm_rg_struct *ret_rg)
{
    struct framephy_struct *frm_lst = NULL;
    addr_t ret_alloc;

    /* 1. Allocate physical frames (handling swapping if necessary) */
    ret_alloc = alloc_pages_range(caller, incpgnum, &frm_lst);

    if (ret_alloc != 0) // Error occurred (e.g. OOM)
        return (addr_t)-1;

    /* 2. Map the allocated frames to the virtual pages */
    /* Note: vmap_page_range updates the Page Table */
    vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg);

    return 0;
}

/* Swap copy content page from source frame to destination frame
 * @mpsrc  : source memphy
 * @srcfpn : source physical page number (FPN)
 * @mpdst  : destination memphy
 * @dstfpn : destination physical page number (FPN)
 **/
int __swap_cp_page(struct memphy_struct *mpsrc, addr_t srcfpn,
                   struct memphy_struct *mpdst, addr_t dstfpn)
{
    int cellidx;
    int addrsrc, addrdst;

    /* Bounds guard so a bad frame number cannot walk past device storage */
    if (mpsrc == NULL || mpdst == NULL)
        return -1;

    int src_base = srcfpn * PAGING_PAGESZ;
    int dst_base = dstfpn * PAGING_PAGESZ;

    if (src_base < 0 || dst_base < 0)
        return -1;

    if (src_base + PAGING_PAGESZ > mpsrc->maxsz)
        return -1;

    if (dst_base + PAGING_PAGESZ > mpdst->maxsz)
        return -1;
    
    /* Iterate through every byte in the page */
    for (cellidx = 0; cellidx < PAGING_PAGESZ; cellidx++)
    {
        /* Calculate physical byte addresses */
        addrsrc = src_base + cellidx;
        addrdst = dst_base + cellidx;

        BYTE data;
        
        /* Read from Source Device */
        if (MEMPHY_read(mpsrc, addrsrc, &data) != 0)
            return -1;
        
        /* Write to Destination Device */
        if (MEMPHY_write(mpdst, addrdst, data) != 0)
            return -1;
    }

    return 0;
}

/*
 * Initialize a empty Memory Management instance
 * @mm:     self mm
 * @caller: mm owner
 */
int init_mm(struct mm_struct *mm, struct pcb_t *caller)
{
  struct vm_area_struct *vma0 = malloc(sizeof(struct vm_area_struct));

  /* Allocate the Page Global Directory (Page Table) */
  mm->pgd = malloc(PAGING_MAX_PGN * sizeof(uint32_t));

  /* Important: Clear the PGD to ensure all pages are initially Invalid/Not Present */
  for (int i = 0; i < PAGING_MAX_PGN; i++) {
      mm->pgd[i] = 0;
  }

  /* Initialize the Symbol Region Table */
  for (int i = 0; i < PAGING_MAX_SYMTBL_SZ; i++) {
      mm->symrgtbl[i].rg_start = 0;
      mm->symrgtbl[i].rg_end = 0;
      mm->symrgtbl[i].rg_next = NULL;
  }

  /* By default the owner comes with at least one vma (VMA0) */
  vma0->vm_id = 0;
  vma0->vm_start = 0;
  vma0->vm_end = vma0->vm_start;
  vma0->sbrk = vma0->vm_start;
  
  /* Initialize the first free region node */
  struct vm_rg_struct *first_rg = init_vm_rg(vma0->vm_start, vma0->vm_end);
  
  /* Initialize the free region list */
  vma0->vm_freerg_list = NULL;
  enlist_vm_rg_node(&vma0->vm_freerg_list, first_rg);

  /* Link VMA to MM */
  vma0->vm_next = NULL;
  vma0->vm_mm = mm;    /* Point vma owner backward */
  mm->mmap = vma0;     /* Update mmap */

  /* Initialize FIFO list for page replacement */
  mm->fifo_pgn = NULL;

  return 0;
}

/*
 * init_vm_rg - Initialize a virtual memory region
 * @rg_start: start address of the region
 * @rg_end: end address of the region
 */
struct vm_rg_struct *init_vm_rg(addr_t rg_start, addr_t rg_end)
{
  struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));

  rgnode->rg_start = rg_start;
  rgnode->rg_end = rg_end;
  rgnode->rg_next = NULL;

  return rgnode;
}

/*
 * enlist_vm_rg_node - Insert a region node into the list
 * @rglist: pointer to the head of the region list
 * @rgnode: the node to insert
 */
int enlist_vm_rg_node(struct vm_rg_struct **rglist, struct vm_rg_struct *rgnode)
{
    rgnode->rg_next = *rglist;
    *rglist = rgnode;

    return 0;
}

/*
 * enlist_pgn_node - Create and insert a page number node into the list
 * @plist: pointer to the head of the page number list (e.g., fifo_pgn)
 * @pgn: the page number to store
 */
int enlist_pgn_node(struct pgn_t **plist, addr_t pgn)
{
    struct pgn_t *pnode = malloc(sizeof(struct pgn_t));

    pnode->pgn = pgn;
    pnode->pg_next = *plist;
    *plist = pnode;

    return 0;
}

/*
 * print_list_fp - Print the list of frames
 * @ifp: head of the frame list
 */
int print_list_fp(struct framephy_struct *ifp)
{
    struct framephy_struct *fp = ifp;

    printf("print_list_fp: ");
    if (fp == NULL) { printf("NULL list\n"); return -1; }
    printf("\n");
    while (fp != NULL)
    {
        printf("fp[" FORMAT_ADDR "]\n", fp->fpn);
        fp = fp->fp_next;
    }
    printf("\n");
    return 0;
}

/*
 * print_list_rg - Print the list of regions
 * @irg: head of the region list
 */
int print_list_rg(struct vm_rg_struct *irg)
{
    struct vm_rg_struct *rg = irg;

    printf("print_list_rg: ");
    if (rg == NULL) { printf("NULL list\n"); return -1; }
    printf("\n");
    while (rg != NULL)
    {
        printf("rg[" FORMAT_ADDR "->" FORMAT_ADDR "]\n", rg->rg_start, rg->rg_end);
        rg = rg->rg_next;
    }
    printf("\n");
    return 0;
}

/*
 * print_list_vma - Print the list of VM areas
 * @ivma: head of the VMA list
 */
int print_list_vma(struct vm_area_struct *ivma)
{
    struct vm_area_struct *vma = ivma;

    printf("print_list_vma: ");
    if (vma == NULL) { printf("NULL list\n"); return -1; }
    printf("\n");
    while (vma != NULL)
    {
        printf("va[" FORMAT_ADDR "->" FORMAT_ADDR "]\n", vma->vm_start, vma->vm_end);
        vma = vma->vm_next;
    }
    printf("\n");
    return 0;
}

/*
 * print_list_pgn - Print the list of page numbers
 * @ip: head of the page number list
 */
int print_list_pgn(struct pgn_t *ip)
{
    printf("print_list_pgn: ");
    if (ip == NULL) { printf("NULL list\n"); return -1; }
    printf("\n");
    while (ip != NULL)
    {
        printf("pgn[" FORMAT_ADDR "]\n", ip->pgn);
        ip = ip->pg_next;
    }
    printf("\n");
    return 0;
}

/*
 * print_pgtbl - Print the page table content for a range
 * @caller: the process to print for
 * @start: virtual address to start from
 * @end: virtual address to end at (or -1 for end of VMA)
 */
int print_pgtbl(struct pcb_t *caller, uint32_t start, uint32_t end)
{
    int pgn_start, pgn_end;
    int pgit;
    struct krnl_t *krnl = caller->krnl;

    if (caller == NULL) { printf("NULL caller\n"); return -1; }

    /* If end is -1, print until the end of the first VMA */
    if (end == -1)
    {
        struct vm_area_struct *cur_vma = get_vma_by_num(krnl->mm, 0);
        if (cur_vma == NULL) return -1;
        end = cur_vma->vm_end;
    }

    pgn_start = PAGING_PGN(start);
    pgn_end = PAGING_PGN(end);

    printf("print_pgtbl: %d - %d\n", start, end);

    for (pgit = pgn_start; pgit < pgn_end; pgit++)
    {
        /* Print only if the page table entry is valid or has content */
        if (krnl->mm->pgd[pgit] != 0) {
            printf("%08ld: %08x\n", pgit * sizeof(uint32_t), krnl->mm->pgd[pgit]);
        }
    }

    return 0;
}


/*
 * pgread - Paging-based READ wrapper
 * @proc: process executing the instruction
 * @source: index of source register (which points to the segment/region)
 * @offset: offset to add to base address
 * @destination: index of destination register (where we store the read byte)
 */
int pgread(struct pcb_t * proc, uint32_t source, addr_t offset, uint32_t destination) 
{
  BYTE data;
  
  // __read uses the 'source' as the Region ID to find the base address
  int val = __read(proc, 0, source, offset, &data);

  if (val == 0) {
      // Store the byte read into the destination register
      // We cast to addr_t to match the register type
      proc->regs[destination] = (addr_t)data; 
  }

  return val;
}


/* VM prototypes */
/*
 * pgwrite - Paging-based WRITE wrapper
 * @proc: process executing the instruction
 * @data: data byte to write to memory
 * @destination: index of destination register (which points to the segment/region)
 * @offset: offset to add to base address
 */
int pgwrite(struct pcb_t * proc, BYTE data, uint32_t destination, addr_t offset)
{
  // __write uses 'destination' as the Region ID to find the base address
  return __write(proc, 0, destination, offset, data);
}

#endif //ndef MM64
