# Simple OS Assignment

This repository contains the implementation and test commands for the **Simple OS Assignment**, including CPU scheduling, user memory management, and kernel memory management.

## Implemented Components

### Scheduler

Implemented files:

- `queue.c`
- `sched.c`

Main features:

- Multi-Level Queue scheduling
- Process queue management
- Support for single-core and multi-core execution
- Gantt chart output generation

> Note: Results may differ slightly when running with multiple CPUs due to scheduling order.

### User Memory Management

Implemented files:

- `libmem.c`
- `sys_mem.c`
- `mm64.c`
- `mm-vm.c`

Main features:

- User memory allocation and deallocation
- Paging
- Swapping
- Read and write memory operations
- Virtual memory region management

### Kernel Memory Management

Implemented files:

- `libmem.c`
- `sys_mem.c`
- `mm64.c`
- `mm-vm.c`

Main features:

- `kmalloc`
- `copy_from_user`
- `copy_to_user`
- `kmem_cache_create`
- `kmem_cache_alloc`

---

## Configuration

Before building or testing, enable the required macros in `os-cfg.h`.

### Scheduler

```c
#define MM_FIXED_MEMSZ
```

### Memory Debug Output

```c
#define DEBUG_PRINT 1
// #define IODUMP 1
```

### I/O Dump Output

```c
// #define DEBUG_PRINT 1
#define IODUMP 1
```

---

## Build

```sh
make
```

---

## Run Scheduler Tests

Enable `MM_FIXED_MEMSZ` for scheduler-only inputs because these files do not include a memory-size line:

```c
#define MM_FIXED_MEMSZ
```

```sh
mkdir -p result_sched

for test in votien_sched_1 votien_sched_2 votien_sched_3 votien_sched_4 votien_sched_5 sched sched_0 sched_1
do
  ./os "$test" > "result_sched/$test.text"
  python3 gantt.py "input/$test" "result_sched/$test.text" "result_sched/$test.png"
done
```

Generated files:

- `.text`: scheduler execution log
- `.png`: Gantt chart visualization

---

## Run User Memory Tests

Enable:

```c
#define DEBUG_PRINT 1
// #define IODUMP 1
// #define MM_FIXED_MEMSZ
```

Then run:

```sh
mkdir -p result_memory

for test in votien_memory_1 votien_memory_2 votien_memory_3 votien_memory_4 votien_memory_5 votien_memory_6 votien_memory_7 votien_memory_8 votien_memory_9
do
  ./os "$test" > "result_memory/$test.text"
done
```

Test descriptions:

| Test | Description |
|---|---|
| `votien_memory_1` | Allocation without swapping |
| `votien_memory_2` | Allocation with swapping |
| `votien_memory_3` | Read/write without swapping |
| `votien_memory_4` | Read/write with swapping |
| `votien_memory_5` | Read/write with swapping |
| `votien_memory_6` | Free memory |
| `votien_memory_7` | `kmalloc` |
| `votien_memory_8` | `copy_from_user`, `copy_to_user`, `kmem_cache_create`, `kmem_cache_alloc` |
| `votien_memory_9` | Kernel memory/cache test |

---

## Run Paging Tests with IODUMP

Enable:

```c
// #define DEBUG_PRINT 1
#define IODUMP 1
// #define MM_FIXED_MEMSZ
```

Then run:

```sh
mkdir -p result_memory

for test in os_0_mlq_paging os_1_mlq_paging os_1_mlq_paging_small_1K os_1_mlq_paging_small_4K os_1_singleCPU_mlq os_1_singleCPU_mlq_paging os_2_mlq_paging os_2_singleCPU_mlq_paging
do
  ./os "$test" > "result_memory/$test.text"
done
```

---

## All Top-Level Input Tests

These are the runnable test names found directly in `input/`. This list excludes `input/proc/`, which contains process descriptions used by the tests.

Scheduler tests:

```txt
votien_sched_1
votien_sched_2
votien_sched_3
votien_sched_4
votien_sched_5
sched
sched_0
sched_1
```

Memory and paging tests:

```txt
votien_memory_1
votien_memory_2
votien_memory_3
votien_memory_4
votien_memory_5
votien_memory_6
votien_memory_7
votien_memory_8
votien_memory_9
os_0_mlq_paging
os_1_mlq_paging
os_1_mlq_paging_small_1K
os_1_mlq_paging_small_4K
os_1_singleCPU_mlq
os_1_singleCPU_mlq_paging
os_2_mlq_paging
os_2_singleCPU_mlq_paging
```

System call tests:

```txt
os_sc
os_syscall
os_syscall_list
```

Run every top-level input test with Bash:

```sh
mkdir -p result_all

for test in $(find input -maxdepth 1 -type f -printf "%f\n" | sort)
do
  echo "Running $test..."
  ./os "$test" > "result_all/$test.text"
done
```

Run every top-level input test with PowerShell:

```powershell
New-Item -ItemType Directory -Force result_all | Out-Null

Get-ChildItem input -File | Sort-Object Name | ForEach-Object {
  $test = $_.Name
  Write-Host "Running $test..."
  .\os $test > "result_all\$test.text"
}
```

Run all scheduler tests and generate Gantt charts:

```sh
mkdir -p result_sched

for test in votien_sched_1 votien_sched_2 votien_sched_3 votien_sched_4 votien_sched_5 sched sched_0 sched_1
do
  echo "Running $test..."
  ./os "$test" > "result_sched/$test.text"
  python3 gantt.py "input/$test" "result_sched/$test.text" "result_sched/$test.png"
done
```

Run all memory, paging, and syscall tests:

```sh
mkdir -p result_memory

for test in votien_memory_1 votien_memory_2 votien_memory_3 votien_memory_4 votien_memory_5 votien_memory_6 votien_memory_7 votien_memory_8 votien_memory_9 os_0_mlq_paging os_1_mlq_paging os_1_mlq_paging_small_1K os_1_mlq_paging_small_4K os_1_singleCPU_mlq os_1_singleCPU_mlq_paging os_2_mlq_paging os_2_singleCPU_mlq_paging os_sc os_syscall os_syscall_list
do
  echo "Running $test..."
  ./os "$test" > "result_memory/$test.text"
done
```

Configuration reminder:

- Scheduler-only tests (`sched*`, `votien_sched_*`) need `#define MM_FIXED_MEMSZ`.
- Memory, paging, and syscall tests with a memory-size line need `// #define MM_FIXED_MEMSZ`.

---

## Output Structure

```txt
result_sched/
├── *.text
└── *.png

result_memory/
└── *.text
```

---

## Notes

- Use `MM_FIXED_MEMSZ` to avoid memory size mismatch errors.
- Use `DEBUG_PRINT` for detailed memory debugging.
- Use `IODUMP` for paging and memory dump outputs.
- Do not enable `DEBUG_PRINT` and `IODUMP` at the same time unless required.

---
