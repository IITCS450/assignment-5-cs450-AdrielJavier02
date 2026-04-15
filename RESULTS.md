# RESULTS

## Context-switching approach
Implemented cooperative user-level threads in user space. Each thread has its own stack. Context switching is done by saving the current thread stack pointer and restoring the next runnable thread stack pointer during `thread_yield()`.

## Limitations
- Supports up to 8 threads
- Fixed stack size of 4096 bytes per thread
- Cooperative scheduling only, so threads must call `thread_yield()`