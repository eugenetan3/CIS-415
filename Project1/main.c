/*
Author: Eugene Tan

Project 1: Pseudo-shell Implementation using Linux System Calls

Date: 4/16/2020

Professor: Allen Malony

Acknowledgments: Jared Hall, Allen Malony, Linux System Manual Page
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "command.h"


/**
Remove leading spaces method.
Used to remove leading spaces at the front of a string by moving the
pointer of the string past all the leading whitespace and then 
incrementing by the amount of space.
*/
void remove_lspaces(char ** line) {
    int i;
    for (i = 0; (*line)[i] == ' '; i++) {
    }
    *line += i;
}


/*
Function Execution Function

Mallocs a table for commands and then performs a split delimited by space
to determine validity of each command segment. Utilizes control statements
to determine error handling and calling of functions.

input: command_block: singular token parsed from read_tokens
output: 0 or 1: 0 for successful execution, 1 for error thrown.
 */
int execute_arg(char *command_block) {
    char **command_args = (char **)malloc(10 * sizeof(char *));
    char *token; 

    /* Catch memory failure */
    if (command_args == NULL) {
        printf("pseudo-shell: allocation failed\n");
        exit(EXIT_FAILURE);
    }

    /* Tokenize by space and tabs */
    int count = 0;
    token = strtok(command_block, " \t");
    
    /* Catch if token is just all spaces or tabs */
    if (token == NULL) {
        printf("Error! Unrecognized command: %s\n", command_block);
        free(command_args);
        return 1;

    } else {
        /* Tokenize the token passed into individual command and parameters */
        while (token != NULL) {
            command_args[count] = token;
            count++;
            token = strtok(NULL , " \t");
        }

        /* Malloc table of valid commands to be called (9 valid commands) */
        char **command_list = (char **)malloc(9 * sizeof(char *));
        command_list[0] = "ls"; //Enter each command into the table as a specific slot
        command_list[1] = "pwd";
        command_list[2] = "mkdir";
        command_list[3] = "cd";
        command_list[4] = "cp";
        command_list[5] = "mv";
        command_list[6] = "rm";
        command_list[7] = "cat";
        command_list[8] = "exit";

        /* For loop to determine which command is to be called based on index # */
        int command = 0;
        for (int i = 0; i < 9; i++) {
            if (strcmp(command_args[0], command_list[i]) == 0) {
                command = i + 1;
                break; //Upon matching, break from the loop
            }
        }
        
        /* Switch structure to access correct command cases given on command integer */
        switch (command) {
            //Case for ls
            case 1:
                //Catch if ls has a duplicate ls behind it
                if (count > 1 && (strcmp(command_args[0], command_args[1]) == 0)) {
                    printf("Error! Incorrect syntax. No control code found.\n");
                    free(command_args);
                    free(command_list);
                    return 1;
                //Catch if there are too many parameters passed
                } else if (count > 1) {
                    
                    printf("Error! Unsupported parameters for command: %s\n", command_args[0]);
                    free(command_args);
                    free(command_list);
                    return 1;
                } else {
                    listDir();
                }
                break;
            //Case for pwd
            case 2:
                //Catch if pwd has a duplicate pwd behind it 
                if (count > 1 && (strcmp(command_args[0], command_args[1]) == 0)) {
                    printf("Error! Incorrect syntax. No control code found.\n");
                    free(command_args);
                    free(command_list);
                    return 1;
                //Catch if there are too many parameters passed
                } else if (count > 1) {
                    printf("Error! Unsupported parameters for command: %s\n", command_args[0]);
                    free(command_args);
                    free(command_list);
                    return 1;
                } else {
                    showCurrentDir();
                }

                break;
            //Case for mkdir    
            case 3: 
                //Catch if there are too many parameters
                if (count > 2) {
                    printf("Error! Incorrect parameters. Too many parameters for command: %s\n", command_args[0]);
                    free(command_args);
                    free(command_list);
                    return 1;
                //Catch if there are too little parameters    
                } else if (count < 2) {
                    printf("Error! Incorrect parameters. Missing parameters for command: %s\n", command_args[0]);
                    free(command_args);
                    free(command_list);
                    return 1;
                //Catch if sufficient parameters but mkdir has a duplicate mkdir behind it    
                } else if (count == 2 && (strcmp(command_args[0], command_args[1]) == 0)) {
                    printf("Error! Incorrect syntax. No control code found.\n");
                    free(command_args);
                    free(command_list);
                    return 1;
                } else {
                    int command = 0;
                    //Catch if any reserved keywords are being used as parameters
                    for (int i = 0; i < 9; i++) {
                        if (strcmp(command_args[1], command_list[i]) == 0) {
                            command = 1;
                        }
                    }
                    //If command is true, reserved keyword is used
                    if (command) {
                        printf("Error! Incorrect syntax. No control code found.\n");
                        free(command_args);
                        free(command_list);
                        return 1;
                    } else {
                        makeDir(command_args[1]);
                    }
                }
                break;
            //Case for cd 
            case 4:
                //Catch if too many parameters are passed
                if (count > 2) {
                    printf("Error! Incorrect parameters. Too many parameters for command: %s\n", command_args[0]);
                    free(command_args);
                    free(command_list);
                    return 1;
                //Catch if missing parameters    
                } else if (count < 2) {
                    printf("Error! Incorrect parameters. Missing parameters for command: %s\n", command_args[0]);
                    free(command_args);
                    free(command_list);
                    return 1;
                //Catch if sufficient parameters but cd has a duplicate cd behind it    
                } else if (count == 2 && (strcmp(command_args[0], command_args[1]) == 0)) {
                    printf("Error! Incorrect syntax. No control code found.\n");
                    free(command_args);
                    free(command_list);
                    return 1;
                //Sufficient parameters
                } else {
                    //Catch if a reserved keyword is being used as cd parameter
                    int command = 0;
                    for (int i = 0; i < 9; i++) {
                        if (strcmp(command_args[1], command_list[i]) == 0) {
                            command = 1;
                        }
                    }

                    //If a reserved keyword is found throw error
                    if (command) {
                        printf("Error! Incorrect syntax. No control code found.\n");
                        free(command_args);
                        free(command_list);
                        return 1;
                    } else {
                        changeDir(command_args[1]);
                    }
                }
            
                break;
            //Case for cp    
            case 5:
                //Catch if too many parameters for cd
                if (count > 3) {
                    printf("Error! Incorrect parameters. Too many parameters for command: %s\n", command_args[0]);
                    free(command_args);
                    free(command_list);
                    return 1;
                //Catch if missing parameters for cd    
                } else if (count < 3) {
                    printf("Error! Incorrect parameters. Missing parameters for command: %s\n", command_args[0]);
                    free(command_args);
                    free(command_list);
                    return 1;
                //Catch if matching command duplicate (repeated command)     
                } else if (count == 3 && (strcmp(command_args[0], command_args[1]) == 0)) {
                    printf("Error! Incorrect syntax. No control code found.\n");
                    free(command_args);
                    free(command_list);
                    return 1;
                } else {
                    //Catch if a reserved keyword is a parameter 
                    int command = 0;
                    for (int i = 0; i < 9; i++) {
                        if (strcmp(command_args[1], command_list[i]) == 0) {
                            command = 1;
                        }
                    }
                    //Catch if a reserved keyword is a parameter
                    int command2 = 0;
                    for (int i = 0; i < 9; i++) {
                        if (strcmp(command_args[2], command_list[i]) == 0) {
                            command2 = 1;
                        }
                    }
                    //If either parameter contains a keyword
                    if (command || command2) {
                        printf("Error! Incorrect syntax. No control code found.\n");
                        free(command_args);
                        free(command_list);
                        return 1;
                    } else {
                        copyFile(command_args[1], command_args[2]);
                    }
                }
                    
            
                break;
            case 6:
                //Case for mv
                //Catch if too many parameters are passed
                if (count > 3) {
                    printf("Error! Incorrect parameters. Too many parameter for command: %s\n", command_args[0]);
                    free(command_args);
                    free(command_list);
                    return 1;
                //Catch if missing parameters    
                } else if (count < 3) {
                    printf("Error! Incorrect parameters. Missing parameters for command: %s\n", command_args[0]);
                    free(command_args);
                    free(command_list);
                    return 1;
                //Catch if duplicate command for repeated command    
                } else if (count == 3 && (strcmp(command_args[0], command_args[1]) == 0)) {
                    printf("Error! Incorrect syntax. No control code found.\n");
                    free(command_args);
                    free(command_list);
                    return 1;
                //Sufficient parameters check parameters    
                } else {
                    //Catch if parameter one is a reserved keyword
                    int command = 0;
                    for (int i = 0; i < 9; i++) {
                        if (strcmp(command_args[1], command_list[i]) == 0) {
                            command = 1;
                        }
                    }
                    //Catch if parameter two is a reserved keyword
                    int command2 = 0;
                    for (int i = 0; i < 9; i++) {
                        if (strcmp(command_args[2], command_list[i]) == 0) {
                            command2 = 1;
                        }
                    }
                    //If one of the parameter is a reserved keyword throw error
                    if (command || command2) {
                        printf("Error! Incorrect syntax. No control code found.\n");
                        free(command_args);
                        free(command_list);
                        return 1;
                    } else {
                        moveFile(command_args[1], command_args[2]);
                    }
                }
                break;
            //Case for rm    
            case 7:
                //Catch if too many parameters are passed
                if (count > 2) {
                    printf("Error! Incorrect parameters. Too many parameters for command: %s\n", command_args[0]);
                    free(command_args);
                    free(command_list);
                    return 1;
                //Catch if missing parameters    
                } else if (count < 2) {
                    printf("Error! Incorrect parameters. Missing parameters for command: %s\n", command_args[0]);
                    free(command_args);
                    free(command_list);
                    return 1;
                //Catch if repeated command without control code    
                } else if (count == 2 && (strcmp(command_args[0], command_args[1]) == 0)) {
                    printf("Error! Incorrect syntax. No control code found.\n");
                    free(command_args);
                    free(command_list);
                    return 1;
                } else {
                    //Sufficient parameters
                    //Catch if a parameter is a reserved keyword 
                    int command = 0;
                    for (int i = 0; i < 9; i++) {
                        if (strcmp(command_args[1], command_list[i]) == 0) {
                            command = 1;
                        }
                    }
                    //If a reserved keyword was found throw an error
                    if (command) {
                        printf("Error! Incorrect syntax. No control code found.\n");
                        free(command_args);
                        free(command_list);
                        return 1;
                    } else {
                        deleteFile(command_args[1]);
                    }
                }
         
                break;
            //Case for cat    
            case 8:
                //Catch for too many parameters
                if (count > 2) {
                    printf("Error! Incorrect parameters. Too many parameters for command: %s\n", command_args[0]);
                    free(command_args);
                    free(command_list);
                    return 1;
                //Catch for missing parameters    
                } else if (count < 2) {

                    printf("Error! Incorrect parameters. Missing parameters for command: %s\n", command_args[0]);
                    free(command_args);
                    free(command_list);
                    return 1;
                //Catch for sufficient parameters but duplicate command for repeated command    
                } else if (count == 2 && (strcmp(command_args[0], command_args[1]) == 0)) {
                    printf("Error! Incorrect syntax. No control code found.\n");
                    free(command_args);
                    free(command_list);
                    return 1;
                //Sufficient parameters    
                } else {
                    //Catch if a parameter is a reserved keyword
                    int command = 0;
                    for (int i = 0; i < 9; i++) {
                        if (strcmp(command_args[1], command_list[i]) == 0) {
                            command = 1;
                        }
                    }
                    //If a parameter is reserved keyword throw an error
                    if (command) {
                        printf("Error! Incorrect syntax. No control code found.\n");
                        free(command_args);
                        free(command_list);
                        return 1;
                    } else {
                        displayFile(command_args[1]);
                    }
                }
                break;
            //Case for exit
            case 9:
                //Catch if too many parameters 
                if (count > 1) {
                    printf("Error! Incorrect parameters. Too many parameters for command: %s\n", command_args[0]);
                    free(command_args);
                    free(command_list);
                    return 1;
                }
                break;
            //Catch if a command does not match any command within the command table
            default:
                printf("Error! Unrecognized command: %s\n", command_args[0]);
                free(command_args);
                free(command_list);
                return 1;
        }
        free(command_list);
    }


    free(command_args);
    return 0;
}


