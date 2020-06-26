//
//Tal Zigdon 313326019
//
#include "threadPool.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
void deleteAllElements(ThreadPool *threadPool);
void deleteAndExit(ThreadPool *thread_pool);
static void runnerFunc(ThreadPool *thread_pool);
static void runOneTask(Task *task) {
  if (task != NULL) {
    task->computeFunc(task->param);
  }
}

ThreadPool *tpCreate(int numOfThreads) {
  ThreadPool *thread_pool = (ThreadPool *) malloc(sizeof(ThreadPool));
  //pthread_t thread;
  if (thread_pool == NULL) {
    perror("Error in system call");
    exit(0);
  }
  thread_pool->numOfRunningThreads = 0;
  thread_pool->threads = (pthread_t *) malloc(sizeof(pthread_t) * numOfThreads);
  if (thread_pool->threads == NULL) {
    perror("Error in system call");
    exit(0);
  }
  if (pthread_mutex_init(&thread_pool->mutex_, NULL) != 0) {
    perror("error with pthread function");
    deleteAndExit(thread_pool);
  }
  if (pthread_cond_init(&thread_pool->workCond, NULL) != 0) {
    perror("error with pthread function");
    deleteAndExit(thread_pool);
  }
  if (pthread_cond_init(&thread_pool->QueIsEmptyCond, NULL) != 0) {
    perror("error with pthread function");
    deleteAndExit(thread_pool);
  }
  if (pthread_cond_init(&thread_pool->AllCurrentProgressEnded, NULL) != 0) {
    perror("error with pthread function");
    deleteAndExit(thread_pool);
  }
  thread_pool->isTasksPushedToQue = 0;
  thread_pool->stopFlag = 0;
  thread_pool->stopNowFlag = 0;
  thread_pool->numOfFunctionsInProgress = 0;
  thread_pool->tasks = osCreateQueue();
  if (thread_pool->tasks == NULL) {
    perror("error with pthread function");
    deleteAndExit(thread_pool);
  }
  //put the threads in the pool
  //determine the attribute of the thread.
  pthread_attr_t pthread_attr;
  if (pthread_attr_init(&pthread_attr) != 0) {
    perror("error with pthread function");
    deleteAndExit(thread_pool);
  }
  pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_JOINABLE);
  int i;
  for (i = 0; i < numOfThreads; i++) {
    if (pthread_create(&(thread_pool->threads[i]), &pthread_attr, (void *(*)(void *)) runnerFunc, thread_pool) != 0) {
      if (errno == 11)
        perror("Resource temporarily unavailable");
      else
        perror("error with pthread function");
      deleteAndExit(thread_pool);
    }
    //pthread_detach(thread);
    thread_pool->numOfRunningThreads++;
    thread_pool->numOfThreads++;
  }
  pthread_attr_destroy(&pthread_attr);
  return thread_pool;
}

