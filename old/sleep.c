#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
int main() 
{
    while(1) {
      printf("This is a backgound process\n");
      usleep(10000);
    }
}
