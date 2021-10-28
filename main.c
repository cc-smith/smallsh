
/*
* Author: Chris Smith
* Program: smallsh.c
* Date: 11/1/2021
* This program is an implementation of a shell in C. The commands "cd", "exit",
* and "status" are built into the shell.  All other commands are passed to 
* exec().
*/

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <math.h> 
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h> 

#define MAX_ARGS 512        // max number of arguments user can enter
#define MAX_LENGTH 2048     // max length of command line input
int sigtstpState = 0;       // flag for storing the state of SIGTSTP

static pid_t exit_pid = 0;  // keeps track of exiting bg processes
static int exit_value = -5; // stores the exit value of bg process


// struct for storing user input 
struct userInput
{
    char* command;
    char* args[MAX_ARGS];
    char* inputFile;
    char* outputFile;
    int background;
};

// Function delcarations
void bgProcessPrint(void);
struct userInput* parseInputString(char* inputString);
char* variableExpansion(char* inputString);
void changeDir(char* newDir);
int redirectIO(char* inputFile, char* outputFile);
void handler(int sig);
void handle_SIGTSTP(int signo);
void storePidInArray(pid_t pid, int* pidArray);
int execCommand(struct userInput* parsedInput, int* pidArray);

int main(void) {

    // Initialize sigaction struct
    struct sigaction SIGINT_action = { 0 }, SIGTSTP_action = { 0 };

    // set the sa_handler to ignore SIGINT and use handler function for STGTSTP
    SIGINT_action.sa_handler = SIG_IGN;
    SIGTSTP_action.sa_handler = handle_SIGTSTP;

    // Set flags
    SIGINT_action.sa_flags = 0;             // No flags set for SIGINT
    SIGTSTP_action.sa_flags = SA_RESTART;   // Auto restart of system calls

    // Install our signal handlers
    sigaction(SIGINT, &SIGINT_action, NULL);
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);


    // Allocate space to store user input string 
    char* inputString;
    inputString = malloc(MAX_LENGTH * sizeof(char));
    memset(inputString, '\0', MAX_LENGTH);

    // Initialize struct where the parsed user input will be stored
    struct userInput* parsedInput;

    // Allocate space for array which stores process ids
    int pidArray[1000];
    memset(pidArray, -1, sizeof(pidArray));

    int exitValue = 0;  // stores the exit code value for most recent fg process


    /* 
    * Display command line prompt to user, parse user input, redirect I/O,
    * and execute commands
    */ 
    for (;;) {
        // Check if any background processes have ended and notify user
        bgProcessPrint();

        // Get input from user
        printf(": ");
        fflush(stdout);
        fgets(inputString, MAX_LENGTH, stdin);

        // Ignore comments ("#") and blank lines 
        if (strncmp(inputString, "#", 1) == 0 || inputString[0] == '\n') {
            continue;
        }

        // Transform any expansion variables ("$$") in the input
        char* expandedString = variableExpansion(inputString);

        // Parse the input and store in a userInput struct
        parsedInput = parseInputString(expandedString);

        // Execute built in command: exit
        if (strcmp(parsedInput->command, "exit") == 0) {
            int i = 0;
            // Loop throught the array of PID's and kill each process
            while (pidArray[i] != -1) {
                //printf("pid%i: %i\n", i, pidArray[i]);
                int result = kill(pidArray[i], SIGKILL);
                i++;
            }
            // Exit shell
            return EXIT_SUCCESS;
        }

        // Execute built in command: cd
        else if (strcmp(parsedInput->command, "cd") == 0) {
            changeDir(parsedInput->args[1]);
        }

        // Execute built in command: status
        else if (strcmp(parsedInput->command, "status") == 0) {

            // Process exited normally, display exit value
            if (exitValue <= 1) {
                printf("exit value %i\n", exitValue);
                fflush(stdout);

            // Process was terminated by a signal, display signal value
            }
            else {
                printf("terminated by signal %i\n", exitValue);
                fflush(stdout);
            }
        }

        // Handle input/output redirection
        else if (parsedInput->inputFile || parsedInput->outputFile) {

            // Redirect stdin and stdout
            int saved_stdin = dup(0);
            int saved_stdout = dup(1);

            // If I/O redirection was succcessful, execute the command
            exitValue = redirectIO(parsedInput->inputFile, parsedInput->outputFile);
            if (exitValue == 0) {
                exitValue = execCommand(parsedInput, pidArray);
            }

            // Reset stdin and stdout to default values
            dup2(saved_stdin, 0);
            dup2(saved_stdout, 1);
            close(saved_stdin);
            close(saved_stdout);
        }

        // Execute all other commands by passing command to exec function
        else {
            // Ignore the background operator ("&") if shell is in fg only mode
            if ((sigtstpState == 1) && (parsedInput->background == 1)) {
                parsedInput->background = 0;
            }
            // Execute the command
            exitValue = execCommand(parsedInput, pidArray);
        }
    }
}