void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks) {
  if (threadPool == NULL)
    return;
  if (pthread_mutex_lock(&threadPool->mutex_) != 0) {
    perror("error with pthread function");
    //  deleteAndExit(threadPool);
  }
  threadPool->stopFlag = 1;
  if (pthread_mutex_unlock(&threadPool->mutex_) != 0) {
    perror("error with pthread function");
    //deleteAndExit(threadPool);
  }
  if (!shouldWaitForTasks) {
    if (pthread_mutex_lock(&threadPool->mutex_) != 0) {
      perror("error with pthread function");
      //deleteAndExit(threadPool);
    }
    threadPool->stopNowFlag = 1;
/*    if (pthread_cond_broadcast(&threadPool->workCond) != 0) {
      perror("error with pthread function");
      deleteAndExit(threadPool);
    }*/
    int i;
    int numOfThreads = threadPool->numOfRunningThreads;
    for (i = 0; i < numOfThreads; i++)
      if (pthread_cond_signal(&threadPool->workCond) != 0) {
        perror("error with pthread function");
        //deleteAndExit(threadPool);
      }
    if (pthread_cond_wait(&threadPool->AllCurrentProgressEnded, &threadPool->mutex_) != 0) {
      perror("error with pthread function");
      //deleteAndExit(threadPool);
    }
    if (pthread_mutex_unlock(&threadPool->mutex_) != 0) {
      perror("error with pthread function");
      //deleteAndExit(threadPool);
    }
    deleteAllElements(threadPool);
  } else {
    if (pthread_mutex_lock(&threadPool->mutex_) != 0) {
      perror("error with pthread function");
      //deleteAndExit(threadPool);
    }
    //wait for end all tasks in the que
    if(threadPool->isTasksPushedToQue) {
      if (pthread_cond_wait(&threadPool->QueIsEmptyCond, &threadPool->mutex_) != 0) {
        perror("error with pthread function");
        //deleteAndExit(threadPool);
      }
    }
    threadPool->stopNowFlag = 1;
    //if (pthread_cond_broadcast(&threadPool->workCond) != 0) {
    int i;
    int numOfThreads = threadPool->numOfRunningThreads;
    for (i = 0; i < numOfThreads; i++)
      if (pthread_cond_signal(&threadPool->workCond) != 0) {
        perror("error with pthread function");
        //deleteAndExit(threadPool);
      }
    //waiting to threads to finish.
    if (pthread_cond_wait(&threadPool->AllCurrentProgressEnded, &threadPool->mutex_) != 0) {
      perror("error with pthread function");
      //deleteAndExit(threadPool);
    }
    if (pthread_mutex_unlock(&threadPool->mutex_) != 0) {
      perror("error with pthread function");
      //deleteAndExit(threadPool);
    }
    deleteAllElements(threadPool);
  }
}
//function that free all the allocate bits
void deleteAllElements(ThreadPool *threadPool) {
  if (pthread_mutex_lock(&threadPool->mutex_) != 0) {
    perror("error with pthread function");
    //deleteAndExit(threadPool);
  }
  Task *temp = (Task *) osDequeue(threadPool->tasks);
  while (temp != NULL) {
    free(temp);
    temp = osDequeue(threadPool->tasks);
  }
  osDestroyQueue(threadPool->tasks);
  //if(pthread_cond_broadcast(&threadPool->workCond)!=0) {
  int i;
  int numOfThreads = threadPool->numOfRunningThreads;
  for (i = 0; i < numOfThreads; i++)
    if (pthread_cond_signal(&threadPool->workCond) != 0) {
      perror("error with pthread function");
    }
  for (i = 0; i < threadPool->numOfThreads; i++) {
    pthread_join(threadPool->threads[i], NULL);
  }
  free(threadPool->threads);
  if (pthread_cond_destroy(&threadPool->workCond) != 0) {
    {
      perror("error with pthread function");
    }
  }
  if (pthread_cond_destroy(&threadPool->AllCurrentProgressEnded) != 0) {
    perror("error with pthread function");
  }
  if (pthread_cond_destroy(&threadPool->QueIsEmptyCond) != 0) {
    perror("error with pthread function");
  }
  if (pthread_mutex_unlock(&threadPool->mutex_) != 0) {
    perror("error with pthread function");
  }
  if (pthread_mutex_destroy(&threadPool->mutex_) != 0) {
    perror("error with pthread function");
  }
  free(threadPool);
  threadPool = NULL;
  //pthread_exit(0);
}
static void runnerFunc(ThreadPool *thread_pool) {
  Task *task = NULL;
  while (1) {
    if (pthread_mutex_lock(&thread_pool->mutex_) != 0) {
      perror("error with pthread function");
      deleteAndExit(thread_pool);
      break;
    }
    if (!(thread_pool->stopNowFlag) && osIsQueueEmpty(thread_pool->tasks)) {
      //wait for signal that tells me that there is task in the que.
      if (pthread_cond_wait(&thread_pool->workCond, &thread_pool->mutex_) != 0) {
        perror("error with pthread function");
        deleteAndExit(thread_pool);
      }
    }
    //when stop flag is not 0 break
    if (thread_pool->stopNowFlag)
      break;
    thread_pool->numOfFunctionsInProgress++;
    task = (Task *) osDequeue(thread_pool->tasks);
    //functions in progress++, save it to tell destroy function to wait.
    if (pthread_mutex_unlock(&(thread_pool->mutex_)) != 0) {
      perror("error with pthread function");
      free(task);
      deleteAndExit(thread_pool);
      break;
    }
    //run function
    if (task != NULL) {
      task->computeFunc(task->param);
      free(task);
      task = NULL;
    }
    if (pthread_mutex_lock(&thread_pool->mutex_) != 0) {
      perror("error with pthread function");
      deleteAndExit(thread_pool);
      break;
    }
    //after task finished do --
    thread_pool->numOfFunctionsInProgress--;
    //signal that tells the destroyFunction that all the current tasks ended.
    if (osIsQueueEmpty(thread_pool->tasks) && thread_pool->numOfFunctionsInProgress == 0) {
      if (pthread_cond_signal(&thread_pool->QueIsEmptyCond) != 0) {
        perror("error with pthread function");
        //deleteAndExit(thread_pool);
      }
      //if (thread_pool->stopFlag)
      //  break;
    }
    if (pthread_mutex_unlock(&thread_pool->mutex_) != 0) {
      perror("error with pthread function");
      deleteAndExit(thread_pool);
    }
  }
  //signal that tell us when all threads stoped and numOfFunctions in progress equals to 0.
  thread_pool->numOfRunningThreads--;
  //signal that tells the destroyFunction that all the threads end.
  if (thread_pool->numOfRunningThreads == 0)
    if (pthread_cond_signal(&thread_pool->AllCurrentProgressEnded) != 0) {
      perror("error with pthread function");
      deleteAndExit(thread_pool);
    }
  if (pthread_mutex_unlock(&thread_pool->mutex_) != 0) {
    perror("error with pthread function");
    deleteAndExit(thread_pool);
  }
  //pthread_exit(0);
}

