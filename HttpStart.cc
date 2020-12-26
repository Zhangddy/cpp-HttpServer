#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
  pid_t pid;
  if ((pid = fork()) < 0) 
  {
    perror("fork error");
    return EXIT_FAILURE;
  } 
  else if (0 == pid) 
  {
    printf("first child is running..\n"); 
    /**在第一个子进程中再次fork***/
    if ((pid = fork()) < 0) 
    {
      perror("fork error");
      return EXIT_FAILURE;
    } 
    else if (pid > 0) 
    {
      /**父进程退出**/
      printf("[%ld] first child is exit...\n", (long)getpid());
      _exit(0);
    }
    sleep(2);/**确保父进程先运行**/
    // printf("second process pid: %ld, second process's parent pid: %ld\n", (long)getpid(), (long)getppid()); 
    // printf("[%ld] is exit..\n", (long)getpid());
    
    execl("./HttpServer", "HttpServer", "80", NULL);

    _exit(0);
  }

  /***获得第一个子进程的退出状态***/
  if (waitpid(pid, NULL, 0) < 0) 
  {
    perror("waitpid error");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