/* 
 * Function: parseInputString
 * Parses a command line string and stores it in a userInput struct
 * 
 * inputString: the command line string entered by the user
 * returns: a userInput struct
*/
struct userInput* parseInputString(char* inputString) {

    // Initialize a struct in which to store the parsed user input
    struct userInput* currInput = malloc(sizeof(struct userInput));

    // Make a copy of the input string for strtok_r calls
    char inputCopy[MAX_LENGTH];
    strcpy(inputCopy, inputString);

    // Check if command is to be executed in the background
    char backgroundFlag[1];
    backgroundFlag[0] = inputCopy[strlen(inputCopy) - 2];
    // If "&" detected in user input, set backgroud flag
    if (strncmp(backgroundFlag, "&", 1) == 0) {
        currInput->background = 1;
    }
    else {
        currInput->background = 0;
    }

    // For use with strtok_r
    char* saveptr;

    // The first token is the command
    char* token = strtok_r(inputString, " \n", &saveptr);
    currInput->command = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currInput->command, token);

    // Store the command in the list of arguments (for execvp call)
    currInput->args[0] = malloc(strlen(token) + 1);
    strcpy(currInput->args[0], token);

    // The next token is the arguments
    if (strncmp(saveptr, "<", 1) != 0 && strncmp(saveptr, ">", 1) != 0 && 
        strncmp(saveptr, "&", 1) != 0) {
        char* saveptr2;
        token = strtok_r(NULL, "<>&\n", &saveptr);

        // Get all the arguments and parse them into a list of lists
        if (token) {
            // Store first argument
            int i = 1;
            char* token2 = strtok_r(token, " \n", &saveptr2);
            currInput->args[i] = malloc(strlen(token2) + 1);
            strcpy(currInput->args[i], token2);
            i++;

            // Store remaining arguments
            token2 = strtok_r(NULL, " \n", &saveptr2);
            while (token2 != NULL) {
                currInput->args[i] = malloc(strlen(token2) + 1);
                strcpy(currInput->args[i], token2);
                token2 = strtok_r(NULL, " \n", &saveptr2);
                i++;
            }
        }
    }

    // The next tokens are input files and/or output files
    token = strtok_r(inputCopy, " \n", &saveptr);
    while (token != NULL) {

        // Token is an input file
        if (strncmp(token, "<", 1) == 0) {
            token = strtok_r(NULL, " \n", &saveptr);
            currInput->inputFile = calloc(strlen(token) + 1, sizeof(char));
            strcpy(currInput->inputFile, token);
        }
        // Token is an output file
        else if (strncmp(token, ">", 1) == 0) {
            token = strtok_r(NULL, " \n", &saveptr);
            currInput->outputFile = calloc(strlen(token) + 1, sizeof(char));
            strcpy(currInput->outputFile, token);
        }
        // Get next token 
        token = strtok_r(NULL, " \n", &saveptr);
    }

    return currInput;
}

/*
 * Function: bgProcessPrint
 * Prints notification to user when a background process has ended
 *
 * returns: none
*/
void bgProcessPrint(void) {

    // background process has ended
    if (exit_pid != 0) {
        // process was terminated by a signal
        if (exit_value > 1) {
            printf(
                "background pid %d is done: terminated by signal %d\n",
                exit_pid,
                exit_value
            );
        }
        // process eiited
        else {
            printf(
                "background pid %d is done: exit value %d\n",
                exit_pid,
                exit_value
            );
        }
        fflush(stdout);

        // reset the global variables
        exit_pid = 0;
        exit_value = -5;
    }
}

