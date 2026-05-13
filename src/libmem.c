/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Caitoa release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

// #ifdef MM_PAGING
/*
 * System Library
 * Memory Module Library libmem.c
 */

#include "string.h"
#include "mm.h"
#include "mm64.h"
#include "syscall.h"
#include "libmem.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
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
  if (rgid < 0 || rgid >= PAGING_MAX_SYMTBL_SZ)
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
  if (caller == NULL || caller->krnl == NULL || caller->krnl->mm == NULL ||
      alloc_addr == NULL || size == 0 ||
      rgid < 0 || rgid >= PAGING_MAX_SYMTBL_SZ)
  {
    return -1;
  }

  pthread_mutex_lock(&mmvm_lock);

  struct vm_rg_struct rgnode;
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);

  if (cur_vma == NULL)
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  /* First try to reuse an existing free virtual-memory region. */
  if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
  {
    caller->krnl->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
    caller->krnl->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
    caller->krnl->mm->symrgtbl[rgid].rg_next = NULL;

    *alloc_addr = rgnode.rg_start;

    pthread_mutex_unlock(&mmvm_lock);
    return 0;
  }

  /* No free region is large enough, so extend the VMA limit. */
  addr_t old_sbrk = cur_vma->sbrk;
  if (inc_vma_limit(caller, vmaid, size) < 0)
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  caller->krnl->mm->symrgtbl[rgid].rg_start = old_sbrk;
  caller->krnl->mm->symrgtbl[rgid].rg_end = old_sbrk + size;
  caller->krnl->mm->symrgtbl[rgid].rg_next = NULL;

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

  if (rgid < 0 || rgid >= PAGING_MAX_SYMTBL_SZ)
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
  struct vm_rg_struct *freerg_node = malloc(sizeof(struct vm_rg_struct));
  freerg_node->rg_start = rgnode->rg_start;
  freerg_node->rg_end = rgnode->rg_end;
  freerg_node->rg_next = NULL;

  rgnode->rg_start = rgnode->rg_end = 0;
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
  addr_t addr;
  int val = __alloc(proc, 0, reg_index, size, &addr);
  if (val == -1)
  {
    return -1;
  }

#ifdef IODUMP
  /* TODO dump IO content (if needed) */
  printf("%s:%d pid=%u pc=%u alloc reg=%u size=%lu addr=0x%lx\n",
         __func__,
         __LINE__,
         proc->pid,
         proc->pc,
         reg_index,
         size,
         (unsigned long)addr);

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
  int val = __free(proc, 0, reg_index);
  if (val == -1)
  {
    return -1;
  }

#ifdef IODUMP
  /* TODO dump IO content (if needed) */
  printf("%s:%d pid=%u pc=%u free reg=%u\n",
         __func__,
         __LINE__,
         proc->pid,
         proc->pc,
         reg_index);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
