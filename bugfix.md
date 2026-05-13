# Code Review Summary

## Critical Issues

| Priority | Issue | Location | Problem |
|---|---|---|---|
| High | 64-bit multi-level paging is mostly stubbed | 
`src/mm64.c:70` | The code extracts `PGD`, `P4D`, `PUD`, `PMD`, and `PT` indices, but functions like `pte_set_swap`, `pte_set_fpn`, and `pte_get_entry` still use flat access through `mm->pgd[pgn]`. This does not satisfy the requirement for theoretical multi-level paging. |
| High | Kernel/user memory separation is not properly implemented | `src/libmem.c:544`, `src/libmem.c:584`, `src/libmem.c:645` | `kmem_cache_create` is almost a no-op, `kmem_cache_alloc` ignores the cache pool and allocates a full page, and kernel memory read/write operations still go through the same user-space `__read` / `__write` path. |
| High | Allocation may create duplicate overlapping free regions | `src/mm-vm.c:165`, `src/libmem.c:122` | After extending the VMA, the aligned leftover region is added once in `mm-vm.c`, then added again in `libmem.c`. This can cause two later allocations to receive overlapping virtual memory. |

---

## Medium Issues

| Priority | Issue | Location | Problem |
|---|---|---|---|
| Medium | Symbol table bounds checks are off by one | `src/libmem.c:57`, `src/libmem.c:75`, `src/libmem.c:147` | The condition `rgid > PAGING_MAX_SYMTBL_SZ` should be `rgid >= PAGING_MAX_SYMTBL_SZ`. Otherwise, index `30` is accepted even when the table only has 30 elements. |
| Medium | Build is not portable on Windows shell | `Makefile:35`, `Makefile:41` | `mingw32-make` fails because the Makefile uses Unix `chmod`. Also, the `sched` target links `$(MEM_OBJ)` instead of `$(SCHED_OBJ)`. |

---

## Suggested Fixes

### 1. Implement real multi-level page table walking

Current implementation still behaves like a flat page table:

```c
mm->pgd[pgn]