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
    char* args;
    char* inputFile;
    char* outputFile;
    int background;
};

struct userInput* parseInputString(char* inputString) {
    struct userInput* currInput = malloc(sizeof(struct userInput));

    char inputCopy[2048];
    strcpy(inputCopy, inputString);

    // Check if background
    char backgroundFlag[1];
    backgroundFlag[0] = inputCopy[strlen(inputCopy) - 3];

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

    // The next token is the arguments
    token = strtok_r(NULL, "<>&\n", &saveptr);
    currInput->args = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currInput->args, token);

    // The next tokens are input files and/or output files
    token = strtok_r(inputCopy, " ", &saveptr);
    while (token != NULL) {

        if (strncmp(token, "<", 1) == 0) {
            token = strtok_r(NULL, " ", &saveptr);
            currInput->inputFile = calloc(strlen(token) + 1, sizeof(char));
            strcpy(currInput->inputFile, token);
        }

        else if (strncmp(token, ">", 1) == 0) {
            token = strtok_r(NULL, " ", &saveptr);
            currInput->outputFile = calloc(strlen(token) + 1, sizeof(char));
            strcpy(currInput->outputFile, token);
        }
        token = strtok_r(NULL, " ", &saveptr);
    }

    return currInput;
}

int main(void) {
    struct userInput* newInput;
    char* inputString;
    inputString = malloc(2048 * sizeof(char));

    memset(inputString, '\0', 2048);
    printf(": ");
    fgets(inputString, 2048, stdin);
    newInput = parseInputString(inputString);
    printf("\nThis is the struct:\n"
        "command: %s\n"
        "args: %s\n"
        "inputFile: %s\n"
        "outputFile: %s\n"
        "background: %i\n",
        newInput->command,
        newInput->args,
        newInput->inputFile,
        newInput->outputFile,
        newInput->background);

    return EXIT_SUCCESS;
}