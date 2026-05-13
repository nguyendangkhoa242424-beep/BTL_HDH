# CO2018 Simple OS Report Work Split

## PDF Report Requirements

The report must answer every question in the assignment description and interpret the results from each test section.

Required report content:

- Scheduling: draw Gantt diagrams and explain how processes are executed by the CPU.
- Memory management: show data-segment memory allocation status.
- User/kernel space: explain how communication uses PID passing and kernel traversal, not direct PCB passing.
- Multi-level paging: show the 64-bit multi-level address translation scheme.
- Overall: interpret the simulation results and explain why the outputs are valid even if they differ from samples due to concurrency.

Grading emphasis:

- Scheduling demonstration: 3 points.
- MMU and user/kernel spaces: 2 points.
- Multi-level paging: 2 points.
- Written report: 3 points.

## Questions

1. What are the advantages and disadvantages of using the unified system call interface for manipulating different system components, i.e. read/write/free for files, memory, and I/O devices? Analyze OS design, performance trade-offs, error handling complexity, and portability versus efficiency.
2. When a system call executes for too long, how does the operating system detect and handle the case?
3. Considering the impact of detailed MLQ policies, what is the benefit of each policy?
4. What is the primary motivation for combining segmentation with paging in memory management? How does this hybrid approach address limitations of using either technique alone?
5. What are the benefits of extending hierarchical paging to N levels?
6. What are the advantages and disadvantages of paging and contiguous memory allocation?
7. What happens if synchronization is not handled in the Simple OS? Illustrate the problem with assignment outputs or kernel memory operation examples if available.

## Report Structure

1. Introduction
   - Assignment goals.
   - Implemented modules: scheduler, synchronization, memory management, system calls.
   - Summary of test environment and configuration macros.

2. Scheduler
   - MLQ policy description.
   - Queue operations: `enqueue`, `dequeue`, `get_proc`, `put_proc`.
   - Gantt diagrams from scheduler outputs.
   - Explanation of output differences caused by concurrent loader and CPU execution.
   - Answer Question 3.

3. System Calls And Kernel Interface
   - System call interface overview.
   - `listsyscall` and `memmap` behavior.
   - PID-based access rule: user code must not pass or access PCB directly.
   - Answer Questions 1 and 2.

4. Memory Management
   - Process virtual memory layout: VMA, `sbrk`, regions, free-region list, symbol region table.
   - Allocation, free, read, and write behavior.
   - Data-segment allocation status from memory test outputs.
   - User/kernel memory separation and copy operations.
   - Answer Questions 4 and 6.

5. Multi-Level Paging
   - 64-bit address layout.
   - PGD, P4D, PUD, PMD, PT, offset extraction.
   - Page table entry format.
   - Page replacement and swap behavior if implemented.
   - Memory-access statistics and multi-level paging storage size.
   - `vmap_pgd_memset` dummy allocation behavior.
   - Answer Question 5.

6. Synchronization And Integrated OS Behavior
   - Shared resources: ready queues, running list, physical memory, swap, VM structures.
   - Locks used in scheduler and memory management.
   - Race-condition examples and expected symptoms.
   - Answer Question 7.

7. Experimental Results
   - Scheduler test result summary.
   - Memory and paging test result summary.
   - System call test result summary.
   - Explain mismatches from sample outputs.

8. Conclusion
   - Summary of completed parts.
   - Known limitations.
   - Lessons learned.

## Four-Member Writing Assignment

### Member 1: Scheduler And Gantt Diagrams

Owner of report sections:

- Section 2: Scheduler.
- Scheduler part of Section 7: Experimental Results.

Tasks:

- Explain the MLQ scheduling policy: one ready queue per priority, `slot = MAX_PRIO - prio`, round-robin within the selected queue.
- Describe the implemented files: `src/queue.c`, `src/sched.c`, and relevant scheduler behavior in `src/os.c`.
- Generate or include Gantt diagrams from `result_sched/*.png`.
- Summarize scheduler outputs from `result_sched/*.text`.
- Answer Question 3.

Expected deliverables:

- 1-2 pages of scheduler explanation.
- At least one Gantt diagram with interpretation.
- Short comparison between actual output and sample output.

### Member 2: System Calls And User/Kernel Interface

Owner of report sections:

- Section 3: System Calls And Kernel Interface.
- System call part of Section 7: Experimental Results.

Tasks:

- Explain how system calls are invoked through registers and syscall table entries.
- Describe `listsyscall` and `memmap`.
- Explain why user code must pass PID and why kernel mode must traverse kernel structures to find the PCB.
- Check `src/sys_mem.c`, `src/syscall.c`, `src/sys_listsyscall.c`, and `src/libstd.c`.
- Summarize outputs from `os_sc`, `os_syscall`, and `os_syscall_list`.
- Answer Questions 1 and 2.

Expected deliverables:

- 1-2 pages on syscall design and PID-based kernel access.
- A small table of syscall tests and observed results.
- Clear answer on long-running syscall detection and handling.

### Member 3: Memory Management And User/Kernel Memory

Owner of report sections:

- Section 4: Memory Management.
- Memory part of Section 7: Experimental Results.

Tasks:

- Explain VMA, `sbrk`, `vm_rg_struct`, `vm_freerg_list`, and `symrgtbl`.
- Describe allocation, free, read, write, `kmalloc`, cache operations, `copy_from_user`, and `copy_to_user`.
- Show memory allocation status from memory tests in `result_memory/*.text`.
- Explain how the implementation prevents or should prevent overlapping virtual memory regions.
- Explain user-space versus kernel-space memory requirements.
- Answer Questions 4 and 6.

Expected deliverables:

- 2 pages on memory layout and operations.
- Diagrams or tables showing allocation/free examples.
- Summary of memory test outputs and important edge cases.

### Member 4: Multi-Level Paging, Synchronization, Integration

Owner of report sections:

- Section 1: Introduction.
- Section 5: Multi-Level Paging.
- Section 6: Synchronization And Integrated OS Behavior.
- Section 8: Conclusion.

Tasks:

- Explain the 64-bit multi-level paging translation flow: PGD -> P4D -> PUD -> PMD -> PT -> offset.
- Include an address-translation example using bit extraction.
- Report memory-access statistics and multi-level paging storage size if available from implementation/output.
- Explain `vmap_pgd_memset` and dummy page-directory allocation for large address spaces.
- Identify shared resources and describe locking strategy.
- Explain race-condition symptoms if synchronization is missing.
- Answer Questions 5 and 7.
- Merge all member sections into the final report and normalize formatting.

Expected deliverables:

- 2 pages on paging and synchronization.
- One address translation diagram or table.
- Final integrated report with consistent style and references.

## Integration Checklist

- [ ] All seven questions are answered.
- [ ] Scheduler section includes at least one Gantt diagram.
- [ ] Memory section includes allocation/free status.
- [ ] User/kernel section explicitly states PID passing and no direct PCB passing from user mode.
- [ ] Paging section includes multi-level address translation.
- [ ] Paging section includes memory-access and page-table storage statistics if implemented.
- [ ] Synchronization section identifies shared resources and lock usage.
- [ ] Experimental results cite scheduler, memory, paging, and syscall outputs.
- [ ] Known limitations are listed honestly.
- [ ] Final report is placed in the source-code directory before packaging.