/*
 * Function: variableExpansion
 * Searches a command line string for "$$" and peforms variable expansion
 * 
 * inputString: the command line string entered by the user
 * returns: a string with expanded variables
*/
char* variableExpansion(char* inputString) {
    // Initialize character pointers 
    char* ptr1;
    char* ptr2;
    ptr1 = inputString;
    ptr2 = inputString + 1;
    char* var = "$";

    // Initialize the expanded string
    char* expandedString;
    expandedString = malloc(2048 * sizeof(char));
    memset(expandedString, '\0', 2048);

    // Store the process ID
    int pid = getpid();
    char buffer[sizeof(int) * 8 + 1];
    sprintf(buffer, "%i", pid);

    int i;
    int pos = 0;    // Keeps track of current position in expandedS

    /* 
    * Using 2 pointers, check each character in the stringand check
    * for instances of "$$"
    */ 
    for (i = 0; i < strlen(inputString); i++) {
        // Variable $$ found
        if(*ptr1 == *var && *ptr2 == *var) {
            ptr1 += 2;
            ptr2 += 2;
            strcat(expandedString, buffer); // copy the PID to the expanded string
            pos += strlen(buffer);  // increment counter by length of PID
        }
        // Variable $$ not found, increment pointer and counter by 1
        else {
            expandedString[pos] = *ptr1;    // Copy char to expanded string
            pos++;
            ptr1++;
            ptr2++;
        }
    }
    return expandedString;
}

/*
 * Function: changeDir
 * Changes the current directory (implementation of "cd" command)
 * 
 * newDir: the path of the desired directory
 * returns: none
*/
void changeDir(char* newDir) {
    char* dir;

    // If user did not provide a path, set the path to HOME
    newDir ? (dir = newDir) : (dir = getenv("HOME"));

    // Path is not valid
    if (chdir(dir) != 0){
        printf("%s: no such file or directory", dir);
        fflush(stdout);
        return;
    }
    
    return;
}

/*
 * Function: redirectIO
 * Implements input and output redirection
 * 
 * inputFile: the name of the desired input file
 * outputFile: the name of the desired output file
 * returns: (int) 0 for successful redirection, 1 for all others
*/
int redirectIO(char* inputFile, char* outputFile) {

    if (inputFile) {
        // Open source file for reading
        int sourceFD = open(inputFile, O_RDONLY);
        if (sourceFD == -1) {
            printf("cannot open %s for input\n", inputFile);
            fflush(stdout);
            return 1;
        }

        // Redirect stdin to source file
        int result = dup2(sourceFD, 0);
        if (result == -1) {
            perror("source dup2()");
            return 1;
        }
    }

    if (outputFile) {
        // Open target file
        int targetFD = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (targetFD == -1) {
            printf("cannot open %s for output", outputFile);
            fflush(stdout);
            return 1;
        }

        // Redirect stdout to target file
        int result = dup2(targetFD, 1);
        if (result == -1) {
            perror("target dup2()");
            return 1;
        }
    }

    // The stdin and stdout are pointing to files
    return(0);
}

/*
 * Function: handler
 * Waits for background child processes to end and notifies the user when a 
 * a process has exited or was termintated by a signal
 * 
 * returns: none
*/
void handler(int sig)
{
    int status;  // store the exit value of a child process

    // get he pid of any background child process that ends
    pid_t pid = waitpid(-1, &status, WNOHANG);

    // pid is a from a child process
    if (pid > 0) {

        // process has exited, set the global variables
        if (WIFEXITED(status)) {
            exit_pid = pid;
            exit_value = WEXITSTATUS(status);

        }

        // process was terminated by a signal, set the global variables
        else if (WIFSIGNALED(status)) {
            exit_pid = pid;
            exit_value = WTERMSIG(status);
        }
    }
}

