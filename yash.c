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
	int pid;
    bool isForeground;
    bool mostRecent;
    char** cmd;
    int numCmds;
    char* outfile;
    char* infile;
    struct Command* nextCommand;
  
};

struct Job
{
	char* cmd;
	int id;
	int pid;
	bool isMostRecent;
	bool isRunning;
	struct Job* nextJob;
};

void runInputLoop(char*);
struct Command* parseInput(char*, char*, bool*);
char* collectInput();
void handleSignal(int);
char* trimTrailingWhitespace(char*);
struct Command createCommand(char*);
void* removeExcess(char**, int);
char** getTokenizedList(char*, char*, int*);
int getFileD(char*, char**, int, bool);
void printJobs(struct Job*);
void pushJob(struct Job*, char*, bool, bool, int, int);
int removeJob(struct Job*, int);
int removeLastJob(struct Job*);
void updateMostRecent();
int updatePID(struct Job*, int);
void stopJob();

int main(int argc, char** args) 
{
    char buf[MAX_BUFFER];

    const char* path = getenv("PATH");
    struct Job* jobs = malloc(sizeof(struct Job));


    signal(SIGTSTP, &handleSignal);
    signal(SIGCHLD, &handleSignal);
    signal(SIGINT, &handleSignal);
    runInputLoop(buf);

}

void runInputLoop(char* buf ) {

	struct Job* jobs = malloc(sizeof(struct Job));
    int jobsSize = 1;

    while(1)
    { 
  	    char* buf2 = malloc(sizeof(char) * MAX_BUFFER);
  	    char* printBuf = NULL; 
  	    printBuf = malloc(sizeof(char) * MAX_BUFFER);
  	    bool haspipe = false;
  	    
  	    int fd1, fd2, status;
  	    int pipefd[2];
  	    int numPaths = 0;

  	    char* path = getenv("PATH");
  	    char* currpath = malloc(strlen(path));
        strcpy(currpath, path);
  	    char** paths = getTokenizedList(currpath, ":", &numPaths);
        printf("# ");
        char* env_list[] = {};

        buf = collectInput();
        strcpy(printBuf,buf);
        
        struct Command* thisCommand = parseInput(buf, buf2, &haspipe);
        printf("Finding jobs buffer: %s\n", buf);
        if(strcmp(buf,"jobs") == 0) {
        	printJobs(jobs);
        	continue;
        } else {
        	printf("Didn't see jobs command\n");
        }
        if(!thisCommand->isForeground) {

        	pushJob(jobs,printBuf,true,true,jobsSize,jobsSize);
        	jobsSize++;
        	if(jobs == NULL) {
        		printf("jobs is null again");
        	} else {
        		printf("jobs is still not null -whew");
        	}
        }  
        printf("jobsSize is now %d\n", jobsSize);  
   
 
        if(thisCommand == NULL) {
        	printf("Bad command. Can't have pipe and background task.\n");
        	continue;
        }

        if(haspipe) {
        	if(pipe(pipefd) == -1) {
        		perror("pipe");
        		exit(-1);
        	}
        }

        pid_t ret = fork();

        if (ret == 0) 
        {
        	printf(" READER Started cpid: %d Current pid: %d\n", ret, getpid());
        	if(haspipe) {
                dup2(pipefd[0],0);
                close(pipefd[1]);

        	} else if(strlen(thisCommand[0].infile) > 0) {
        		fd2 = getFileD(thisCommand[0].infile, paths, numPaths, false);
        		if(fd2 == -1) {
        			printf("There was an error opening the input file.\n");
        			// TODO: make sure jobis not in jobs list
        			continue;
        		} else {
                    dup2(fd2,0);
        		}
        	} 


        	if(strlen(thisCommand[0].outfile) > 0) {
        		fd1 = getFileD(thisCommand[0].outfile, paths, numPaths, true);
        		if(fd1 == -1)
        		{
        			printf("There was an error opening or creating the out file.\n");
        		} else {
        			dup2(fd1,1);
        		}	
        	}


        	//printf("READER calling exec: %s\n", thisCommand[0].cmd[0]);
        	

        	execvpe(thisCommand[0].cmd[0], thisCommand[0].cmd, environ);
            //TODO: do we ever execut here?
        	if (fd1 != -1) close(fd1);
        	if (fd2 != -1) close(fd2);
        	exit(1);
        } else if (ret < 0) 
        {

        } else if (haspipe) {
        	printf("Parent here cpid: %d pid: %d\n", ret, getpid());
        	ret = fork();
        	printf("Parent here cpid: %d pid: %d\n", ret, getpid());

        	if(ret ==0) {

	        	if(haspipe) {
                    dup2(pipefd[1],1);
                    close(pipefd[0]);

	        	} else if(strlen(thisCommand[1].outfile) > 0) {

	        		fd1 = getFileD(thisCommand[1].outfile, paths, numPaths, true);
	        		if(fd1 == -1)
	        		{
	        			printf("There was an error opening or creating the out file.\n");
	        		} else {
	        			dup2(fd1,1);
	        		}
	        	}


	        	if(strlen(thisCommand[1].infile) > 0) {

	        		fd2 = getFileD(thisCommand[1].infile, paths, numPaths, false);
	        		if(fd2 == -1) {
	        			printf("There was an error opening the input file.\n");
	        			// TODO: make sure jobis not in jobs list
	        			continue;
	        		} else {
	        			dup2(fd2,0);
	        		}
	        	}

                //TODO: do we ever see this printf? 
                printf("WRITER Started cpid: %d Current pid: %d\n", ret, getpid());
                execvpe(thisCommand[1].cmd[0], thisCommand[1].cmd, environ);

                if (fd1 != -1) close(fd1);
                printf("WRITER exec returned\n");
                exit(2);

            } else {

            	close(pipefd[1]);
		        if(thisCommand[1].isForeground) {
		        	//TODO: does this do anything?
		            printf("This is a forground task\n");
		            waitpid(ret, &status, 0);
		        } else {
		            printf("This is a background task\n");
		            continue;
		        }
            	
            }
            
        }
        if(thisCommand[0].isForeground) {
            printf("This is a forground task\n");
            waitpid(0, &status, 0);
        } else {
            printf("This is a background task\n");
            updatePID(jobs, ret);
        	printJobs(jobs);
            continue;
        }	
        
    }
  
}

