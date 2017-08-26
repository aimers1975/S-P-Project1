#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
//#include <sys/types.h>
//#include <sys/wait.h>
#include <string.h>
#include <signal.h>

#define MAX_BUFFER 200
#define true 1
#define false 0

extern char** environ;

typedef int bool;

void runInputLoop(char*);
void parseInput(char*, char*, bool*, bool*);
char* collectInput();
void handleSignal(int);


int main(int argc, char** args) 
{
  char buf[MAX_BUFFER];
  const char* path = getenv("PATH");

  printf("PATH :%s\n", (path!=NULL)? path : "getenv returned NULL");
  signal(SIGTSTP, &handleSignal);
  signal(SIGCHLD, &handleSignal);
  signal(SIGINT, &handleSignal);
  runInputLoop(buf);

}

void runInputLoop(char* buf ) {

  while(1)
  { 
  	char* buf2 = malloc(sizeof(char) * MAX_BUFFER);
  	bool pipe = false;
  	bool bg = false;
    printf("# ");
    char* env_list[] = {};
    buf = collectInput();
    parseInput(buf, buf2, &pipe, &bg);


    if(pipe) {
    	printf(" There was a pipe\n");
    }
    if(bg) {
    	printf(" There was a bg request\n");
    }
  }
  
}

char* collectInput() 
{
  int position = 0;
  int bufsize = MAX_BUFFER;
  char* buffer = malloc(sizeof(char) * bufsize);
  int c;
  
  if (!buffer) {
  	fprintf(stderr, "Collect input failed to allocate buffer");
  	exit(EXIT_FAILURE);
  }

  while(1) {  	
  	c = getchar();

  	if (c == EOF || c == '\n' || position == (200-1)) {
  		buffer[position] = '\0';
  		return buffer;
  		
  	} else {
  		buffer[position] = c;
  		position++;
  	} 
  }
  return buffer;
}

//void parseInput(char* buf, ssize_t bufsize) 
void parseInput(char* buf, char* buf2, bool* pipe, bool* bg)
{
	printf("\n You wrote: %s", buf);
	printf("\n The buffer size was: %zd\n", strlen(buf));
	printf(" Pipe is: %d\n", *pipe);
	int position = 0;
	for(int i=0; i < 200; i++) {
		if(buf[i] == '|' && !*pipe) {
			buf[i] = '\0';
			*pipe = true;
			i++;
			buf[i] = '\0';
		} else if (*pipe) {
			buf2[position] = buf[i];
			position++;
			buf[i] = '\0';
		} else if (buf[i] == '&')
		{
			*bg = true;
			buf[i] = '\0';
		} else if (*bg) {
			buf[i] = '\0';
		} else if (buf[i] == '<') {

		} else if (buf[i] == '>') {

		}
	}
	printf("Command 1: %s\n", buf);
	printf("Command 2: %s\n", buf2);
}

void handleSignal(int signal) {
  const char* signal_name;
  sigset_t pending;
  printf("This signal: %d", signal);

  switch(signal) {
  	case SIGINT:
  	  signal_name = "SIGINT";
  	  printf("signal is: %s\n", signal_name);
  	  break;
  	case SIGTSTP:
  	  signal_name = "SIGTSTP";
  	  printf("signal is: %s\n", signal_name);
  	  break;
  	case SIGCHLD:
  	  signal_name = "SIGCHLD";
  	  printf("signal is: %s\n", signal_name);
  	  break;
  	default:
  	  printf("Can't find signal.");
  	  return;

  }
}

