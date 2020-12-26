#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main()
{
  char strParamRes[1024];
  char* strEnv;

  strEnv = getenv("HTTP_CONTENT_LENGTH");
  //printf("end getenv()= %s\n", strLib);
  int nSize = atoi(strEnv);
  int i = 0;
  for (; i < nSize; i++)
  {
    read(0, strParamRes + i, 1);
  }

  strParamRes[i] = '\n';
  printf("In CGI. Length: %d, echo: %s\r\n", nSize, strParamRes);
  return 0;
}
