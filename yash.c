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
  	    bool haspipe = false;
  	    bool bg = false;
  	    struct Command* jobs;
  	    int fd1, fd2, status;
  	    int pipefd[2];
  	    char ch[2]={0,0}, pch=128;
  	    int numPaths = 0;
  	    char* path = getenv("PATH");
  	    char* currpath = malloc(strlen(path));
        strcpy(currpath, path);
  	    char** paths = getTokenizedList(currpath, ":", &numPaths);
        printf("# ");
        char* env_list[] = {};
        buf = collectInput();

        struct Command* thisJob = parseInput(buf, buf2, &haspipe, &bg);

        if(haspipe) {
        	if(pipe(pipefd) == -1) {
        		perror("pipe");
        		exit(-1);
        	}
        }

        pid_t ret = fork();

        if (ret == 0) 
        {
        	// First Child, if there is a pipe, there can't be an outfile

        	if(haspipe) {
                dup2(pipefd[0],0);
                close(pipefd[1]);

        	}
        	else if(strlen(thisJob[0].outfile) > 0) {
        		printf("Trying to open outfile: %s\n", thisJob[0].outfile);
        		fd1 = getFileD(thisJob[0].outfile, paths, numPaths, true);
        		if(fd1 == -1)
        		{
        			printf("There was an error opening or creating the out file.\n");
        		}	
        	}
        	if(strlen(thisJob[0].infile) > 0) {
        		printf("Trying to open: %s\n", thisJob[0].outfile);
        		fd2 = getFileD(thisJob[0].infile, paths, numPaths, false);
        		if(fd2 == -1) {
        			printf("There was an error opening or creating the in file.\n");
        		}
        	}

        	printf("READER calling exec: %s\n", thisJob[0].cmd[0]);
        	printf("Started cpid: %d Current pid: %d\n", ret, getpid());
        	execvpe(thisJob[0].cmd[0], thisJob[0].cmd, environ);
        	printf("READER exec returned\n");
        	
        	exit(1);
        } else if (ret < 0) 
        {

        } else if (haspipe) {
        	ret = fork();
        	//printf("Parent here cpid: %d pid: %d\n", ret, getpid());
        	if(ret ==0) {
                //printf("The first process command: %s pid: %d\n", thisJob[1].cmd[0], getpid());

	        	if(haspipe) {
                    dup2(pipefd[1],1);
                    close(pipefd[0]);

	        	} else if(strlen(thisJob[1].infile) > 0) {
	        		printf("Trying to open: %s\n", thisJob[1].outfile);
	        		fd2 = getFileD(thisJob[1].infile, paths, numPaths, false);
	        		if(fd2 == -1) {
	        			printf("There was an error opening or creating the in file.\n");
	        		}
	        	}

	        	if(strlen(thisJob[1].outfile) > 0) {
	        		printf("Trying to open outfile: %s\n", thisJob[1].outfile);
	        		fd1 = getFileD(thisJob[1].outfile, paths, numPaths, true);
	        		if(fd1 == -1)
	        		{
	        			printf("There was an error opening or creating the out file.\n");
	        		}	
	        	}

                printf("Parent WRITER calling exec: %s\n", thisJob[1].cmd[0]); 
                printf("Started cpid: %d Current pid: %d\n", ret, getpid());
                execvpe(thisJob[1].cmd[0], thisJob[1].cmd, environ);
                printf("WRITER exec returned\n");
                exit(2);

            } else {
            	//wait(&status);
            	close(pipefd[1]);
            	waitpid(ret, &status, 0);
            	
            }
            
        }	
        waitpid(0, &status, 0);
        //wait(&status);
        //if (thisJob[0].isForeground) {
        //	printf("Checking if foreground\n");
        	//printf("Calling first wait pid: %d, ret: %d\n", getpid(), ret);
        //	waitpid(ret, &status, 0);  
        //	if(haspipe) {
        //	    printf("Calling second wait pid: %d, ret: %d\n", getpid(), ret);
        //	    waitpid(0, &status, 0);
        //	}          	
        //}

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
	//printf(" haspipe is: %d\n", *haspipe);
    
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
	buf2 = trimTrailingWhitespace(buf2);
	//printf("Command 1: %s", buf);
	//printf("end\n");
	//printf("Command 2: %s", buf2);
	//printf("end\n");
	struct Command retCmd = createCommand(buf);
	struct Command* cmdList;
	struct Command retcmd2;
	if(*haspipe) {
        cmdList = malloc(2 * sizeof(struct Command));
        cmdList[1] = retCmd;
        retcmd2 = createCommand(buf2);    
        cmdList[0] = retcmd2;
	} else {
        cmdList = malloc(sizeof(struct Command));
        cmdList[0] = retCmd;
	}	
	return cmdList;   	
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
    	printf("Trying to open: %s\n", file);
        filed = open(file, O_RDWR|O_CREAT, 0777);
        printf("No file found, created %d\n", filed);
        return filed;
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