void pushJob(struct Job* head, char* thisCmd, bool isRun, bool isRecent, int size, int pid)
{

    struct Job* current = head;

	if(current != NULL) {
        while(current->nextJob != NULL) {
		    current = current->nextJob;
	    }

	    current->nextJob = malloc(sizeof(struct Job));
	    current->nextJob->cmd = thisCmd;
	    current->nextJob->id = size;
	    current->nextJob->pid = pid;
	    current->nextJob->isRunning = isRun;
	    current->nextJob->isMostRecent = isRecent;
	    current->nextJob->nextJob =NULL;
	} else {
		//TODO: Do we get here?
		printf("Push job head equals null\n");
		if(current == NULL) {
			printf("head is still null\n");
		} else {
			printf("head is no longer null\n");
			current->nextJob = NULL;
		    current->cmd = thisCmd;
	        current->id = size;
	        current->pid = pid;
	        current->isRunning = isRun;
	        current->isMostRecent = isRecent;
		}
	}
}

int removeJob(struct Job* jobsList, int pid) {
	struct Job* current = jobsList;
	struct Job* previous = NULL;
    while(current != NULL) {
    	if(current->pid == pid)
    	{
    		printf("Trying to remove job\n");
    		previous->nextJob = current->nextJob;
    		free(current);
    		return pid;
    	}
    	previous=current; 
    	current = current->nextJob;   
    }
    return -1;

}