int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param) {
  //need to delete threadPool, cant push new task.
  if (threadPool->stopFlag)
    return -1;
  //allocate space for task
  Task *newFunc = (Task *) malloc(sizeof(Task));
  if (newFunc == NULL) {
    perror("Error with system call");
    deleteAndExit(threadPool);
  }
  //insert data to task
  newFunc->computeFunc = computeFunc;
  newFunc->param = param;
  //insert task to queue.
  if (pthread_mutex_lock(&(threadPool->mutex_)) != 0) {
    perror("error with pthread function");
    free(newFunc);
    deleteAndExit(threadPool);
  }
  if (threadPool->stopFlag == 1) {
    free(newFunc);
    return -1;
  }
  osEnqueue(threadPool->tasks, newFunc);
  threadPool->isTasksPushedToQue = 1;
  //tell the runner func that there is task in the que.
  if (pthread_cond_signal(&(threadPool->workCond)) != 0) {
    perror("error with pthread function");
    //free(newFunc);
    deleteAndExit(threadPool);
  }
  if (pthread_mutex_unlock(&(threadPool->mutex_)) != 0) {
    perror("error with pthread function");
    //free(newFunc);
    deleteAndExit(threadPool);
  }
  return 0;
}
void deleteAndExit(ThreadPool *thread_pool) {
  if (pthread_mutex_lock(&thread_pool->mutex_) != 0) {
    perror("error with pthread function");
  }
  thread_pool->stopNowFlag = 1;
  thread_pool->stopFlag = 1;
  //if (pthread_cond_broadcast(&thread_pool->workCond) != 0) {
  int i;
  int numOfThread = thread_pool->numOfRunningThreads;
  perror("error with pthread function");
  for (i = 0; i < numOfThread; i++)
    if (pthread_cond_signal(&thread_pool->workCond) != 0) {
      perror("error with pthread function");
    }
  if (pthread_cond_wait(&thread_pool->AllCurrentProgressEnded, &thread_pool->mutex_) != 0) {
    perror("error with pthread function");
  }
  for (i = 0; i < thread_pool->numOfThreads; i++) {
    if(pthread_join(thread_pool->threads[i],NULL)!=0){
      perror("error with pthread function");
    }
  }
  thread_pool->numOfThreads = 0;
  if (pthread_mutex_unlock(&thread_pool->mutex_) != 0) {
    perror("error with pthread function");
  }
  deleteAllElements(thread_pool);
  exit(1);
}
