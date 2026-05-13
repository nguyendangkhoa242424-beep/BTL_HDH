/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Caitoa release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */
 
 /* NOTICE this moudle is deprecated in Caitoa release
  *        the structure is maintained for future 64bit-32bit
  *        backward compatible feature or PAE feature 
  */
 
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
  *pte = 0;
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
      CLRBIT(*pte, PAGING_PTE_PRESENT_MASK);
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
    addr_t pgn;

    pgn = PAGING_PGN(addr);

    return get_pd_from_pagenum(
        pgn,
        pgd,
        p4d,
        pud,
        pmd,
        pt
    );
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
    if (pgd) *pgd = 0;
    if (p4d) *p4d = 0;
    if (pud) *pud = 0;
    if (pmd) *pmd = 0;

    /*
     * Entire page number acts as page-table index
     */
    if (pt) *pt = pgn;

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
  addr_t *pte = &krnl->mm->pgd[pgn];
	
  CLRBIT(*pte, PAGING_PTE_PRESENT_MASK);
  SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
  SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

  return 0;
}

/*
 * pte_set_swap - Set PTE entry for on-line page
 * @pte   : target page table entry (PTE)
 * @fpn   : frame page number (FPN)
 */
int pte_set_fpn(struct pcb_t *caller, addr_t pgn, addr_t fpn)
{
  struct krnl_t *krnl = caller->krnl;
  addr_t *pte = &krnl->mm->pgd[pgn];

  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);

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
    struct krnl_t *krnl;

    if (caller == NULL || caller->krnl == NULL)
        return 0;

    krnl = caller->krnl;

    return (uint32_t) krnl->mm->pgd[pgn];
}

/* Set PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry
 **/
int pte_set_entry(struct pcb_t *caller, addr_t pgn, uint32_t pte_val)
{
    if (caller == NULL || caller->krnl == NULL)
    return -1;
    struct krnl_t *krnl = caller->krnl;
    krnl->mm->pgd[pgn] = pte_val;

    return 0;
}

/*
 * vmap_pgd_memset - map a range of page at aligned address
 */
int vmap_pgd_memset(struct pcb_t *caller,           // process call
                    addr_t addr,                       // start address which is aligned to pagesz
                    int pgnum)                      // num of mapping page
{
    struct krnl_t *krnl = caller->krnl;

    addr_t pgn_start;
    int i;

    pgn_start = PAGING_PGN(addr);

    for (i = 0; i < pgnum; i++) {

        addr_t pgn = pgn_start + i;
        if (pgn >= PAGING_MAX_PGN)
            break;
        /*
         * Reset PTE entry
         */
        krnl->mm->pgd[pgn] = 0;
    }

    return 0;
}

/*
 * vmap_page_range - map a range of page at aligned address
 */
addr_t vmap_page_range(struct pcb_t *caller,           // process call
                    addr_t addr,                       // start address which is aligned to pagesz
                    int pgnum,                      // num of mapping page
                    struct framephy_struct *frames, // list of the mapped frames
                    struct vm_rg_struct *ret_rg)    // return mapped region, the real mapped fp
{                                                   // no guarantee all given pages are mapped
    addr_t pgn_start;
    int i;

    struct framephy_struct *fpit;

    pgn_start = PAGING_PGN(addr);

    fpit = frames;

    for (i = 0; i < pgnum; i++) {

        if (fpit == NULL)
            break;

        pte_set_fpn(caller,
                    pgn_start + i,
                    fpit->fpn);
        enlist_pgn_node(
    &caller->krnl->mm->fifo_pgn,
    pgn_start + i);
        fpit = fpit->fp_next;
    }

    if (ret_rg != NULL) {
        ret_rg->rg_start = addr;
        ret_rg->rg_end =
            addr + i * PAGING_PAGESZ;
    }

    return addr;
}

/*
 * alloc_pages_range - allocate req_pgnum of frame in ram
 * @caller    : caller
 * @req_pgnum : request page num
 * @frm_lst   : frame list
 */

