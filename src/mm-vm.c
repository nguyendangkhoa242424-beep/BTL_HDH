/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Caitoa release
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
#include "mm64.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

extern addr_t vm_map_ram(struct pcb_t *caller, addr_t astart, addr_t aend,
                         addr_t mapstart, int incpgnum,
                         struct vm_rg_struct *ret_rg);

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  struct vm_area_struct *pvma = mm->mmap;

  if (mm->mmap == NULL)
    return NULL;

  int vmait = pvma->vm_id;

  while (vmait < vmaid)
  {
    if (pvma == NULL)
      return NULL;

    pvma = pvma->vm_next;
    vmait = pvma->vm_id;
  }

  return pvma;
}

int __mm_swap_page(struct pcb_t *caller, addr_t vicfpn , addr_t swpfpn)
{
    __swap_cp_page(caller->krnl->mram, vicfpn, caller->krnl->active_mswp, swpfpn);
    return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, addr_t size, addr_t alignedsz)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);
  struct vm_rg_struct *newrg;

  if (cur_vma == NULL || alignedsz < size)
    return NULL;

  newrg = malloc(sizeof(struct vm_rg_struct));
  if (newrg == NULL)
    return NULL;

  newrg->vmaid = vmaid;
  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end = newrg->rg_start + alignedsz;
  newrg->rg_next = NULL;

  return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, addr_t vmastart, addr_t vmaend)
{
  if (vmastart >= vmaend)
    return -1;

  struct vm_area_struct *vma = caller->krnl->mm->mmap;
  struct vm_area_struct *cur_area = get_vma_by_num(caller->krnl->mm, vmaid);

  if (vma == NULL || cur_area == NULL)
    return -1;

  while (vma != NULL)
  {
    if (vma != cur_area &&
        OVERLAP(vmastart, vmaend, vma->vm_start, vma->vm_end))
      return -1;

    vma = vma->vm_next;
  }

  return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size
 *
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, addr_t inc_sz)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);
  struct vm_rg_struct *area;
  struct vm_rg_struct mapped_rg;
  addr_t aligned_sz;
  addr_t old_end;
  int incnumpage;

  if (cur_vma == NULL || inc_sz == 0)
    return -1;

#ifdef MM64
  aligned_sz = PAGING64_PAGE_ALIGNSZ(inc_sz);
  incnumpage = (int)(aligned_sz / PAGING64_PAGESZ);
#else
  aligned_sz = PAGING_PAGE_ALIGNSZ(inc_sz);
  incnumpage = (int)(aligned_sz / PAGING_PAGESZ);
#endif

  area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, aligned_sz);
  if (area == NULL)
    return -1;

  if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0)
  {
    free(area);
    return -1;
  }

  old_end = cur_vma->vm_end;

  if (vm_map_ram(caller, area->rg_start, area->rg_end, old_end, incnumpage, &mapped_rg) < 0)
  {
    free(area);
    return -1;
  }

  cur_vma->vm_end = area->rg_end;
  cur_vma->sbrk = area->rg_end;

  if (area->rg_start + inc_sz < area->rg_end)
  {
    struct vm_rg_struct *leftover = init_vm_rg(area->rg_start + inc_sz, area->rg_end);
    if (leftover != NULL)
    {
      leftover->vmaid = vmaid;
      enlist_vm_rg_node(&cur_vma->vm_freerg_list, leftover);
    }
  }

  free(area);

  return 0;
}

// #endif
