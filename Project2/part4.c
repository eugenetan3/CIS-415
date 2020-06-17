//Author: Eugene Tan
//Project 2: Part4.c First portion of the MCP Ghost in a Shell Implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int pindex = 0; //Global index for keeping track of current index of array


/* Function to determine how many lines
   exist within the input file or how many
   commands are entered (1 command per line)

   args: inputFile: input file to read from
   ret: numPrograms: number of lines / commands to be expected
*/
int numberPrograms(char *inputFile) {
    FILE *fp = fopen(inputFile, "r"); //Open file
    int numPrograms = 0;
    char *buffer = NULL; //Dynamic allocation of these variables with getline
    size_t bufsize = 0;
    while (getline(&buffer, &bufsize, fp) != -1) {
        numPrograms++; //While getline does not encounter EOF increment numPrograms by 1
    }
    fclose(fp); //Free memory and return number of programs expected.
    free(buffer);
    return numPrograms;
}

/* Function to mimic the top command in the terminal
   by extracting useful information about a specific 
   process ID */
void topStatus(pid_t pid) {

    char pidProcPath[60]; //Buffer for address
    sprintf(pidProcPath, "/proc/%d/stat", pid); //Format the address using the pid
    FILE *procptr; //File pointer for opening the proc folder path found
    procptr = fopen(pidProcPath, "r");
    if (procptr != NULL) {
        char *buffer = NULL; //Dynamically allocate line buffer
        size_t bufsiz = 0;

        //Variables to hold result of proc/stat
        int pidnum;
        char command[BUFSIZ];
        char state;
        int parentpidnum;
        int groupid;
        int sessionid;
        int terminal;
        int foreground;
        unsigned int kernal_flags;
        unsigned long minorfault;
        unsigned long cminorfault;
        unsigned long majorfault;
        unsigned long cmajorfault;
        unsigned long userscheduledtime;
        unsigned long kernalscheduledtime;
        long int childusertime;
        long int childkernaltime;
        long int priority;
        long int nice;
        long int num_threads;
        long int itrealvalue;
        unsigned long long starttime;
        unsigned long virtualmemory;
        long int rss;
        unsigned long rsslim;
        unsigned long startcode;
        unsigned long endcode;
        unsigned long startstack;
        unsigned long kstkeesp;
        unsigned long kstkeip;
        unsigned long signal;
        unsigned long blocked;
        unsigned long sigignore;
        unsigned long sigcatch;
        unsigned long wchan;
        unsigned long nswap;
        unsigned long cnswap;
        int exit_signal;
        int processor;
        unsigned int rt_priority;
        unsigned int policy;
        unsigned long long delayacct_blkio_ticks;
        unsigned long guesttime;
        long int cguesttime;
        unsigned long startdata;
        unsigned long enddata;
        unsigned long start_brk;
        unsigned long arg_start;
        unsigned long arg_end;
        unsigned long envstart;
        unsigned long envend;
        int exit_code;


        getline(&buffer, &bufsiz, procptr); //Get the stat information as a line

        //Scan the line and store in according variable.
        sscanf(buffer, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %ld %llu %lu %ld %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %d %d %u %u %llu %lu %ld %lu %lu %lu %lu %lu %lu %lu %d", &pidnum, command, &state, &parentpidnum, &groupid, &sessionid, &terminal, &foreground, &kernal_flags, &minorfault, &cminorfault, &majorfault, &cmajorfault, &userscheduledtime, &kernalscheduledtime, &childusertime, &childkernaltime, &priority, &nice, &num_threads, &itrealvalue, &starttime, &virtualmemory, &rss, &rsslim, &startcode, &endcode, &startstack, &kstkeesp, &kstkeip, &signal, &blocked, &sigignore, &sigcatch, &wchan, &nswap, &cnswap, &exit_signal, &processor, &rt_priority, &policy, &delayacct_blkio_ticks, &guesttime, &cguesttime, &startdata, &enddata, &start_brk, &arg_start, &arg_end, &envstart, &envend, &exit_code);
        //Print relevant information
        printf("Alive Process ID: %d - Command Process: %s - State: %c - Parent PID: %d - Process Group: %d - Process Priority: %ld - User Time: %ld - Kernal Time: %ld \n", pidnum, command, state, parentpidnum, groupid, priority, userscheduledtime / sysconf(_SC_CLK_TCK), kernalscheduledtime / sysconf(_SC_CLK_TCK)); 
        free(buffer); //Free memory
    }    
    fclose(procptr);
}


/* Process handler function used in lab to send
   the SIGUSR1 signal and any other specified 
   signal to the child processes.

   args: pidargs: array of process id args
         signal: signal being sent to process
         numProgram: number of programs to iterate through to send signal to all
   ret: 0 int 
 */
int processHandler(pid_t *pidargs, int signal, int numProgram) {
    for (int j = 0; j < numProgram; j++) {
         printf("Parent process: %d - Sending signal: %d to child process: %d\n", getppid(), signal, pidargs[j]);
         kill(pidargs[j], signal); //Send signal using kill
    }
    return 0;
}


/* Function to determine if at least one process is alive or not
   to verify if a process is alive.

    args: pid: process id array to be checked
          numPrograms: number of programs to check
    ret: 0 if a process is alive or 1 if a process is dead
*/   
//credit: https://stackoverflow.com/questions/5278582/checking-the-status-of-a-child-process-in-c
int processAlive(pid_t *pid, int numPrograms) { //Figure out how to tell if any of your children are alive
    int wait_pid_status;
    int wait_pid_result;
    int dead = 1;
    int alive = 0;


    //Loop through all processes and find if a single process is still alive
    for (int i = 0; i < numPrograms; i++) {
        wait_pid_result = waitpid(pid[i], &wait_pid_status, WNOHANG); //returns 0 or -1 0 = alive -1 = error / dead
        if (getpgid(pid[i]) >= 0) { //Check if a child is alive or not
            return alive;
        }
    }
    //If reach here all processes dead
    return dead; //All dead
}


/* Function to perform Round Robin scheduling on alive processes
   by stopping and continuing a process with a time quantum of 1
   second

   args: pid: process id array to loop through
         numPrograms: number of programs to use to check for dead processes
   ret: 0 upon failure 1 upon success
*/
//credit: https://stackoverflow.com/questions/31664441/monitor-if-a-process-has-terminated-in-c
//credit: https://stackoverflow.com/questions/5278582/checking-the-status-of-a-child-process-in-c
int signalSchedule(pid_t *pid, int numPrograms) { //RR handler that checks for dead processes and alive processes, is done if all processes are dead after calling SIGSTOP and SIGCONT
    int wait_pid_status;
    int wait_pid_result;
    if (processAlive(pid, numPrograms)) { //Initially check if any children still are alive
        printf("All child processes terminated!\n");
        return 0;
    }
    printf("-------------------------------------------------------\n"); //To separate schedule blocks    
    int sentinel = 1;
    //Loop through all processes in the pid array to find a process that is alive
    //if a process is not alive then we continue to index until we find next alive.
    while (sentinel) {
        wait_pid_result = waitpid(pid[pindex], &wait_pid_status, WNOHANG | WCONTINUED);
        if (getpgid(pid[pindex]) >= 0) { //Check if a child is alive
            printf("Child Process - %d: SIGSTOP signal given. \n", pid[pindex]);
            kill(pid[pindex], SIGSTOP);
            pindex++;
            pindex = pindex % numPrograms;
            while (sentinel) {
                waitpid(pid[pindex], &wait_pid_status, WNOHANG | WCONTINUED);
                if (getpgid(pid[pindex]) >= 0) { //Check if a child is alive
                    printf("Child Process - %d: SIGCONT signal given. \n", pid[pindex]);
                    kill(pid[pindex], SIGCONT);
                    break;
                } else { //Child is dead
                    printf("Child Process - %d: Process has been terminated. \n", pid[pindex]);
                    pindex++;
                    pindex = pindex % numPrograms;
                }
            }
            break;
        } else { //Child is dead
            printf("Child Process - %d: Process has been terminated. \n", pid[pindex]);            
            pindex++;
            pindex = pindex % numPrograms;
        }
    }
    return 1;
}


/* Process the input file and execute each command
   after forking each child process until EOF is
   encountered.

   args: inputFile: file pointer to read from
         numPrograms: number of programs to be expected
   ret: 0 or 1 depending on if the program successfully
        executed or not.
*/
int processInput(FILE *inputFile, int numPrograms) {
    char *buffer = NULL; //Dynamic allocation of these variables with getline
    size_t bufsize = 0;

    pid_t pidArgs[numPrograms]; //Array of process ids of size of numprograms
    int j;
    int i = 0;


    sigset_t set; //Create a set
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1); //Add SIGUSR1 to set
    sigprocmask(SIG_BLOCK, &set, NULL); //Create the bitmask
    while (getline(&buffer, &bufsize, inputFile) != -1) { //Read from input file line by line
        char **commandArgs = (char **)malloc(sizeof(char *) * 100);
        if (commandArgs == NULL) {
            fprintf(stderr, "ERROR: Unable to malloc argument array.\n");
            exit(EXIT_FAILURE);
        }
        for (int j = 0; j < 100; j++) {
            commandArgs[j] = NULL;
        }

        char *token = strtok(buffer, " \n");
        int count = 0;

        if (token != NULL) { //Tokenize command arguments
            commandArgs[0] = token;
            count++;
        }
        while(token != NULL) {
            token = strtok(NULL, " \n");
            commandArgs[count] = token;
            count++;
        }

        pidArgs[i] = fork(); //Fork a child process
       
        if (pidArgs[i] < 0) {
            printf("Parent failed to fork. \n");
            exit(EXIT_FAILURE);
        } 
        if (pidArgs[i] == 0) { //Child forked successfully -> call execvp
            printf("Child Process: %d - Waiting for SIGUSR1 ...\n", getpid());
            j = sigwait(&set, &j); //Wait here until a SIGUSR1 signal is received
            if (j == 0) {
                printf("Child Process: %d - Received SIGUSR1 - Calling exec()\n", getpid());
            }
            if (execvp(commandArgs[0], commandArgs) < 0) {
                //execvp should not reach here unless an error was encountered free memory
                printf("Child (PID %d) failed to execute program %s. \n", getpid(), commandArgs[0]);
                free(buffer);
                free(commandArgs);
                fclose(inputFile);
                exit(-1);
            }
        }
        free(commandArgs);
        i++;
    }
    //Begin sending signals.
    sigaddset(&set, SIGALRM); //Add SIGALRM to the set
    sigprocmask(SIG_BLOCK, &set, NULL); //Add set to bit mask
    sleep(1);
    printf("Sending SIGUSR1 to processes...\n");
    processHandler(pidArgs, SIGUSR1, numPrograms); //Send the SIGUSR1 signal to child processes
    processHandler(pidArgs, SIGSTOP, numPrograms); //Send the SIGSTOP signal to child processes
    printf("Sent SIGSTOP to stop all processes...\n");
    printf("Beginning Round-Robin Time Slicing - Time Quantum: 1 second...\n");
    while(!processAlive(pidArgs, numPrograms)) { //Loop through all alive processes and call SIGCONT SIGSTOP on them until all child processes are dead
        signalSchedule(pidArgs, numPrograms); //Call helper function that loops through pid array calling SIGSTOP and SIGCONT if a process is alive, iterate through pidArgs array if alive process is not encountered.
        alarm(1);
        sigwait(&set, &j);
        for (int i = 0; i < numPrograms; i++) {
            if (getpgid(pidArgs[i]) >= 0) {
                topStatus(pidArgs[i]);
            }
        }
    }    
    //Wait for processes to exit
    printf("Beginning wait for children...\n");
    for (int i = 0; i < numPrograms; i++) {
        printf("Waiting for child (PID %d) to exit.\n", pidArgs[i]);
        waitpid(pidArgs[i], NULL, 0);
    }
    fclose(inputFile); //Free all memory
    free(buffer);
    exit(EXIT_SUCCESS);
}


/* Primary program driver for the MCP programs
   calls primary functions that execute MCP

   args: argc: number of arguments passed into commandline
         argv: arguments passed into commandline as a char array
   ret: 0 or 1 depending on successful execution or not
*/
int main(int argc, char *argv[]) {
    if (argc != 2) { //Check if file argument was passed
        fprintf(stderr, "ERROR: Missing file argument.\n");
        return 1;
    } else {
        FILE *fp = fopen(argv[1], "r"); //Open fileargument
        if (fp == NULL) { //If file is null throw an error, else proceed
            fprintf(stderr, "ERROR: Unable to open file argument: %s", argv[1]);
            return 1;
        } else {
            int numberCommands = numberPrograms(argv[1]); //Calculate number of programs expected
            processInput(fp, numberCommands); //Call the processInput function to execute 
            fclose(fp);
        }
    }
    return 0;
}
