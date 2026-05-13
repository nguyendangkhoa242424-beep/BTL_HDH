/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

/*
 * PAGING based Memory Management
 * Memory management unit mm/mm.c
 */

#include "mm64.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#if defined(MM64)
static addr_t *page_walk(struct mm_struct *mm,
                         addr_t pgn,
                         int create)
{
    addr_t pgd_idx, p4d_idx, pud_idx, pmd_idx, pt_idx;

    get_pd_from_pagenum(
        pgn,
        &pgd_idx,
        &p4d_idx,
        &pud_idx,
        &pmd_idx,
        &pt_idx
    );

    if (mm->pgd->entries[pgd_idx] == NULL) {
        if (!create) return NULL;

        mm->pgd->entries[pgd_idx] =
            calloc(1, sizeof(p4d_t));
    }

    p4d_t *p4d = mm->pgd->entries[pgd_idx];

    if (p4d->entries[p4d_idx] == NULL) {
        if (!create) return NULL;

        p4d->entries[p4d_idx] =
            calloc(1, sizeof(pud_t));
    }

    pud_t *pud = p4d->entries[p4d_idx];

    if (pud->entries[pud_idx] == NULL) {
        if (!create) return NULL;

        pud->entries[pud_idx] =
            calloc(1, sizeof(pmd_t));
    }

    pmd_t *pmd = pud->entries[pud_idx];

    if (pmd->entries[pmd_idx] == NULL) {
        if (!create) return NULL;

        pmd->entries[pmd_idx] =
            calloc(1, sizeof(pt_t));
    }

    pt_t *pt = pmd->entries[pmd_idx];

    return &pt->entries[pt_idx];
}
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
	/* Extract page direactories */
	*pgd = (addr&PAGING64_ADDR_PGD_MASK)>>PAGING64_ADDR_PGD_LOBIT;
	*p4d = (addr&PAGING64_ADDR_P4D_MASK)>>PAGING64_ADDR_P4D_LOBIT;
	*pud = (addr&PAGING64_ADDR_PUD_MASK)>>PAGING64_ADDR_PUD_LOBIT;
	*pmd = (addr&PAGING64_ADDR_PMD_MASK)>>PAGING64_ADDR_PMD_LOBIT;
	*pt = (addr&PAGING64_ADDR_PT_MASK)>>PAGING64_ADDR_PT_LOBIT;

	/* TODO: implement the page direactories mapping */

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
	/* Shift the address to get page num and perform the mapping*/
	return get_pd_from_address(pgn << PAGING64_ADDR_PT_SHIFT,
                         pgd,p4d,pud,pmd,pt);
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
    addr_t *pte;

#ifdef MM64
    pte = page_walk(krnl->mm, pgn, 1);

    if (pte == NULL)
        return -1;
#else
    pte = &krnl->mm->pgd[pgn];
#endif
	
  CLRBIT(*pte, PAGING_PTE_PRESENT_MASK);
  SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
  SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

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

    addr_t *pte;

#ifdef MM64
    pte = page_walk(krnl->mm, pgn, 1);

    if (pte == NULL)
        return -1;
#else
    pte = &krnl->mm->pgd[pgn];
#endif

    SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
    CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);

    SETVAL(*pte,
           fpn,
           PAGING_PTE_FPN_MASK,
           PAGING_PTE_FPN_LOBIT);

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

#ifdef MM64
    addr_t *pte;

    pte = page_walk(krnl->mm, pgn, 0);

    if (pte == NULL)
        return 0;

    return (uint32_t)(*pte);
#else
    return (uint32_t) krnl->mm->pgd[pgn];
#endif
}

/* Set PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry
 **/
