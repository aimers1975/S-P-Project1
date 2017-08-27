#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
//#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

#define MAX_BUFFER 200
#define true 1
#define false 0

extern char** environ;

typedef int bool;

struct Command 
{
	char* isRunning;
    bool isForeground;
    char** cmd;
    int numCmds;
    char* outfile;
    char* infile;
  
};

void runInputLoop(char*);
struct Command parseInput(char*, char*, bool*, bool*);
char* collectInput();
void handleSignal(int);
char* trimTrailingWhitespace(char*);
struct Command createCommand(char*);
void* removeExcess(char**, int);


int main(int argc, char** args) 
{
    char buf[MAX_BUFFER];
    const char* path = getenv("PATH");

    //printf("PATH :%s\n", (path!=NULL)? path : "getenv returned NULL");
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
  	    struct Command* jobs;
        printf("# ");
        char* env_list[] = {};
        buf = collectInput();
        struct Command thisJob = parseInput(buf, buf2, &pipe, &bg);
        pid_t ret = fork();
        if (ret == 0) 
        {
        	// Child
        	execv(thisJob.cmd[0], thisJob.cmd);
        	printf("Started pid: %uz\n", ret);
        } else if (ret < 0) 
        {

        } 
        int status;	
        if (thisJob.isForeground) {
        	printf("Checking if foreground\n");
        	waitpid(ret, &status, 0);
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
struct Command parseInput(char* buf, char* buf2, bool* pipe, bool* bg)
{
	printf("You wrote: %s\n", buf);
	//printf("\n The buffer size was: %zd\n", strlen(buf));
	//printf(" Pipe is: %d\n", *pipe);

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
		} 
	}

	buf = trimTrailingWhitespace(buf);
	buf2 = trimTrailingWhitespace(buf2);
	//printf("Command 1: %s", buf);
	//printf("end\n");
	//printf("Command 2: %s", buf2);
	//printf("end\n");
	printf(" Calling create command\n");
	struct Command retCmd = createCommand(buf);
	printf("Cmd: %s\n", retCmd.cmd[0]);
	return retCmd;
	if(*pipe) {
	    createCommand(buf2);
	}    	
}

void handleSignal(int signal) {
    const char* signal_name;
    sigset_t pending;
    printf("This signal: %d\n", signal);

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
  	        printf("Can't find signal.\n");
  	        return;
    }
}

char* trimTrailingWhitespace(char* buf) {
	char *end;
	end = buf + strlen(buf) - 1;
	while(end > buf && *end == ' ') end--;
	*(end+1) = 0;
	return buf;
}

void* removeExcess(char** buf, int trimNum) {
	if (trimNum == 0) {
		return NULL;
	}
	char** cmds = malloc(sizeof(char*) * trimNum);
	memcpy(cmds, buf, (sizeof(char*) * trimNum));
	return cmds;
}

struct Command createCommand(char* buf) {
	printf("This starting buffer is: %s\n", buf);
	int numCmds = 1;
	bool isFor = true;
	char* isRunning = "Stopped";
    // Find number of strings in this array
	for(int i=0; i<strlen(buf); i++) {
		if(buf[i] == ' ' || buf[i] == '\0') 
		{
			numCmds++;
		}
	}
    // Get all the strings divided by spaces
	char** cmds = malloc(sizeof(char*) * numCmds);
	char* token = strtok(buf, " ");
	int position = 0;
	while(token) {
		printf("%s\n", token);
		cmds[position] =token;
		position++;
		token = strtok(NULL, " ");
	}

    // Figure if this command will be a background process
	char* infile = "";
	char* outfile = "";
	for(int i=0; i < numCmds; i++) {
		if((strcmp(cmds[i],"&") == 0) && isFor) {
            isFor = false;
            numCmds--;
            break;
        }
    }   

    // Get in and outfiles
    int lastCmd = 0;
	printf("Num commands is now: %d\n", numCmds);
	printf("Last command is now: %d\n", lastCmd);
	for(int i=0; i<numCmds; i++) {
	   if (strcmp(cmds[i], "<") == 0) {
            infile = cmds[i + 1];
            printf("Found <\n");
            if(lastCmd == 0) lastCmd = i;
            printf("Numcmds: %d lastCmd: %d\n", numCmds, lastCmd);
		} else if (strcmp(cmds[i], ">") == 0) {
            outfile = cmds[i + 1];
            printf("Found >\n");
            if(lastCmd == 0) lastCmd = i;
		} 
	}	
	if (lastCmd < numCmds && lastCmd != 0) numCmds = lastCmd;
	cmds = removeExcess(cmds, numCmds);


	struct Command thisCommand = {isRunning, isFor, cmds, numCmds, outfile, infile};
	//printf("Command is Running: %s\n", thisCommand.isRunning);
	//printf("Command is For: %d\n", thisCommand.isForeground);
	//printf("Command numCmds: %d\n", thisCommand.numCmds);
	//printf("Command outfile: %s\n", thisCommand.outfile);
	//printf("Command infile: %s\n", thisCommand.infile);
	return thisCommand;
}



