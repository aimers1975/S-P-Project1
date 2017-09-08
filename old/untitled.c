#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
//#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

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
struct Command* parseInput(char*, char*, bool*, bool*);
char* collectInput();
void handleSignal(int);
char* trimTrailingWhitespace(char*);
struct Command createCommand(char*);
void* removeExcess(char**, int);
char** getTokenizedList(char*, char*, int*);
int getFileD(char*, char**, int, bool);



int main(int argc, char** args) 
{
    char buf[MAX_BUFFER];
    signal(SIGTSTP, &handleSignal);
    signal(SIGCHLD, &handleSignal);
    signal(SIGINT, &handleSignal);
    runInputLoop(buf);

}

void runInputLoop(char* buf ) {

    int numPaths = 0;
  	char* path = getenv("PATH");
    //printf("PATH :%s\n", (path!=NULL)? path : "getenv returned NULL");
    char** paths = getTokenizedList(path, ":", &numPaths);
    //printf("The returned number of paths is: %d\n", numPaths);
    while(1)
    { 
  	    char* buf2 = malloc(sizeof(char) * MAX_BUFFER);
  	    bool haspipe = false;
  	    bool bg = false;
  	    struct Command* jobs;
  	    int fd1;
  	    int fd2;
  	    int pfd[2],status;
  	    pid_t cpid;

        printf("# ");
        buf = collectInput();
        struct Command* thisJob = parseInput(buf, buf2, &haspipe, &bg);
        // For pipe: standard in is 0, standard out is 1
        if(haspipe) {
            pipe(pfd);
        }

        pid_t ret = fork();
        if (ret == 0) 
        {
        	// Child
        	// This child writes to pipe or to a file
        	if(haspipe) {
        	//	close(pfd[1]);
        	//	dup2(pfd[0], 0);
        	//	close(pfd[0]);
        	}
        	else if(strlen(thisJob[0].outfile) > 0) {
        		fd1 = getFileD(thisJob[0].outfile, paths, numPaths, true);
        		if(fd1 == -1)
        		{
        			printf("There was an error opening or creating file.\n");
        		}	
        	}
        	if(strlen(thisJob[0].infile) > 0) {
        		fd2 = getFileD(thisJob[0].infile, paths, numPaths, false);
        		if(fd2 == -1) {
        			printf("There was an error opening or creating file.\n");
        		}
        	}
        	printf("About to call exec\n");
        	execvpe(thisJob[0].cmd[0], thisJob[0].cmd, environ);
        	printf("Started pid: %uz\n", ret);
        } else if (ret < 0) 
        {
            printf("There was an error.");
        } else {
		    printf("Parent here: %d\n", getpid());
		    //printf("cpid: %u\n", cpid);
		    //cpid = fork();
		    //if(cpid == 0) {
		    //  printf("2 Child here %d\n", getpid());
		      // This child reads from pipe
		    //  close(pfd[0]);
        	//  dup2(pfd[1], 1);
        	//  close(pfd[1]);
		    //  execvpe(thisJob.cmd[0], thisJob.cmd, environ); 
		    //  exit(2);
		    //}
		    //wait(&status);
		    //printf("Child terminated with status: %d\n", status>>8);
		    //wait(&status);
		    //printf("Child terminated with status: %d\n", status>>8); 
		    if (thisJob[0].isForeground) {
        	    printf("Checking if foreground\n");
        	    waitpid(ret, &status, 0);
            }       	
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
struct Command* parseInput(char* buf, char* buf2, bool* haspipe, bool* bg)
{
	printf("You wrote: %s\n", buf);
	//printf("\n The buffer size was: %zd\n", strlen(buf));
	//printf(" Pipe is: %d\n", *pipe);

	int position = 0;
	for(int i=0; i < 200; i++) {
		if(buf[i] == '|' && !*haspipe) {
			buf[i] = '\0';
			*haspipe = true;
			i++;
			buf[i] = '\0';
		} else if (*haspipe) {
			buf2[position] = buf[i];
			position++;
			buf[i] = '\0';
		} 
	}

	buf = trimTrailingWhitespace(buf);
	if(strlen(buf2) > 0) buf2 = trimTrailingWhitespace(buf2);
	printf("Command 1: %s\n", buf);
	//printf("end\n");
	//printf("Command 2: %s", buf2);
	//printf("end\n");
	//printf(" Calling create command\n");
	//TODO fix this
	struct Command retCmd1 = createCommand(buf);
	printf("Called create command\n");
	struct Command retCmd2;
	if(*haspipe) {
	   retCmd2 = createCommand(buf2);
	}   
	struct Command* retCmdList;
	if(*haspipe) {
		printf("Has pipe, calling create command\n");
	    retCmd2 = createCommand(buf2);
	    retCmdList = malloc(sizeof(struct Command*) *2);
	    retCmdList[0] = retCmd1;
	    retCmdList[1] = retCmd2;
	} else
	{
		printf("Has no pipe, calling create command\n");
		retCmdList = malloc(sizeof(struct Command*));
		retCmdList[0] = retCmd1;
	}   
	printf("Cmd: %s\n", retCmd1.cmd[0]);
	if(*haspipe) printf("Cmd2: %s\n", retCmd2.cmd[0]);	
	return retCmdList;
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
	printf("Num cmds is: %d\n", numCmds );
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
	printf("Num commands is now: %d\n", numCmds);
	cmds = removeExcess(cmds, numCmds);


	struct Command thisCommand = {isRunning, isFor, cmds, numCmds, outfile, infile};
	printf("Got a command\n");
	return thisCommand;
}

int getFileD(char* file, char** paths, int num, bool create) {

    int filed ;
    if (access(file, F_OK) != -1) {
    	printf("File is in current dir.\n");
    	filed = open(file, O_RDWR|O_CREAT, 0777);
    	printf("The filed is: %d\n", filed);
    	return filed;
    }
    char* filepath;
    for(int i=0; i< num; i++) {
    	filepath = malloc(strlen(paths[i]) + strlen(file) + 2);
        strcpy(filepath, paths[i]);
        strcat(filepath, "/");
        strcat(filepath, file);
        printf("The final location is: %s\n", filepath);
    	if(access(filepath, F_OK) != -1) 
    	{
    		printf("The file is in this dir: %s\n", filepath);
    		filed = open(filepath, O_RDWR|O_CREAT, 0777);
    		printf("The filed is: %d\n", filed);
    	    return filed;
    	}
    }
    if(create)
    {	
        filed = open(file, O_RDWR|O_CREAT, 0777);
        printf("No file found, created %d\n", filed);
    }    
	return -1;

}


char** getTokenizedList(char* breakString, char* search, int* numStrings) {

	for(int i=0; i<strlen(breakString); i++) {
		if(breakString[i] == ':') (*numStrings)++;
	}
    if(strlen(breakString) >= 1 && (*numStrings) == 0) 
    {
        (*numStrings)=1;
    }
    else if (strlen(breakString) >= 1)
    {
    	(*numStrings)++;
    }    
    char** pathList = malloc(sizeof(char*) * (*numStrings));
	char* token = strtok(breakString, search);
	int position = 0;
	while(token) {
		pathList[position] =token;
		position++;
		token = strtok(NULL, search);
	}
	return pathList;
}


