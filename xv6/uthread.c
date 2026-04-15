#include "types.h"
#include "stat.h"
#include "user.h"
#include "uthread.h"

#define MAX_THREADS 8
#define STACK_SIZE  4096

enum tstate {
  T_FREE = 0,
  T_RUNNABLE,
  T_RUNNING,
  T_ZOMBIE
};

struct thread {
  tid_t tid;
  int state;
  void *stack;
  uint sp;
  void (*fn)(void *);
  void *arg;
};

static struct thread threads[MAX_THREADS];
static int current_tid = 0;
static int initialized = 0;

static void thread_stub(void);
static void thread_schedule(void);
void thread_switch(uint *old_sp, uint new_sp);

__asm__(
".globl thread_switch\n"
"thread_switch:\n"
"  movl 4(%esp), %eax\n"
"  movl 8(%esp), %edx\n"
"  pushl %ebp\n"
"  pushl %ebx\n"
"  pushl %esi\n"
"  pushl %edi\n"
"  movl %esp, (%eax)\n"
"  movl %edx, %esp\n"
"  popl %edi\n"
"  popl %esi\n"
"  popl %ebx\n"
"  popl %ebp\n"
"  ret\n"
);

void
thread_init(void)
{
  int i;

  for(i = 0; i < MAX_THREADS; i++){
    threads[i].tid = i;
    threads[i].state = T_FREE;
    threads[i].stack = 0;
    threads[i].sp = 0;
    threads[i].fn = 0;
    threads[i].arg = 0;
  }

  threads[0].state = T_RUNNING;
  current_tid = 0;
  initialized = 1;
}

tid_t
thread_create(void (*fn)(void*), void *arg)
{
  int i;
  uint *sp;
  char *stack;

  if(!initialized)
    thread_init();

  for(i = 1; i < MAX_THREADS; i++){
    if(threads[i].state == T_FREE)
      break;
  }

  if(i == MAX_THREADS)
    return -1;

  stack = malloc(STACK_SIZE);
  if(stack == 0)
    return -1;

  sp = (uint *)(stack + STACK_SIZE);

  *--sp = (uint)thread_stub;
  *--sp = 0;
  *--sp = 0;
  *--sp = 0;
  *--sp = 0;

  threads[i].tid = i;
  threads[i].state = T_RUNNABLE;
  threads[i].stack = stack;
  threads[i].sp = (uint)sp;
  threads[i].fn = fn;
  threads[i].arg = arg;

  return i;
}

static void
thread_schedule(void)
{
  int old = current_tid;
  int next = -1;
  int i;

  for(i = 1; i <= MAX_THREADS; i++){
    int idx = (old + i) % MAX_THREADS;
    if(threads[idx].state == T_RUNNABLE){
      next = idx;
      break;
    }
  }

  if(next == -1)
    return;

  if(threads[old].state == T_RUNNING)
    threads[old].state = T_RUNNABLE;

  threads[next].state = T_RUNNING;
  current_tid = next;

  thread_switch(&threads[old].sp, threads[next].sp);
}

void
thread_yield(void)
{
  if(!initialized)
    thread_init();

  thread_schedule();
}

static void
thread_stub(void)
{
  int tid = current_tid;

  threads[tid].fn(threads[tid].arg);
  threads[tid].state = T_ZOMBIE;

  thread_yield();

  for(;;)
    thread_yield();
}

int
thread_join(tid_t tid)
{
  if(tid <= 0 || tid >= MAX_THREADS)
    return -1;

  if(threads[tid].state == T_FREE)
    return -1;

  while(threads[tid].state != T_ZOMBIE){
    thread_yield();
  }

  if(threads[tid].stack){
    free(threads[tid].stack);
    threads[tid].stack = 0;
  }

  threads[tid].sp = 0;
  threads[tid].fn = 0;
  threads[tid].arg = 0;
  threads[tid].state = T_FREE;

  return 0;
}