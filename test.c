#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "osqueue.h"
#include "threadPool.h"

void hello (void* a)
{
  printf("hello\n");
}
void bye (void* a)
{
  printf("bye\n");
}
void check (void* a)
{
  printf("checkcheck\n");
}

void test_thread_pool_sanity()
{
  int i;

  ThreadPool* tp = tpCreate(5);

  for(i=0; i<100; ++i)
  {
    tpInsertTask(tp,hello,NULL);
  }
  for(i=0; i<423; ++i)
  {
    tpInsertTask(tp,check,NULL);
  }
  for(i=0; i<400; ++i)
  {
    tpInsertTask(tp,bye,NULL);
  }
  //sleep(2);
  tpDestroy(tp,0);
}


int main()
{
  //test_thread_pool_sanity();
  ThreadPool* tp = tpCreate(10);
  tpDestroy(tp,1);

  return 0;
}
