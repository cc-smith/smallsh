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

/* struct for user input */
struct userInput
{
    char* command;
    char* args[512];
    char* inputFile;
    char* outputFile;
    int background;
};

struct userInput* parseInputString(char* inputString) {

    // Initialize a struct in which to store the parsed user input
    struct userInput* currInput = malloc(sizeof(struct userInput));

    // Make a copy of the input string for strtok_r calls
    char inputCopy[2048];
    strcpy(inputCopy, inputString);

    // Check if command to be executed in the background
    char backgroundFlag[1];
    backgroundFlag[0] = inputCopy[strlen(inputCopy) - 3];
    if (strncmp(backgroundFlag, "&", 1) == 0) {
        currInput->background = 1; // "&" detected, set the background process flag
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
            char* token2 = strtok_r(token, " ", &saveptr2);
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
    token = strtok_r(inputCopy, " ", &saveptr);
    while (token != NULL) {

        // Token is an input file
        if (strncmp(token, "<", 1) == 0) {
            token = strtok_r(NULL, " ", &saveptr);
            currInput->inputFile = calloc(strlen(token) + 1, sizeof(char));
            strcpy(currInput->inputFile, token);
        }
        // Token is an output file
        else if (strncmp(token, ">", 1) == 0) {
            token = strtok_r(NULL, " ", &saveptr);
            currInput->outputFile = calloc(strlen(token) + 1, sizeof(char));
            strcpy(currInput->outputFile, token);
        }
        // Get next token 
        token = strtok_r(NULL, " ", &saveptr);
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

//stackoverflow.com/questions/298510/how-to-get-the-current-directory-in-a-c-program
char* changeDir(char* newDir) {
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
        return;
    }

    // Return current working dircectory
    getcwd(cwd, sizeof(cwd));
    return cwd;
}

int main(void) {
    struct userInput* parsedInput;
    char* inputString;
    inputString = malloc(2048 * sizeof(char));
    memset(inputString, '\0', 2048);

    for(;;) {

        // Get input from user
        printf(": ");
        fgets(inputString, 2048, stdin);

        // Ignore comments ("#") and blank lines 
        if (strncmp(inputString, "#", 1) == 0 || inputString[0] == '\n') {
            continue;
        }

        // Exit from shell
    /*    if (strcmp(inputString, "exit")) {
            return EXIT_SUCCESS;
        }*/

        // Transform any expansion variables ("$$") in the input
        char* expandedString = variableExpansion(inputString);

        // Parse the input into a struct
        parsedInput = parseInputString(expandedString);
        printStruct(parsedInput);

        // Check if command is a built in command (cd, exit, status)
        if (strcmp(parsedInput->command, "cd") == 0) {
            changeDir(parsedInput->args);
        }
        
        // Execute all other commands
        int execFlag = execCommand(parsedInput);
        execvp(parsedInput->command, parsedInput->args);

    }
    

    return EXIT_SUCCESS;
}