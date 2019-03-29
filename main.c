#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#define PRINT_ERROR fprintf(stderr, "Error in system call\n");
#define MAX_LENGTH 512
#define PROMPT "> "


typedef struct Job {
    pid_t pid;
    char cmd[MAX_LENGTH];
} Job;

int main() {
    Job jobsArray[MAX_LENGTH];
    int jobsCounter = 0;
    int flag = 0;

    // previous_pwd - path to previous working directory
    char previousPwd[MAX_LENGTH] = "not set";

    while(1) {
        pid_t pid;
        char command[MAX_LENGTH];
        char *token;
        char *arguments[MAX_LENGTH];
        int spaces = 0, lastArgNum;


        // show the prompt
        printf("%s", PROMPT);
        fflush(stdout);
        // read command
        fgets(command, sizeof(command), stdin);
        // remove the \n at the end of the command
        command[strlen(command) - 1] = 0;

        // if the command is empty
        if (strcmp(command, "\0") == 0) {
            continue;
        }

        // create backup of command
        char commandCopy[MAX_LENGTH];
        strcpy(commandCopy, command);

        // if the path contains "
        if (strchr(command, '\"') != NULL) {
            token = strtok(command, "\"");
            // split command and append tokens to array arguments
            while (token) {
                arguments[spaces] = token;
                token = strtok(NULL, "\"");
                spaces++;
            }
            arguments[0] = "cd";
        } else if (strchr(command, '\'') != NULL) {
            token = strtok(command, "\'");
            // split command and append tokens to array arguments
            while (token) {
                arguments[spaces] = token;
                token = strtok(NULL, "\'");
                spaces++;
            }
            arguments[0] = "cd";
        } else {
            token = strtok(command, " ");
            // split command and append tokens to array arguments
            while (token) {
                arguments[spaces] = token;
                token = strtok(NULL, " ");
                spaces++;
            }
        }

        // add NULL to the arguments
        if (strcmp(arguments[spaces - 1], "&") == 0) {
            arguments[spaces - 1] = NULL;
            lastArgNum = spaces - 2;
            flag = 1;
        } else {
            arguments[spaces] = NULL;
            lastArgNum = spaces - 1;
        }


        // exit command
        if (strcmp(arguments[0], "exit") == 0) {
            pid_t currentPID = getpid();
            printf("%d\n", currentPID);

            int j = 0;
            // kill all the processes
            for (j = 0; j < jobsCounter; j++) {
                kill(jobsArray[j].pid, SIGTERM);
            }
            exit(0);
        }

        // cd command
        else if (strcmp(arguments[0], "cd") == 0) {
            pid_t currentPID = getpid();
            printf("%d\n", currentPID);

            // save current working dir
            char currentPwd[MAX_LENGTH];
            if (getcwd(currentPwd, MAX_LENGTH) == NULL) {
                PRINT_ERROR
            }
            // for global path:
            // "cd" or "cd ~ ..."
            if (strcmp(arguments[lastArgNum], "cd") == 0 ||
                strcmp(arguments[1], "~") == 0) {
                // set working directory to HOME for global
                if (chdir(getenv("HOME")) != -1) {
                    // if successfully changed wd to home, update prev p working dir
                    strcpy(previousPwd,currentPwd);
                } else {
                    PRINT_ERROR;
                }
            } else if (strcmp(arguments[1], "-") == 0) {
                // go to previous folder
                // chdir to father dir
                if (strcmp(previousPwd, "not set") != 0) {
                    // go to previous folder
                    if (chdir(previousPwd) == -1) {
                        PRINT_ERROR;
                    } else {
                        printf("%s\n", previousPwd);
                        strcpy(previousPwd,currentPwd);
                    }
                } else {
                    //OLDWD not set
                    fprintf(stderr, "cd: OLDWD not set\n");
                }
            } else {
                // change pwd path using chdir
                // set working directory to param path
                if (chdir(arguments[1]) == -1) {
                    PRINT_ERROR;
                } else {
                    strcpy(previousPwd,currentPwd);
                }
            }
        }

        // jobs command
        else if (strcmp(arguments[0], "jobs") == 0) {
            Job tempJobsArray[MAX_LENGTH];
            int i, tempJobsCounter = 0;

            // update the Jobs array to include only living processes
            for (i = 0; i < jobsCounter; i++) {
                // if pid is alive
                if (waitpid(jobsArray[i].pid, NULL, WNOHANG) == 0) {
                    // save the process in the tempJobsArray
                    tempJobsArray[tempJobsCounter] = jobsArray[i];
                    tempJobsCounter++;
                }
            }

            // copy the tempJobsArray that now includes only living processes to the jobsArray
            for (i = 0; i < tempJobsCounter; i++) {
                jobsArray[i] = tempJobsArray[i];
            }
            jobsCounter = tempJobsCounter;

            // print living processes
            for (i = 0; i < jobsCounter; i++) {
                printf("%ld ", (long)jobsArray[i].pid);
                // clear the "&" if exists
                const char symbol[2] = "&";
                char *clearCommand;
                clearCommand = strtok(jobsArray[i].cmd, symbol);
                printf("%s\n", clearCommand);
            }
        } else {
            pid = fork();
            switch (pid) {
                case -1:
                    PRINT_ERROR;
                    break;
                case 0:
                    execvp(arguments[0], arguments);
                    PRINT_ERROR;
                    break;
                default:
                    //print the pid of the child
                    printf("%d\n", pid);
                    //if the last token is not & wait for the child to finish
                    if (flag) {
                        Job newProcess;
                        newProcess.pid = pid;
                        strcpy(newProcess.cmd, commandCopy);
                        jobsArray[jobsCounter] = newProcess;
                        jobsCounter++;
                        flag = 0;
                    } else {
                        // wait for the child process to end
                        int waitRet = waitpid(pid, &pid, 0);
                        if (waitRet == -1) {
                            PRINT_ERROR;
                        }
                    }
                    break;
            }
        }

    }
}