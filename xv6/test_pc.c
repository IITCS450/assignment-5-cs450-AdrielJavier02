#include "types.h"
#include "stat.h"
#include "user.h"
#include "uthread.h"
#include "umutex.h"

#define N 8

static int buf[N], head, tail, count;
static umutex_t mu;

static void
producer(void *arg)
{
  int id = (int)arg;

  for(int i = 0; i < 100; ){
    mutex_lock(&mu);
    if(count < N){
      buf[tail] = id * 1000 + i;
      tail = (tail + 1) % N;
      count++;
      i++;
    }
    mutex_unlock(&mu);
    thread_yield();
  }
}

static void
consumer(void *arg)
{
  (void)arg;
  int got = 0;

  while(got < 200){
    mutex_lock(&mu);
    if(count > 0){
      int x = buf[head];
      head = (head + 1) % N;
      count--;
      got++;
      if(got % 50 == 0)
        printf(1, "consumer got %d (last=%d)\n", got, x);
    }
    mutex_unlock(&mu);
    thread_yield();
  }
}

int
main(void)
{
  thread_init();
  mutex_init(&mu);

  tid_t p1 = thread_create(producer, (void*)1);
  tid_t p2 = thread_create(producer, (void*)2);
  tid_t c1 = thread_create(consumer, 0);

  if(p1 < 0 || p2 < 0 || c1 < 0){
    printf(1, "thread_create failed\n");
    exit();
  }

  thread_join(p1);
  thread_join(p2);
  thread_join(c1);

  printf(1, "test_pc: done\n");
  exit();
}