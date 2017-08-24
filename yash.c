#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
//#include <sys/types.h>
//#include <sys/wait.h>
#include <string.h>

#define MAX_BUFFER 200
typedef int bool;
#define true 1
#define false 0

void runInputLoop(char*);
//void parseInput(char* buf, ssize_t bufsize);
void parseInput(char*, char*, bool*, bool*);
char* collectInput();

int main(int argc, char** args) 
{
  char buf[MAX_BUFFER];
  runInputLoop(buf);
}

void runInputLoop(char* buf ) {

  while(1)
  { 
  	char* buf2 = malloc(sizeof(char) * MAX_BUFFER);
  	bool pipe = false;
  	bool bg = false;
    printf("# ");
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
  //ssize_t bufsize = 0;
  //getline(&buf, &bufsize, stdin);
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
		}
	}
	printf("Command 1: %s\n", buf);
	printf("Command 2: %s\n", buf2);


}

//scanf("%200[0-9a-zA-Z,.<>\?/;\':\"|-=_+()`\\~!@#$%^&*' ]", buf);