#endif
  return 0; // val;
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
    uint32_t pte = pte_get_entry(caller, pgn);

    if (!PAGING_PAGE_PRESENT(pte))
{
    addr_t vicpgn;
    addr_t swpfpn;
    addr_t vicfpn;
    uint32_t vicpte;
    addr_t tgtfpn;

    if (MEMPHY_get_freefp(caller->krnl->mram, &tgtfpn) == -1)
    {
        if (find_victim_page(caller->krnl->mm, &vicpgn) == -1)
            return -1;

        vicpte = pte_get_entry(caller, vicpgn);

        vicfpn = PAGING_FPN(vicpte);

        if (MEMPHY_get_freefp(
                caller->krnl->active_mswp,
                &swpfpn) == -1)
            return -1;

        __swap_cp_page(
            caller->krnl->mram,
            vicfpn,
            caller->krnl->active_mswp,
            swpfpn
        );

        pte_set_swap(
            caller,
            vicpgn,
            0,
            swpfpn
        );

        tgtfpn = vicfpn;
    }

    /*
     * CHỈ swap-in nếu page thực sự đang swapped
     */
    if (pte & PAGING_PTE_SWAPPED_MASK)
    {
        addr_t tgtswp = PAGING_SWP(pte);

        __swap_cp_page(
            caller->krnl->active_mswp,
            tgtswp,
            caller->krnl->mram,
            tgtfpn
        );
    }

    pte_set_fpn(
        caller,
        pgn,
        tgtfpn
    );

    enlist_pgn_node(
        &caller->krnl->mm->fifo_pgn,
        pgn
    );
}

    *fpn = PAGING_FPN(pte_get_entry(caller, pgn));
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
  if (mm == NULL || caller == NULL || caller->krnl == NULL)
    return -1;

  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr); // MAKE changes
  int fpn;

  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; /* invalid page access */

  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off; // uncomment

  /* TODO 
   *  MEMPHY_read(caller->krnl->mram, phyaddr, data);
   *  MEMPHY READ 
   *  SYSCALL 17 sys_memmap with SYSMEM_IO_READ
   */
  if (MEMPHY_read(caller->krnl->mram,
                  phyaddr,
                  data) != 0)
  {
    return -1;
  }

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
  if (mm == NULL || caller == NULL || caller->krnl == NULL)
    return -1;

  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed. */
  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; /* invalid page access */

  addr_t phyaddr = ((addr_t)fpn << PAGING_ADDR_FPN_LOBIT) + off;

  if (MEMPHY_write(caller->krnl->mram, phyaddr, value) != 0)
    return -1;

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
  if (caller == NULL || caller->krnl == NULL || caller->krnl->mm == NULL || data == NULL)
    return -1;

  struct vm_rg_struct *currg = get_symrg_byid(caller->krnl->mm, rgid);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);

  if (currg == NULL || cur_vma == NULL ||
      currg->rg_start == currg->rg_end ||
      currg->rg_start + offset >= currg->rg_end)
    return -1;

  return pg_getval(caller->krnl->mm, currg->rg_start + offset, data, caller);
}

/*libread - PAGING-based read a region memory */
int libread(
    struct pcb_t *proc, // Process executing the instruction
    uint32_t source,    // Index of source register
    addr_t offset,      // Source address = [source] + [offset]
    uint32_t *destination)
{
  BYTE data;
  int val = __read(proc, 0, source, offset, &data);
  if (val != 0)
    return -1;

  *destination = data;
#ifdef IODUMP
  /* TODO dump IO content (if needed) */
  printf("%s:%d pid=%u pc=%u read reg=%u offset=%lu data=%u dest=%u\n",
         __func__,
         __LINE__,
         proc->pid,
         proc->pc,
         source,
         offset,
         (unsigned int)data,
         *destination);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
#endif

  return 0;
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
  if (caller == NULL || caller->krnl == NULL || caller->krnl->mm == NULL)
    return -1;

  pthread_mutex_lock(&mmvm_lock);

  struct vm_rg_struct *currg = get_symrg_byid(caller->krnl->mm, rgid);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);

  if (currg == NULL || cur_vma == NULL ||
      currg->rg_start == currg->rg_end ||
      currg->rg_start + offset >= currg->rg_end) /* Invalid memory identify */
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  int ret = pg_setval(caller->krnl->mm, currg->rg_start + offset, value, caller);

  pthread_mutex_unlock(&mmvm_lock);
  return ret;
}

/*libwrite - PAGING-based write a region memory */
int libwrite(
    struct pcb_t *proc,   // Process executing the instruction
    BYTE data,            // Data to be wrttien into memory
    uint32_t destination, // Index of destination register
    addr_t offset)
{
  int val = __write(proc, 0, destination, offset, data);
  if (val != 0)
    return -1;

#ifdef IODUMP
  /* TODO dump IO content (if needed) */
  /*printf("%s:%d pid=%u pc=%u write data=%u reg=%u offset=%lu\n",
         __func__,
         __LINE__,
         proc->pid,
         proc->pc,
         (unsigned int)data,
         destination,
         offset);*/
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
#endif

  return 0;
}

/*libkmem_malloc- alloc region memory in kmem
 *@caller: caller
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: memory size
 */

int libkmem_malloc(struct pcb_t *caller, uint32_t size, uint32_t reg_index)
{
  addr_t addr;

  if (caller == NULL || size == 0)
    return -1;

  addr = __kmalloc(caller, 0, reg_index, size, &addr);
  if (addr == (addr_t)-1)
    return -1;

  caller->regs[reg_index] = addr;
  return 0;
}

