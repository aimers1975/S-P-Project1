#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char** args) 
{
  pid_t cpid;
  int pfd[2],status;
  pipe(pfd);
  cpid=fork();
  char buf[80];
  
  if (cpid == 0) 
  {
    sprintf(buf, "%s:%d", args[1], getpid());
    write(pfd[1],args[1], strlen(args[1])+1);
    exit(1); 

  } else {
    cpid=fork();
    if(cpid ==0) {
      read(pfd[0],buf,80);
      printf("The child read:%s\n", buf);
      exit(2);
    } else { 


    }
    wait(&status);
    printf("Child %d\n", status>>8);
    wait(&status);
    printf("Child %d\n", status>>8);
  }
}