int pte_set_entry(struct pcb_t *caller, addr_t pgn, uint32_t pte_val)
{
	struct krnl_t *krnl = caller->krnl;
	#ifdef MM64
    addr_t *pte;

    pte = page_walk(krnl->mm, pgn, 1);

    if (pte == NULL)
        return -1;

    *pte = pte_val;
#else
    krnl->mm->pgd[pgn] = pte_val;
#endif
	
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

    addr_t pgn_start = PAGING_PGN(addr);

    int i;

for (i = 0; i < pgnum; i++) {
    pte_set_entry(
        caller,
        pgn_start + i,
        0
    );
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

        pte_set_fpn(
            caller,
            pgn_start + i,
            fpit->fpn
        );

        enlist_pgn_node(
            &caller->krnl->mm->fifo_pgn,
            pgn_start + i
        );

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
    addr_t fpn;

    struct framephy_struct *head = NULL;
    struct framephy_struct *tail = NULL;

    for (i = 0; i < req_pgnum; i++) {

        ret = MEMPHY_get_freefp(caller->krnl->mram, &fpn);

    if (ret != 0) {
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
            caller->krnl->mram,
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
  addr_t ret_alloc = 0;
//int pgnum = incpgnum;

  /*@bksysnet: author provides a feasible solution of getting frames
   *FATAL logic in here, wrong behaviour if we have not enough page
   *i.e. we request 1000 frames meanwhile our RAM has size of 3 frames
   *Don't try to perform that case in this simple work, it will result
   *in endless procedure of swap-off to get frame and we have not provide
   *duplicate control mechanism, keep it simple
   */
  ret_alloc = alloc_pages_range(
    caller,
    incpgnum,
    &frm_lst
);

  if (ret_alloc < 0 && ret_alloc != -3000)
    return -1;

  /* Out of memory */
  if (ret_alloc == -3000)
  {
    return -1;
  }

  /* it leaves the case of memory is enough but half in ram, half in swap
   * do the swaping all to swapper to get the all in ram */
   vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg);
  struct framephy_struct *tmp;

  while (frm_lst != NULL) {
      tmp = frm_lst;
      frm_lst = frm_lst->fp_next;
      free(tmp);
  }
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
  addr_t addrsrc, addrdst;
  for (cellidx = 0; cellidx < PAGING_PAGESZ; cellidx++)
  {
    addrsrc = srcfpn * PAGING_PAGESZ + cellidx;
    addrdst = dstfpn * PAGING_PAGESZ + cellidx;

    BYTE data;
    MEMPHY_read(mpsrc, addrsrc, &data);
    MEMPHY_write(mpdst, addrdst, data);
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
  
  struct vm_area_struct *vma0 = malloc(sizeof(struct vm_area_struct));
  if (vma0 == NULL)
    return -1;
  /* TODO init page table directory */
   //mm->pgd = ...
   //mm->p4d = ...
   //mm->pud = ...
   //mm->pmd = ...
   //mm->pt = ...

    int i;
    mm->pgd = calloc(1, sizeof(pgd_t));

    if (mm->pgd == NULL) {
        return -1;
    }


    mm->fifo_pgn = NULL;
  /* By default the owner comes with at least one vma */
  vma0->vm_id = 0;
  vma0->vm_start = 0;
  vma0->vm_end = vma0->vm_start;
  vma0->sbrk = vma0->vm_start;

  struct vm_rg_struct *first_rg = init_vm_rg(vma0->vm_start, vma0->vm_end);
  if (first_rg == NULL) {
    free(vma0);
    return -1;
}
  vma0->vm_freerg_list = NULL;
  enlist_vm_rg_node(&vma0->vm_freerg_list, first_rg);

  vma0->vm_next = NULL;
  vma0->vm_mm = mm;

  mm->mmap = vma0;

      for (i = 0; i < PAGING_MAX_SYMTBL_SZ; i++) {
          mm->symrgtbl[i].rg_start = 0;
          mm->symrgtbl[i].rg_end = 0;
          mm->symrgtbl[i].rg_next = NULL;
          mm->symrgtbl[i].vmaid = -1;
      }

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
  struct framephy_struct *fp = ifp;

  printf("print_list_fp: ");
  if (fp == NULL) { printf("NULL list\n"); return -1;}
  printf("\n");
  while (fp != NULL)
  {
    printf("fp[" FORMAT_ADDR "]\n", fp->fpn);
    fp = fp->fp_next;
  }
  printf("\n");
  return 0;
}

int print_list_rg(struct vm_rg_struct *irg)
{
  struct vm_rg_struct *rg = irg;

  printf("print_list_rg: ");
  if (rg == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (rg != NULL)
  {
    printf("rg[" FORMAT_ADDR "->"  FORMAT_ADDR "]\n", rg->rg_start, rg->rg_end);
    rg = rg->rg_next;
  }
  printf("\n");
  return 0;
}

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

int print_list_pgn(struct pgn_t *ip)
{
  printf("print_list_pgn: ");
  if (ip == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (ip != NULL)
  {
    printf("va[" FORMAT_ADDR "]-\n", ip->pgn);
    ip = ip->pg_next;
  }
  printf("\n");
  return 0;
}

int print_pgtbl(struct pcb_t *caller, addr_t start, addr_t end)
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
        uint32_t pte = pte_get_entry(caller, pgn);

        printf("PGN %-5u : PTE = 0x%08x ",
               (uint32_t)pgn,
               pte);

        if ((pte & PAGING_PTE_PRESENT_MASK))
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
        else if ((pte & PAGING_PTE_SWAPPED_MASK))
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

#endif  //def MM64
