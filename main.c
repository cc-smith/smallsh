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
int sigtstpState = 0;
pid_t current_pid = 0;

/* struct for user input */
struct userInput
{
    char* command;
    char* args[512];
    char* inputFile;
    char* outputFile;
    int background;
};

void printStruct(struct userInput* input) {
    printf("\n\n--------------------\nThis is the struct:\n\n"
        "command: %s\n"
        "inputFile: %s\n"
        "outputFile: %s\n"
        "background: %i\n",
        input->command,
        input->inputFile,
        input->outputFile,
        input->background);
    int i = 0;
    while (input->args[i] != NULL) {
        printf("arg%i: %s\n", i, input->args[i]);
        i++;
    }
    printf("--------------------\n\n");
    return;
}

struct process
{
    pid_t pid;
    struct process* next;
};
struct process* storePid(int pid, struct process* head, struct process* tail)
{

    // Get a new movie node corresponding to the current line
    struct process* curProcess = malloc(sizeof(struct process));
    curProcess->pid = pid;
    curProcess->next = NULL;




    // Is this the first node in the linked list?
    if (head == NULL)
    {
        // This is the first node in the linked link
        // Set the head and the tail to this node
        head = curProcess;
        tail = curProcess;
    }
    else
    {
        // This is not the first node.
        // Add this node to the list and advance the tail
        tail->next = curProcess;
        tail = curProcess;
    }
    printf("\n\nHEAD: %i", head->pid);
    return head;
}
//int execCommand2(struct userInput* parsedInput, struct process* head, struct process* tail) {
//
//
//
//    pid_t spawnPid = -5;
//    int childExitMethod = -5;
//    int childStatus;
//
//    struct sigaction SIGINT_action = { 0 };
//    // For background processes, ignore ctrl+c
//    if (parsedInput->background == 1) {
//        signal(SIGCHLD, handler);
//        // set the sa_handler to ignore SIGINT
//        SIGINT_action.sa_handler = SIG_IGN;
//        // No flags set
//        SIGINT_action.sa_flags = 0;
//        // Install our signal handler
//        sigaction(SIGINT, &SIGINT_action, NULL);
//    }
//
//    // For foreground processes, do not ignore ctrl+c
//    else {
//        // Fill out the SIGINT_action struct
//        // Register handle_SIGINT as the signal handler
//        SIGINT_action.sa_handler = handle_SIGINT;
//        // Block all catchable signals while handle_SIGINT is running
//        sigfillset(&SIGINT_action.sa_mask);
//        // No flags set
//        SIGINT_action.sa_flags = 0;
//        sigaction(SIGINT, &SIGINT_action, NULL);
//    }
//
//    spawnPid = fork();
//    int pid;
//
//
//    switch (spawnPid) {
//    case -1:
//        perror("fork()\n");
//        return 1;
//        break;
//
//    case 0:
//
//        pid = getpid();
//        //storePid(pid);
//
//        if (parsedInput->background == 1) {
//            printf("background pid is %d\n", pid);
//        }
//
//        execvp(parsedInput->command, parsedInput->args);
//        printf("%s: no such file or directory\n", parsedInput->command);
//
//        return 1;
//        break;
//
//    default:
//        // In the parent process
//        // Wait for child's termination
//        if (parsedInput->background != 1) {
//            spawnPid = waitpid(spawnPid, &childStatus, 0);
//
//            if (WIFEXITED(childStatus)) {
//                return WEXITSTATUS(childStatus);
//            }
//            else if (WIFSTOPPED(childStatus)) {
//                //printf("terminated by signal %d\n", WSTOPSIG(childStatus));
//                return WSTOPSIG(childStatus);
//            }
//
//        }
//
//
//        //else {
//        //    do {
//        //        spawnPid = waitpid(spawnPid, &childStatus, WNOHANG);
//        //    } while (!WIFEXITED(childStatus) && !WIFSIGNALED(childStatus));
//        //    if (WIFEXITED(childStatus)) {
//        //        printf("exited, status=%d\n", WEXITSTATUS(childStatus));
//        //    }
//        //    /*spawnPid = waitpid(spawnPid, &childStatus, WNOHANG);
//        //    while (spawnPid == 0) {
//        //        spawnPid = waitpid(spawnPid, &childStatus, WNOHANG);
//        //    }
//        //    printf("PARENT(%d): child(%d) terminated. Exiting\n", getpid(), spawnPid);*/
//        //  /*  while (spawnPid == 0) {
//        //        spawnPid = waitpid(spawnPid, &childStatus, WNOHANG);
//
//        //    }*/
//        //}
//        break;
//    }
//
//    return pid;
//}

