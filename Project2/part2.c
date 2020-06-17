//Author: Eugene Tan
//Project 2: Part2.c First portion of the MCP Ghost in a Shell Implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

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
    fclose(fp); //Free memory and return number of programs expected
    free(buffer);
    return numPrograms;
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
         kill(pidargs[j], signal); 
    }
    return 0;
}

/* Signal handler for the sigaction function to be used
   as a signal handler; incompatible with sigwait */
void signalHandler(int signum, siginfo_t *signalinfo, void *item) {
    write(STDOUT_FILENO,"l\n", 3);
    return;
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

    struct sigaction signaler; //Initialization of sigaction structure
    memset(&signaler, '\0', sizeof(signaler)); 
    signaler.sa_sigaction = &signalHandler; //Specify sigaction signal handler
    signaler.sa_flags = SA_SIGINFO; 

    sigset_t set; //Create a set
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1); //Add SIGUSR1 to set

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
        sigaction(SIGUSR1, &signaler, NULL);
       
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
    sleep(1);
    printf("Sending SIGUSR1 to processes...\n");
    processHandler(pidArgs, SIGUSR1, numPrograms); //Send the SIGUSR1 signal to child processes
    printf("Sending SIGSTOP to processes...\n");
    processHandler(pidArgs, SIGSTOP, numPrograms); //Send the SIGSTOP signal to child processes
    printf("Sending SIGCONT to processes...\n");
    processHandler(pidArgs, SIGCONT, numPrograms); //Send the SIGCONT signal to child processes
    printf("Beginning wait for children...\n");

    //Wait for processes to exit
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
    //FILE *fp;

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
            processInput(fp, numberCommands); //Call the processInput function to execute.           
            fclose(fp);
        }
    }
    return 0;
}