/*
 * Function: handle_SIGTSTP
 * Notifies user when shell has entered/exited foreground only mode
 * 
 * returns: none
*/
void handle_SIGTSTP(int signo) {

    // Notify user that the shell is in foreground only mode
    if (sigtstpState == 0) {
        char* message = "\nEntering foreground-only mode (& is now ignored)\n";
        sigtstpState = 1;   // update the SIGTSTP flag
        write(STDOUT_FILENO, message, 50);
    }

    // Notify user that the shell has exited foreground only mode
    else {
        char* message = "\nExiting foreground-only mode\n";
        sigtstpState = 0;   // reset the SIGTSTP flag
        write(STDOUT_FILENO, message, 30);
    }

    return;
}

/*
 * Function: storePidInArray
 * Stores a PID in an array. A PID is added to the array every time a new 
 * process is forked. Upone exit from the shell, the PID
 * 
 * pid: the PID to be stored
 * outputFile: the name of the desired output file
 * returns: (int) 0 for successful redirection, 1 for all others
*/
void storePidInArray(pid_t pid, int *pidArray) {
    int i = 0;
    while (pidArray[i] != -1) {
        i++;
    }
    pidArray[i] = pid;
    return;
}

/*
 * Function: execCommand
 * Forks a child process and executes a given command.
 *
 * parsedInput: the parsed input from the user
 * pidArray: the array in which PIDs are stored
 * returns: (int) the exit or termination value of the child process
*/
int execCommand(struct userInput* parsedInput, int *pidArray) {
    int pid;
    pid_t spawnPid = -5;
    int childExitMethod = -5;
    int childStatus;

    // Initialize the sigation structure and set conditions for bg/fg processes
    struct sigaction SIGINT_action = { 0 };
    // For background processes, ignore ctrl+c
    if (parsedInput->background == 1) {
        signal(SIGCHLD, handler);
        // set the sa_handler to ignore SIGINT
        SIGINT_action.sa_handler = SIG_IGN;
        // No flags set
        SIGINT_action.sa_flags = 0;
    }
     // For foreground processes, do not ignore ctrl+c
    else {
        // Set the sa_handler to the default action for SIGINT
        SIGINT_action.sa_handler = SIG_DFL;
        // Block all catchable signals while handle_SIGINT is running
        sigfillset(&SIGINT_action.sa_mask);
        // No flags set
        SIGINT_action.sa_flags = 0;
    }
    // Install our signal handler
    sigaction(SIGINT, &SIGINT_action, NULL);

    // Fork a child process
    spawnPid = fork();

    switch (spawnPid) {
    // Fork error
    case -1:
        perror("fork()\n");
        return 1;
        break;

    // In the child process
    case 0:
        // Get the pid and display to the user
        pid = getpid();
        if (parsedInput->background == 1) {
            printf("background pid is %d\n", pid);
            fflush(stdout);
        }

        // Execute the command
        execvp(parsedInput->command, parsedInput->args);

        // Exec error
        printf("%s: no such file or directory\n", parsedInput->command);
        fflush(stdout);
        exit(1);

        return 1;
        break;

    // In the parent process
    default:

        pid = getpid();
        storePidInArray(spawnPid, pidArray);
        // set the sa_handler to ignore SIGINT
        SIGINT_action.sa_handler = SIG_IGN;
        // No flags set
        SIGINT_action.sa_flags = 0;
        // Install our signal handler
        sigaction(SIGINT, &SIGINT_action, NULL);

        // Wait for child's termination if child is running in foreground
        if (parsedInput->background != 1) {
            spawnPid = waitpid(spawnPid, &childStatus, 0);
            storePidInArray(spawnPid, pidArray);    // store the PID

            // Notify user if the child process was terminated by a signal
            if (WIFSIGNALED(childStatus)) {
                printf("terminated by signal %d\n", WTERMSIG(childStatus));
                fflush(stdout);
                return WTERMSIG(childStatus);
            }
            // Notify the user if the child process exited
            else if (WIFEXITED(childStatus)) {
                return WEXITSTATUS(childStatus);
            }
        }
        // Store background PID in the array
        storePidInArray(spawnPid, pidArray);
        break;
    }
}