//void printProcesses(struct process* p) {
//    printf("****p->pid", p->pid);
//    while (p != NULL) {
//        printf("\n\n--------------------\nProcesses:\n\n"
//            "pid: %i\n",
//            p->pid);
//        p = p->next;
//    }
//
//    printf("--------------------\n\n");
//    return;
//}

struct userInput* parseInputString(char* inputString) {

    // Initialize a struct in which to store the parsed user input
    struct userInput* currInput = malloc(sizeof(struct userInput));

    // Make a copy of the input string for strtok_r calls
    char inputCopy[2048];
    strcpy(inputCopy, inputString);

    // Check if command to be executed in the background
    char backgroundFlag[1];
    backgroundFlag[0] = inputCopy[strlen(inputCopy) - 2];
    if (strncmp(backgroundFlag, "&", 1) == 0) {
        currInput->background = 1; // "&" detected, set the background process flag to true
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
    if (strncmp(saveptr, "<", 1) != 0 && strncmp(saveptr, ">", 1) != 0 && strncmp(saveptr, "&", 1) != 0) {
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

    // Using 2 pointers, check each character in the string and check for instances of "$$"
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
 /*   printf("\nbuffer: %s", buffer);
    printf("\nEXPANDED STRING: %s", expandedString);*/
    return expandedString;
}

//stackoverflow.com/questions/298510/how-to-get-the-current-directory-in-a-c-program
void changeDir(char* newDir) {
    char* dir;
    char cwd[2048];
    memset(cwd, '\0', 2048); 
    getcwd(cwd, sizeof(cwd));

    // User did not specify path, change directory to HOME
    if (!newDir) {
        dir = getenv("HOME");
    }

    // User specified path
    else {
        dir = newDir;
    }

    // Path is not valid
    if (chdir(dir) != 0){
        printf("%s: no such file or directory", dir);
        fflush(stdout);
        return;
    }

    // Return current working dircectory
    getcwd(cwd, sizeof(cwd));
    return;
}

// Exploration: Processes and I/O
int redirectIO(char* inputFile, char* outputFile) {

    if (inputFile) {
        // Open source file
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




void handler(int sig)
{
    //pid_t pid = wait(NULL);
    int status;

    pid_t pid = waitpid(-1, &status, WNOHANG);

    if (pid != -1) {
        if (WIFEXITED(status)) {
            int es = WEXITSTATUS(status);
            printf("background pid %d is done: exit value %d\n", pid, es);
            fflush(stdout);
        }
        else if (WIFSIGNALED(status)) {
            printf("background pid %d is done: terminated by signal %d\n", pid, WTERMSIG(status));
            fflush(stdout);
        }

    }

    /* WARNING : to show the call of the handler, do not do that
       in a 'real' code, we are in a handler of a signal */
    //printf("Child pid %i ended (signal %i)\n", pid, sig);

    /* does 'something' to allow the parent to know chpid
       terminated in a way compatible with parent requirement */
}

//void handle_SIGINT(int signo) {
//    char* message = "terminated by signal 2\n";
//    write(STDOUT_FILENO, message, 23);
//    printf("current_pid: %i", current_pid);
//    kill(current_pid, SIGINT);
//}

void handle_SIGTSTP(int signo) {
    if (sigtstpState == 0) {
        char* message = "\nEntering foreground-only mode (& is now ignored)\n";
        sigtstpState = 1;
        write(STDOUT_FILENO, message, 50);
    }
    else {
        char* message = "\nExiting foreground-only mode\n";
        sigtstpState = 0;
        write(STDOUT_FILENO, message, 30);
    }

    return;
}

void storePidInArray(pid_t pid, int *pidArray) {
    int i = 0;
    while (pidArray[i] != -1) {
        i++;
    }
    pidArray[i] = pid;
    return;
}

int execCommand(struct userInput* parsedInput, int *pidArray) {

    pid_t spawnPid = -5;
    int childExitMethod = -5;
    int childStatus;

    struct sigaction SIGINT_action = { 0 };
    // For background processes, ignore ctrl+c
    if (parsedInput->background == 1) {
        signal(SIGCHLD, handler);
        // set the sa_handler to ignore SIGINT
        SIGINT_action.sa_handler = SIG_IGN;
        // No flags set
        SIGINT_action.sa_flags = 0;
        // Install our signal handler
        sigaction(SIGINT, &SIGINT_action, NULL);
    }

     //For foreground processes, do not ignore ctrl+c
    else {
        // Fill out the SIGINT_action struct
        // Register handle_SIGINT as the signal handler
        SIGINT_action.sa_handler = SIG_DFL;
        // Block all catchable signals while handle_SIGINT is running
        sigfillset(&SIGINT_action.sa_mask);
        // No flags set
        SIGINT_action.sa_flags = 0;
        sigaction(SIGINT, &SIGINT_action, NULL);
    }

    spawnPid = fork();

    if (spawnPid > 0) {
        current_pid = spawnPid;
    }

    int pid;


    switch (spawnPid) {
    case -1:
        perror("fork()\n");
        return 1;
        break;

    case 0:

        pid = getpid();
        if (parsedInput->background == 1) {
            printf("background pid is %d\n", pid);
            fflush(stdout);
        }
        execvp(parsedInput->command, parsedInput->args);
        fflush(stdout);
        //fprintf(stderr, "%s", parsedInput->command);
        //perror("");
        printf("%s: no such file or directory\n", parsedInput->command);
        fflush(stdout);

        return 1;
        break;

    default:

        // set the sa_handler to ignore SIGINT
        SIGINT_action.sa_handler = SIG_IGN;
        // No flags set
        SIGINT_action.sa_flags = 0;
        // Install our signal handler
        sigaction(SIGINT, &SIGINT_action, NULL);

        // In the parent process
        // Wait for child's termination
        if (parsedInput->background != 1) {
            spawnPid = waitpid(spawnPid, &childStatus, 0);

       

            storePidInArray(spawnPid, pidArray);
            if (WIFSIGNALED(childStatus)) {
                printf("terminated by signal %d\n", WTERMSIG(childStatus));
            }
            else if (WIFEXITED(childStatus)) {
                return WEXITSTATUS(childStatus);
            }
         

        }
        storePidInArray(spawnPid, pidArray);



        //else {
        //    do {
        //        spawnPid = waitpid(spawnPid, &childStatus, WNOHANG);
        //    } while (!WIFEXITED(childStatus) && !WIFSIGNALED(childStatus));
        //    if (WIFEXITED(childStatus)) {
        //        printf("exited, status=%d\n", WEXITSTATUS(childStatus));
        //    }
        //    /*spawnPid = waitpid(spawnPid, &childStatus, WNOHANG);
        //    while (spawnPid == 0) {
        //        spawnPid = waitpid(spawnPid, &childStatus, WNOHANG);
        //    }
        //    printf("PARENT(%d): child(%d) terminated. Exiting\n", getpid(), spawnPid);*/
        //  /*  while (spawnPid == 0) {
        //        spawnPid = waitpid(spawnPid, &childStatus, WNOHANG);

        //    }*/
        //}
        break;
    }

    return pid;
}


int main(void) {

    // Initialize sigaction struct
    struct sigaction SIGINT_action = { 0 }, SIGTSTP_action = { 0 };




    // set the sa_handler to ignore SIGINT
    SIGINT_action.sa_handler = SIG_IGN;
    // No flags set
    SIGINT_action.sa_flags = 0;
    // Install our signal handler
    sigaction(SIGINT, &SIGINT_action, NULL);
  
    // SIGTSTP
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    // Block all catchable signals while handle_SIGUSR2 is running
    sigfillset(&SIGTSTP_action.sa_mask);
    // Set auto restart of interrupted system calls
    SIGTSTP_action.sa_flags = SA_RESTART;
    // Install our signal handler
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);



    struct userInput* parsedInput;
    char* inputString;
    inputString = malloc(2048 * sizeof(char));
    memset(inputString, '\0', 2048);

    // Initialize new linked list of processes
    // The head of the linked list
    struct process* head = malloc(sizeof(struct process));
    head = NULL;
    // The tail of the linked list
    struct process* tail = malloc(sizeof(struct process));
    tail = NULL;

    int pidArray[1000];
    memset(pidArray, -1, sizeof(pidArray));



    pid_t pid;
    int status;
    int exitValue = 0;

    for(;;) {
   /*     int i = 0;
        while (pidArray[i] != -1) {
            printf("\n**pid%i: %i\n", i, pidArray[i]);
            i++;
        }*/

       /* pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0)
            printf("background pid %d is done: exit value %d\n", pid, status);*/

        // Get input from user
        printf(": ");
        fflush(stdout);
        fgets(inputString, 2048, stdin);

        // Ignore comments ("#") and blank lines 
        if (strncmp(inputString, "#", 1) == 0 || inputString[0] == '\n') {
            continue;
        }

        // Transform any expansion variables ("$$") in the input
        char* expandedString = variableExpansion(inputString);

        // Parse the input into a struct
        parsedInput = parseInputString(expandedString);
        //printStruct(parsedInput);

        // Built in command: exit
        if (strcmp(parsedInput->command, "exit") == 0) {
            //printProcesses(head);
            int i = 0;
            while (pidArray[i] != -1) {
                int result = kill(pidArray[i], SIGKILL);
                //printf("\npid%i: %i  ->  %i", i, pidArray[i], result);
                i++;
            }
            //printf("\n");
            return EXIT_SUCCESS;
        }

        // Built in command: cd
        else if (strcmp(parsedInput->command, "cd") == 0) {
            changeDir(parsedInput->args[1]);
        }

        // Built in command: status
        else if (strcmp(parsedInput->command, "status") == 0) {
            if (exitValue <= 1) {
                printf("exit value %i\n", exitValue);
                fflush(stdout);

            }
            else {
                printf("terminated by signal %i\n", exitValue);
                fflush(stdout);
            }
        }

        // Handle input/output redirection
        else if (parsedInput->inputFile || parsedInput->outputFile) {
            int saved_stdout = dup(1);
            int saved_stdin = dup(0);

            exitValue = redirectIO(parsedInput->inputFile, parsedInput->outputFile);
            if (exitValue == 0) {
                //exitValue = execCommand(parsedInput, head, tail);
                exitValue = execCommand(parsedInput, pidArray);
            }

            dup2(saved_stdout, 1);
            close(saved_stdout);

            dup2(saved_stdin, 0);
            close(saved_stdin);
        }

        // Execute all other commands
        else {
            // Ignore the background operator ("&") if the shell is in foreground only mode
            if ( (sigtstpState == 1) && (parsedInput->background == 1)) {
                parsedInput->background = 0;
            }

           //pid = execCommand(parsedInput, head, tail);
           exitValue = execCommand(parsedInput, pidArray);
           

           //head = storePid(exitValue, head, tail);
           //printProcesses(head);
        }
    }
    

    return EXIT_SUCCESS;
}