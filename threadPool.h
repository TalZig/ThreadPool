//Tal Zigdon 313326019
#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include "osqueue.h"
#include <pthread.h>
typedef struct Task{
  void (*computeFunc) (void *);
  void* param;
}Task ;
typedef struct thread_pool
{
 //The field x is here because a struct without fields
 //doesn't compile. Remove it once you add fields of your own
 int numOfFunctionsInProgress;
 pthread_mutex_t mutex_;
 OSQueue* tasks;
 int stopFlag;
 int stopNowFlag;
 pthread_t* threads;
 int numOfThreads;
 int isTasksPushedToQue;
 pthread_cond_t workCond;
 pthread_cond_t QueIsEmptyCond;
 pthread_cond_t AllCurrentProgressEnded;
 int numOfRunningThreads;
}ThreadPool;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);
void checkFunc(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif
