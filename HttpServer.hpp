#pragma once
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include "ProtocolUtil.hpp"
#include "ThreadPool.hpp"

#define THREAD_NUM 5

class HttpServer
{
  public:
    HttpServer(int nPort)
      : mListen_sock(-1)
      , mPort(nPort)
    {
      
    }

    void InitServer()
    {
      mListen_sock = SocketApi::Socket();
      SocketApi::Bind(mListen_sock, mPort);
      SocketApi::Listen(mListen_sock);
    }

    void Start()
    {
      // 忽略SIGPIPE信号, 否则进程有可能会退出!
      signal(SIGPIPE, SIG_IGN); 

      for (;;)
      {
        std::string strPeer_ip;
        int nPeer_port;
        int nSock = SocketApi::Accept(mListen_sock, strPeer_ip, nPeer_port);
        if (nSock >= 0)
        {
          std::cout << strPeer_ip << " : " << nPeer_port << std::endl;
          Task task(nSock, Entry::HandlerRequest);

          Singleton::GetInstance()->PushTask(task); 
          // pthread_t tid;
          // pthread_create(&tid, NULL, Entry::HandlerRequest, (void*)pSock);
        }
      }
    }

    ~HttpServer()
    {
      if (mListen_sock >= 0)
      {
        close(mListen_sock);
      }
    }

  private:
    int mListen_sock;
    int mPort;
};