addr_t alloc_pages_range(struct pcb_t *caller, int req_pgnum, struct framephy_struct **frm_lst)
{
    int i;
    int ret;
    int fpn;

    struct framephy_struct *head = NULL;
    struct framephy_struct *tail = NULL;

    for (i = 0; i < req_pgnum; i++) {

        ret = MEMPHY_get_freefp(
    caller->mram,
    &fpn);

        if (ret != 0){
            if (i == 0)
        return -3000;

    goto cleanup;
        }
        struct framephy_struct *newfp =
            malloc(sizeof(struct framephy_struct));

        if (newfp == NULL)
            goto cleanup;

        newfp->fpn = fpn;
        newfp->fp_next = NULL;

        if (head == NULL) {
            head = tail = newfp;
        } else {
            tail->fp_next = newfp;
            tail = newfp;
        }
    }

    *frm_lst = head;

    return 0;

cleanup:
    while (head != NULL) {
        struct framephy_struct *tmp = head;

        MEMPHY_put_freefp(
            caller->mram,
            head->fpn);

        head = head->fp_next;

        free(tmp);
    }

    return -1;
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
    struct framephy_struct *tmp;

    int ret;

ret = alloc_pages_range(
          caller,
          incpgnum,
          &frm_lst);

if (ret == -3000)
    return -3000;

if (ret < 0)
    return -1;

    ret = vmap_page_range(
              caller,
              mapstart,
              incpgnum,
              frm_lst,
              ret_rg);

    while (frm_lst != NULL) {
        tmp = frm_lst;
        frm_lst = frm_lst->fp_next;
        free(tmp);
    }

    if (ret < 0)
        return -1;

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
    int i;
    BYTE data;

    addr_t srcaddr = srcfpn * PAGING_PAGESZ;
    addr_t dstaddr = dstfpn * PAGING_PAGESZ;

    for (i = 0; i < PAGING_PAGESZ; i++) {

        MEMPHY_read(
            mpsrc,
            srcaddr + i,
            &data);

        MEMPHY_write(
            mpdst,
            dstaddr + i,
            data);
    }

    return 0;
}

/*
 *Initialize a empty Memory Management instance
 * @mm:     self mm
 * @caller: mm owner
 */
int init_mm(struct mm_struct *mm, struct pcb_t *caller)
{
    int i;

    struct vm_area_struct *vma0;
    struct vm_rg_struct *first_rg;

    mm->pgd = calloc(PAGING_MAX_PGN, sizeof(uint32_t));
    if (mm->pgd == NULL)
    return -1;

    /*
     * Initialize page table entries
     */
    for (i = 0; i < PAGING_MAX_PGN; i++) {
        mm->pgd[i] = 0;
    }

    /*
     * FIFO page replacement queue
     */
    mm->fifo_pgn = NULL;

    /*
     * Initialize symbol region table
     */
    for (i = 0; i < PAGING_MAX_SYMTBL_SZ; i++) {
        mm->symrgtbl[i].rg_start = 0;
        mm->symrgtbl[i].rg_end = 0;
        mm->symrgtbl[i].rg_next = NULL;
        mm->symrgtbl[i].vmaid = -1;
    }

    /*
     * Create default VMA
     */
    vma0 = malloc(sizeof(struct vm_area_struct));

    if (vma0 == NULL)
        return -1;

    vma0->vm_id = 0;

    vma0->vm_start = 0;
    vma0->vm_end   = 0;
    vma0->sbrk     = 0;

    vma0->vm_next = NULL;
    vma0->vm_mm   = mm;

    vma0->vm_freerg_list = NULL;

    first_rg = init_vm_rg(
        vma0->vm_start,
        vma0->vm_end
    );

    if (first_rg != NULL) {
        enlist_vm_rg_node(
            &vma0->vm_freerg_list,
            first_rg
        );
    }

    /*
     * Attach mmap list
     */
    mm->mmap = vma0;

    return 0;
}

struct vm_rg_struct *init_vm_rg(addr_t rg_start, addr_t rg_end)
{
    struct vm_rg_struct *rgnode;

    rgnode = malloc(sizeof(struct vm_rg_struct));

    if (rgnode == NULL)
        return NULL;

