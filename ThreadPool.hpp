#pragma once
#include <iostream>
#include <queue>
#include <pthread.h>

#define THREAD_NUM 5

typedef void (*handler_t)(int);
class Task
{
  public:
    Task(int nSock, handler_t hHandler)
      : m_nSock(nSock)
      , m_hHandler(hHandler)
    {

    }
    ~Task()
    {

    }
    void Run()
    {
      m_hHandler(m_nSock);
    }

  private:
    int m_nSock;
    handler_t m_hHandler;
};

class ThreadPool
{
  public:
    ThreadPool(int nNum)
      : m_nMaxThreadNum(nNum)
      , m_nFreeThreadNum(0)
    {
      pthread_mutex_init(&m_mutex, NULL);
      pthread_cond_init(&m_cond, NULL);
    }

    ~ThreadPool()
    {
      pthread_mutex_destroy(&m_mutex);
      pthread_cond_destroy(&m_cond);
    }

    bool IsTaskQueueEmpty()
    {
      return m_taskQueue.empty();
    }

    void LockQueue()
    {
      pthread_mutex_lock(&m_mutex);
    }
    
    void UnlockQueue()
    {
      pthread_mutex_unlock(&m_mutex);
    }

    void GetFreeThread()
    {
      m_nFreeThreadNum++;
      pthread_cond_wait(&m_cond, &m_mutex);
      m_nFreeThreadNum--;
    }

    void WakeUp()
    {
      pthread_cond_signal(&m_cond);
    }

    Task PopTask()
    {
      Task t = m_taskQueue.front();
      m_taskQueue.pop();
      return t;
    }

    void PushTask(Task& t)
    {
      LockQueue();
      m_taskQueue.push(t);
      UnlockQueue();
      WakeUp();
    }

    static void* ThreadRoutine(void* arg)
    {
      pthread_detach(pthread_self());
      ThreadPool* tp = (ThreadPool*)arg;

      for (;;)
      {
        tp->LockQueue();
        while (tp->IsTaskQueueEmpty())
        {
          tp->GetFreeThread();
        }

        Task task = tp->PopTask();
        tp->UnlockQueue();
        
        std::cout << "task in: " << pthread_self() << std::endl;
        task.Run();
      }
    }

    void InitThreadPool()
    {
      pthread_t tid;
      for (int i = 0; i < m_nMaxThreadNum; i++)
      {
        pthread_create(&tid, NULL, ThreadRoutine, (void*)this);
      }
    }
    
  private:
    int m_nMaxThreadNum;
    int m_nFreeThreadNum; // 空闲的线程数
    std::queue<Task> m_taskQueue;
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};

class Singleton
{
  public:
    static ThreadPool* GetInstance()
    {
      if (NULL == m_pool)
      {
        pthread_mutex_lock(&m_mutex);
        if (NULL == m_pool)
        {
          m_pool = new ThreadPool(THREAD_NUM);
          m_pool->InitThreadPool();
        }
        pthread_mutex_unlock(&m_mutex);
      }

      return m_pool;
    }
  private:
    static ThreadPool* m_pool;
    static pthread_mutex_t m_mutex;
};

ThreadPool* Singleton::m_pool = NULL;
pthread_mutex_t Singleton::m_mutex = PTHREAD_MUTEX_INITIALIZER;