/*kmalloc - alloc region memory in kmem
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: memory size
 *@alloc_addr: allocated address
 */
addr_t __kmalloc(struct pcb_t *caller, int vmaid, int rgid, addr_t size, addr_t *alloc_addr)
{
  if (caller == NULL || alloc_addr == NULL || size == 0)
    return (addr_t)-1;

  if (__alloc(caller, vmaid, rgid, size, alloc_addr) != 0)
    return (addr_t)-1;

  return *alloc_addr;
}

/*libkmem_cache_pool_create - create cache pool in kmem
 *@caller: caller
 *@size: memory size
 *@align: alignment size of each cache slot (identical cache slot size)
 *@cache_pool_id: cache pool ID
 */
int libkmem_cache_pool_create(struct pcb_t *caller, uint32_t size, uint32_t align, uint32_t cache_pool_id)
{
  /* Minimal validation. The provided skeleton does not expose a real kcpool table
   * layout here, so allocation still falls back to __kmalloc(). */
  if (caller == NULL || size == 0 || align == 0)
    return -1;

  (void)cache_pool_id;
  return 0;
}

/*libkmem_cache_alloc - allocate cache slot in cache pool, cache slot has identical size
 * the allocated size is embedded in pool management mechanism
 *@caller: caller
 *@cache_pool_id: cache pool ID
 *@reg_index: memory region index
 */
int libkmem_cache_alloc(struct pcb_t *proc, uint32_t cache_pool_id, uint32_t reg_index)
{
  addr_t addr;

  if (proc == NULL)
    return -1;

  addr = __kmem_cache_alloc(proc, 0, reg_index, cache_pool_id, &addr);
  if (addr == (addr_t)-1)
    return -1;

  proc->regs[reg_index] = addr;
  return 0;
}

/*kmem_cache_alloc - alloc region memory in kmem cache
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@cache_pool_id: cached pool ID
 *@alloc_addr: allocated address
 */

addr_t __kmem_cache_alloc(struct pcb_t *caller, int vmaid, int rgid, int cache_pool_id, addr_t *alloc_addr)
{
  if (caller == NULL || alloc_addr == NULL)
    return (addr_t)-1;

  (void)cache_pool_id;

#ifdef MM64
  return __kmalloc(caller, vmaid, rgid, PAGING64_PAGESZ, alloc_addr);
#else
  return __kmalloc(caller, vmaid, rgid, PAGING_PAGESZ, alloc_addr);
#endif
}

int libkmem_copy_from_user(struct pcb_t *caller, uint32_t source, uint32_t destination, uint32_t offset, uint32_t size)
{
  BYTE data;
  uint32_t i;

  if (caller == NULL)
    return -1;

  for (i = 0; i < size; i++)
  {
    if (__read_user_mem(caller, 0, source, offset + i, &data) != 0)
      return -1;

    if (__write_kernel_mem(caller, 0, destination, i, data) != 0)
      return -1;
  }

  return 0;
}

int libkmem_copy_to_user(struct pcb_t *caller, uint32_t source, uint32_t destination, uint32_t offset, uint32_t size)
{
  BYTE data;
  uint32_t i;

  if (caller == NULL)
    return -1;

  for (i = 0; i < size; i++)
  {
    if (__read_kernel_mem(caller, 0, source, i, &data) != 0)
      return -1;

    if (__write_user_mem(caller, 0, destination, offset + i, data) != 0)
      return -1;
  }

  return 0;
}

/*__read_kernel_mem - read value in kernel region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@offset: offset to acess in memory region
 *@value: data value
 */
int __read_kernel_mem(struct pcb_t *caller, int vmaid, int rgid, addr_t offset, BYTE *data)
{
  if (caller == NULL || data == NULL)
    return -1;

  /* The skeleton does not provide a separate public krnl_pgd API here, so this
   * wrapper uses the existing protected __read() path through caller->krnl. */
  return __read(caller, vmaid, rgid, offset, data);
}

/*__write_kernel_mem - write a kernel region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@offset: offset to acess in memory region
 *@value: data value
 */