    rgnode->vmaid = -1;
    rgnode->rg_start = rg_start;
    rgnode->rg_end = rg_end;
    rgnode->rg_next = NULL;

    return rgnode;
}

int enlist_vm_rg_node(struct vm_rg_struct **rglist, struct vm_rg_struct *rgnode)
{
    if (rglist == NULL || rgnode == NULL)
        return -1;

    rgnode->rg_next = *rglist;
    *rglist = rgnode;

    return 0;
}

int enlist_pgn_node(struct pgn_t **plist, addr_t pgn)
{
    struct pgn_t *node;

    node = malloc(sizeof(struct pgn_t));

    if (node == NULL)
        return -1;

    node->pgn = pgn;
    node->pg_next = NULL;

    if (*plist == NULL) {
        *plist = node;
    } else {
        struct pgn_t *tmp = *plist;

        while (tmp->pg_next != NULL)
            tmp = tmp->pg_next;

        tmp->pg_next = node;
    }

    return 0;
}

int print_list_fp(struct framephy_struct *ifp)
{
    while (ifp != NULL) {
        printf(FORMAT_ADDR " ", ifp->fpn);
        ifp = ifp->fp_next;
    }

    printf("\n");

    return 0;
}

int print_list_rg(struct vm_rg_struct *irg)
{
    struct vm_rg_struct *rg = irg;

    while (rg != NULL) {

printf("RG[%d]: " FORMAT_ADDR " -> " FORMAT_ADDR "\n",
       rg->vmaid,
       rg->rg_start,
       rg->rg_end);
        rg = rg->rg_next;
    }

    return 0;
}

int print_list_vma(struct vm_area_struct *ivma)
{
    struct vm_area_struct *vma = ivma;

    while (vma != NULL) {

        printf("VMA[%lu]: " FORMAT_ADDR " -> " FORMAT_ADDR "\n",
               vma->vm_id,
               vma->vm_start,
               vma->vm_end);

        vma = vma->vm_next;
    }

    return 0;
}

int print_list_pgn(struct pgn_t *ip)
{
    while (ip != NULL) {
        printf(FORMAT_ADDR " ", ip->pgn);
        ip = ip->pg_next;
    }

    printf("\n");

    return 0;
}

int print_pgtbl(struct pcb_t *caller, uint32_t start, uint32_t end)
{
    struct krnl_t *krnl = caller->krnl;

    addr_t pgn_start;
    addr_t pgn_end;
    addr_t pgn;

    pgn_start = PAGING_PGN(start);
    pgn_end   = PAGING_PGN(end);

    if (pgn_end >= PAGING_MAX_PGN)
        pgn_end = PAGING_MAX_PGN - 1;

    printf("===== PAGE TABLE =====\n");

    for (pgn = pgn_start; pgn <= pgn_end; pgn++)
    {
        uint32_t pte = krnl->mm->pgd[pgn];

        printf("PGN %-5u : PTE = 0x%08x ",
               (uint32_t)pgn,
               pte);

        if (pte & PAGING_PTE_PRESENT_MASK)
        {
            addr_t fpn;

            fpn = GETVAL(
                pte,
                PAGING_PTE_FPN_MASK,
                PAGING_PTE_FPN_LOBIT
            );

            printf("[PRESENT] FPN=%u",
                   (uint32_t)fpn);
        }
        else if (pte & PAGING_PTE_SWAPPED_MASK)
        {
            addr_t swptype;
            addr_t swpoff;

            swptype = GETVAL(
                pte,
                PAGING_PTE_SWPTYP_MASK,
                PAGING_PTE_SWPTYP_LOBIT
            );

            swpoff = GETVAL(
                pte,
                PAGING_PTE_SWPOFF_MASK,
                PAGING_PTE_SWPOFF_LOBIT
            );

            printf("[SWAPPED] TYPE=%u OFF=%u",
                   (uint32_t)swptype,
                   (uint32_t)swpoff);
        }
        else
        {
            printf("[EMPTY]");
        }

        printf("\n");
    }

    printf("======================\n");

    return 0;
}

#endif //ndef MM64
