#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

#define true 1
#define false 0

extern char** environ;

typedef int bool;

void handleSignal(int);

int main(int argc, char** args) 
{

    signal(SIGTSTP, &handleSignal);
    signal(SIGCHLD, &handleSignal);
    signal(SIGINT, &handleSignal);

    while(1) {
        printf("Sleeping 10\n");
        sleep(10);
    }


}

void handleSignal(int signal) {
    const char* signal_name;
    sigset_t pending;

    switch(signal) {
  	    case SIGINT:
  	        signal_name = "SIGINT";
  	        break;
  	    case SIGTSTP:
            signal_name = "SIGTSTP";
  	        break;
  	    case SIGCHLD:
            signal_name = "SIGCHLD";
  	        break;
        case SIGHUP:
            signal_name ="SIGHUP";
            break;
        case SIGQUIT:
            signal_name = "SIGQUIT";
            break;
        case SIGSTOP:
            signal_name = "SIGSTOP";
            break;
        case SIGCONT:
            signal_name = "SIGCONT";
            break;
  	    default:
  	        printf("Can't find signal.\n");
        printf("The signal was: %s\n", signal_name);
  	    return;

    }
}

//SIGILL
//SIGTRAP
//SIGABRT
//SIGBUS
//SIGFPE
//SIGKILL
//SIGUSR1
//SIGSEGV
//SIGUSR2
//SIGPIPE
//SIGALRM
//SIGTERM
//SIGSTKFLT
//SIGCHLD
//SIGCONT
//SIGSTOP
//SIGTSTP
//SIGTTIN
//SIGTTOU
//SIGURG
//SIGXCPU
//SIGXFSZ
//SIGVTALRM
//SIGPROF
//SIGWINCH
//SIGIO
//SIGPWR
//SIGSYS
//SIGRTMIN
//SIGRTMIN+1