/*
Parse Tokens Function
Function to accept a line input, a buffersize to store bufsize many items
within an array. Function mallocs an array of strings and returns the number
of tokens parsed and an array of parsed string tokens. Tokenization within
this function occurs by splitting by the delimiter ;.

Input: line: line to be split into tokens
       bufsize: number of spaces to allocate into the array
       numItems: integer to hold number of items that exist in malloced array

Output: parsed_args: array of char * tokens that were parsed from the line
        (numItems) : value is retained by pass by reference.
*/
char **read_tokens(char *line, size_t bufsize, int *numItems) {
    //Malloc char ** to hold parsed tokens
    char **parsed_args = (char **)malloc(bufsize * sizeof(char *));
    char *token;

    //Error out if malloc fails
    if (parsed_args == NULL) {
        fprintf(stderr, "pseudo-shell: allocation failed\n");
        exit(EXIT_FAILURE);
    }
    //Count how many items are processed by counting valid tokens
    int count = 0;
    token = strtok(line, ";");
    
    while (token != NULL) {
        parsed_args[count] = token; //Assign item into array at position count
        count++; //Increment count of items inside array
        token = strtok(NULL, ";");
    }

    (*numItems) = count; //Dereference count
    return parsed_args; //Return array of parsed tokens.
}


/*Primary Program Driver*/
int main(int argc, char **argv) {
    setbuf(stdout, NULL); //Disable buffer
    FILE *fp = stdin;
    int f_flag = 0; //Flag to indicate if file mode is activated

    //For loop to catch if -f flag is specified or not
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') { // '-' detected
            for (int j = 1; argv[i][j] != '\0'; j++) {
                if (argv[i][j] == 'f') { // '-f' detected
                    f_flag = 1;
                } else {
                    fprintf(stderr, "pseudo-shell: illegal option -- %c\n", argv[i][j]);
                    exit(EXIT_FAILURE);
                }
            }
        } else {
            break;
        }
    }
    //Check if file mode has been activated
    if (f_flag) { //File mode activated
        fp = fopen(argv[2], "r");
        if (fp != NULL) {
            char *buffer = NULL; //Set null for getline to dynamically allocate
            size_t bufsize = 0; //Set to 0 for getline to dynamically allocate
            FILE *output = freopen("output.txt", "w", stdout); //Redirect stdout to output.txt

            /* Loop through and read through the input file until no more lines */
            while(getline(&buffer, &bufsize, fp) != -1) { 
                int length = strlen(buffer); //Find length of the string
                if (length > 0) {
                    buffer[length - 1] = '\0'; //Replace \n with null token 
                }
                
                //Catch leading control code
                if (buffer[0] == ';') {
                    printf("Error! Incorrect syntax, control code leading.\n");
                    continue; // Skip processing rest of line
                }

                //Catch if input line is just "exit"
                if (strcmp(buffer, "exit") == 0) {
                    break;
                }
                
                //Catch trailing ;
                int trailcode = 0;
                if (buffer[length - 2] == ';') {
                    trailcode = 1;
                }

                int count; // number of items in array
                int exittok = 0; //Check if exit command was passed in a command line
                int errortok = 0; //Check if an error was thrown
                char **parsed_args = read_tokens(buffer, bufsize, &count);
                
                /* Loop through number of items inside the parsed argument array */
                for (int i = 0; i < count; i++) {
                    remove_lspaces(&parsed_args[i]); //Remove leading spaces to make direct comparisons for exit easier
                    if ((strcmp(parsed_args[i], "exit") == 0) || (strcmp(parsed_args[i], "exit ") == 0)) {
                        exittok = 1; //exit detected so set exittok = 1
                        break;
                    }
                    if (execute_arg(parsed_args[i])) {
                        //See if an error was encountered during execution
                        errortok = 1; //Error encountered errortok = 1
                        break;      
                    }
                }
                //If no error was encountered during execution and the line has a trailing control code ; then error
                if ((!errortok) && (trailcode)) {
                    printf("Error! Incorrect syntax trailing control code.\n");
                }

                if (exittok) {
                    break;
                }
                free(parsed_args); //Free token array
            }
            fclose(output); //Close output file for freopen
            fclose(fp); //Close input file
            free(buffer); //Free buffer for getline
        } else {
            //Unable to open file exit
            fprintf(stderr, "pseudo-shell: %s: No such file or directory, file mode failure\n", argv[2]);
            fclose(fp);
            exit(EXIT_FAILURE);
        }

    } else {
        //Interactive mode enabled if f_flag is false

        char *buffer = NULL; //Dynamically allocate buffer and bufsize;
        size_t bufsize = 0;
        int exit_condition = 1; //Loop condition
        int numItems;

        /* While exit condition is not encountered continue looping */
        while (exit_condition) { 
            printf(">>> "); //Line prompt
            getline(&buffer, &bufsize, stdin); // Receive a line from stdin
            char *tmpbuffer = strdup(buffer); // Duplicate buffer

            if (strtok(tmpbuffer, " ;\n\t") == NULL) { // Check if line consists of only spaces, tabs, newlines, or semicolons
                free(tmpbuffer);
                continue; //skip line if so
            }

            free(tmpbuffer); //free memory duplicated buffer
            
            int length = strlen(buffer); 

            if (length > 0) {
                buffer[length - 1] = '\0'; //Replaced \n character with null token
            }
            
            //Catch leading ;
            if (buffer[0] == ';') {
                printf("Error! Incorrect syntax, control code leading.\n");
                continue;
            }
            //Catch trailing ;
            int trailcode = 0;
            if (buffer[length - 2] == ';') {
                trailcode = 1;
            }

            //If bufferline is just "exit" we have exit condition
            if (strcmp(buffer, "exit") == 0) {
                exit_condition = 0;
            } else {
                //If line is not exit alone we continue to parse the line.
                int exittok = 0; //Exit command encountered
                int errortok = 0; //Error thrown during execution
                char **parsed_args = read_tokens(buffer, bufsize, &numItems);

                /* Loop through parsed token array */
                for (int i = 0; i < numItems; i++) {
                    remove_lspaces(&parsed_args[i]); //Remove leading spaces so exit comparison is easier in case multiple spaces exist
                    if ((strcmp(parsed_args[i], "exit") == 0) || (strcmp(parsed_args[i], "exit ") == 0)) {
                        exittok = 1; //exittok = 1 if exit command found
                        break;
                    }
                    if (execute_arg(parsed_args[i])) { //True if an error is thrown during execution
                        errortok = 1; //errortok = 1 if an error encountered
                        break;
                    } 
                }
                //During execution of line, no errors were thrown but a trailing ; was detected earlier. 
                if ((!errortok) && (trailcode)) {
                    printf("Error! Incorrect syntax, control trailing.\n");
                }
                free(parsed_args); // free malloced array
                if (exittok) { //If exittok was ticked we exit
                    break;
                }
            }
        }
        free(buffer); //free getline buffer
    }
    return 0;
}
