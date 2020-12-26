#include <iostream>
#include "HttpServer.hpp"

static void Usage(std::string strProc)
{
  std::cout << "Usage: " << strProc << " port" << std::endl;

}

int main(int argc, const char* argv[])
{
  if (argc != 2)
  {
    Usage(argv[0]);
    exit(1);
  }
  HttpServer* pServer = new HttpServer(atoi(argv[1]));
  pServer->InitServer();
  pServer->Start();

  return 0;
}