int removeLastJob(struct Job* jobsList) {
	struct Job* current = jobsList;
	struct Job* previous = NULL;
    while(current != NULL) {
    	previous=current;
    	if(current->nextJob == NULL) {
    		int pid = current->pid;
    		free(current);
    		previous->nextJob = NULL;
    		return pid;
    	} 
    	current = current->nextJob; 
    }
    return -1;
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

struct Command* parseInput(char* buf, char* buf2, bool* haspipe)
{
   
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

	struct Command retCmd = createCommand(buf);

	// Restrict pipe and bg
	if(retCmd.isForeground == false && *haspipe) return NULL;

	struct Command* cmdList;
	struct Command retcmd2;

	if(*haspipe) {
        cmdList = malloc(2 * sizeof(struct Command));
        cmdList[1] = retCmd;
        retcmd2 = createCommand(buf2);
        // Restrict pipe and bg
	    if(retcmd2.isForeground == false && *haspipe) return NULL;    
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

    switch(signal) {
  	    case SIGINT:
  	        signal_name = "SIGINT";
  	        //printf("signal is: %s\n", signal_name);
  	        break;
  	    case SIGTSTP:
  	        signal_name = "SIGTSTP";
  	        //printf("signal is: %s\n", signal_name);
  	        break;
  	    case SIGCHLD:
  	        signal_name = "SIGCHLD";
  	        //printf("signal is: %s\n", signal_name);
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
	//printf("This starting buffer is: %s\n", buf);
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
		//printf("%s\n", token);
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
	//printf("Num commands is now: %d\n", numCmds);
	//printf("Last command is now: %d\n", lastCmd);
	for(int i=0; i<numCmds; i++) {
	   if (strcmp(cmds[i], "<") == 0) {
            infile = cmds[i + 1];
            //printf("Found <\n");
            if(lastCmd == 0) lastCmd = i;
            //printf("Numcmds: %d lastCmd: %d\n", numCmds, lastCmd);
		} else if (strcmp(cmds[i], ">") == 0) {
            outfile = cmds[i + 1];
            //printf("Found >\n");
            if(lastCmd == 0) lastCmd = i;
		} 
	}	
	if (lastCmd < numCmds && lastCmd != 0) numCmds = lastCmd;
	cmds = removeExcess(cmds, numCmds);
    
	struct Command thisCommand = {isRunning, -1, isFor, true, cmds, numCmds, outfile, infile};

	return thisCommand;
}

int getFileD(char* file, char** paths, int num, bool create) {

    int filed ;
    if (access(file, F_OK) != -1) {
    	//printf("File is in current dir.\n");
    	filed = open(file, O_RDWR|O_CREAT, 0777);
    	//printf("The filed is: %d\n", filed);
    	return filed;
    }
    char* filepath;
    for(int i=0; i< num; i++) {
    	filepath = malloc(strlen(paths[i]) + strlen(file) + 2);
        strcpy(filepath, paths[i]);
        strcat(filepath, "/");
        strcat(filepath, file);
    	if(access(filepath, F_OK) != -1) 
    	{
    		filed = open(filepath, O_RDWR|O_CREAT, 0777);
    	    return filed;
    	}
    }
    if(create)
    {	
        filed = open(file, O_RDWR|O_CREAT, 0777);
        return filed;
    }    
	return -1;

}

int updatePID(struct Job* jobsList, int pid) {
	struct Job* current = jobsList;
	struct Job* previous = NULL;
    while(current != NULL) {
    	previous=current;
    	if(current->nextJob == NULL) {
    		current->pid = pid;
    		return pid;
    	} 
    	current = current->nextJob; 
    }
    return -1;	
}

void printJobs(struct Job* jobsList) {
		    //[1] - Running   sleep 5 &
        //[2] - Stopped   sleep 5 &
        //[3] + Running   log_run | grep > out.txt

    struct Job* current = jobsList;
    while(current != NULL) {
    	if(current->cmd != NULL)
    	{
    	    printf("[%d]", current->id);
    	    if(current->isMostRecent) 
    	    	printf(" +");
    	    else
    	    	printf(" -");
    	    if(current->isRunning) printf(" Running ");
    	    printf(" %s", current->cmd);
    	    printf(" PID: %d\n", current->pid);
    	} 
    	current = current->nextJob;   
    }
    printf("Jobslist now empty\n");

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