int __write_kernel_mem(struct pcb_t *caller, int vmaid, int rgid, addr_t offset, BYTE value)
{
  if (caller == NULL)
    return -1;

  /* The skeleton does not provide a separate public krnl_pgd API here, so this
   * wrapper uses the existing protected __write() path through caller->krnl. */
  return __write(caller, vmaid, rgid, offset, value);
}

/*__read_user_mem - read value in user region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@offset: offset to acess in memory region
 *@value: data value
 */
int __read_user_mem(struct pcb_t *caller, int vmaid, int rgid, addr_t offset, BYTE *data)
{
  if (caller == NULL || data == NULL)
    return -1;

  return __read(caller, vmaid, rgid, offset, data);
}

/*__write_user_mem - write a user region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@offset: offset to acess in memory region
 *@value: data value
 */
int __write_user_mem(struct pcb_t *caller, int vmaid, int rgid, addr_t offset, BYTE value)
{
  if (caller == NULL)
    return -1;

  return __write(caller, vmaid, rgid, offset, value);
}

/*free_pcb_memphy - collect all memphy of pcb
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 */
int free_pcb_memph(struct pcb_t *caller)
{
  if (caller == NULL || caller->krnl == NULL || caller->krnl->mm == NULL)
    return -1;

  pthread_mutex_lock(&mmvm_lock);

  int pagenum, fpn;
  uint32_t pte;

#ifdef MM64
  int max_pgn = PAGING64_MAX_PGN;
#else
  int max_pgn = PAGING_MAX_PGN;
#endif

  for (pagenum = 0; pagenum < max_pgn; pagenum++)
  {
    pte = pte_get_entry(caller, pagenum);

    if (PAGING_PAGE_PRESENT(pte))
    {
      fpn = PAGING_FPN(pte);
      MEMPHY_put_freefp(caller->krnl->mram, fpn);
    }
    else if (pte & PAGING_PTE_SWAPPED_MASK)
    {
      fpn = PAGING_SWP(pte);
      MEMPHY_put_freefp(caller->krnl->active_mswp, fpn);
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

    if (pg == NULL)
        return -1;

    struct pgn_t *prev = NULL;
    int safety_count = 0;
    while (pg->pg_next != NULL)
    {
        prev = pg;
        pg = pg->pg_next;
        if (safety_count++ > 10000) { // Nếu lặp quá nhiều là có biến
             fprintf(stderr, "Error: Infinite loop in FIFO list!\n");
             return -1;
        }
    }

    *retpgn = pg->pgn;

    if (prev == NULL)
    {
        mm->fifo_pgn = NULL;
    }
    else
    {
        prev->pg_next = NULL;
    }

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
  if (caller == NULL || caller->krnl == NULL || caller->krnl->mm == NULL ||
      newrg == NULL || size <= 0)
    return -1;

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);
  if (cur_vma == NULL)
    return -1;

  struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;

  if (rgit == NULL)
    return -1;

  /* Probe uninitialized newrg. */
  newrg->rg_start = newrg->rg_end = -1;
  newrg->rg_next = NULL;

  /* Traverse the free vm region list to find a fit space. */
  while (rgit != NULL)
  {
    if (rgit->rg_start + size <= rgit->rg_end)
    { /* Current region has enough space. */
      newrg->rg_start = rgit->rg_start;
      newrg->rg_end = rgit->rg_start + size;

      /* Update left space in chosen region. */
      if (rgit->rg_start + size < rgit->rg_end)
      {
        rgit->rg_start = rgit->rg_start + size;
      }
      else
      { /* Use up all space, remove current node by cloning the next one. */
        struct vm_rg_struct *nextrg = rgit->rg_next;

        if (nextrg != NULL)
        {
          rgit->rg_start = nextrg->rg_start;
          rgit->rg_end = nextrg->rg_end;
          rgit->rg_next = nextrg->rg_next;
          free(nextrg);
        }
        else
        { /* End of free list. */
          rgit->rg_start = rgit->rg_end; // dummy, size 0 region
          rgit->rg_next = NULL;
        }
      }
      break;
    }

    rgit = rgit->rg_next;
  }

  if (newrg->rg_start == -1) // new region not found
    return -1;

  return 0;
}

// #endif
