#include <stdio.h>
#include <stdlib.h> // Needed by exit()
#include <limits.h> // Needed by PATH_MAX
#include <unistd.h> // Needed by getcwd(), chdir()
#include <signal.h> // Needed by signal()
#include <errno.h> // Needed by errno
#include <string.h> // Needed by strtok()
#include <glob.h>

#define MAX_INPUT_COMMAND_LINE 257 //one for new-line, one for NULL-terminated character

int main(int argc, char* argv[]) {
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGSTOP, SIG_DFL);
    signal(SIGKILL, SIG_DFL);
    setenv("PATH", "/bin:/usr/bin:.", 1);

    while(1) {
        char cwd[PATH_MAX + 1]; // NULL-terminated symbol at last
        if (getcwd(cwd, PATH_MAX + 1) != NULL) {

            char input_command_line[MAX_INPUT_COMMAND_LINE];
            printf("[3150 shell:%s]$", cwd);
            if (fgets(input_command_line, MAX_INPUT_COMMAND_LINE, stdin) != NULL) {
                input_command_line[strlen(input_command_line) - 1] = '\0'; // Remove the new-line character
                char *command_array[129];
                int i = 0;
                char *token;
                // Create Command Array
                for (token = strtok(input_command_line, " "); token != NULL; token = strtok(NULL, " ")) {
                    command_array[i++] = token;
                }
                command_array[i] = NULL; // Indicating end of command list

                if (i > 0) {
                    // Built-in command handling
                    if (strcmp(command_array[0], "cd") == 0) {
                        if (i != 2) {
                            printf("cd: wrong number of arguments\n");
                            continue;
                        } else {
                            if (chdir(command_array[1]) != 0) {
                                // perror(NULL);
                                printf("%s: cannot change directory\n", command_array[1]);
                                continue;
                            } else {
                                continue;
                            }
                        }
                    }

                    if (strcmp(command_array[0], "exit") == 0) {
                        if (i != 1) {
                            printf("exit: wrong number of arguments\n");
                            continue;
                        } else {
                            exit(0);
                        }
                    }

                    // Command
                    if (! fork()) {
                        signal(SIGINT, SIG_DFL);
                        signal(SIGTERM, SIG_DFL);
                        signal(SIGQUIT, SIG_DFL);
                        signal(SIGTSTP, SIG_DFL);
                        //child
                        // Expand the argument list
                        int i = 0;
                        glob_t glob_result;
                        while (command_array[i] != NULL) {
                            // DOOFFS - reserve some place at front
                            // NOCHECK - put the no-match pattern as it is
                            // MARK - add a slash at directory name
                            glob(command_array[i], GLOB_APPEND | GLOB_NOCHECK, NULL, &glob_result);
                            i++;
                        }

                        //Debugging routine
                        // i = 0;
                        // while (glob_result.gl_pathv[i] != NULL) {
                        //     printf("%s\n", glob_result.gl_pathv[i]);
                        //     i++;
                        // }
                        // exit(0);
                        execvp(command_array[0], glob_result.gl_pathv);
                        if (errno == 2) {
                            printf("%s: command not found\n", command_array[0]);
                            exit(errno);
                        } else {
                            printf("%s: unknow error\n", command_array[0]);
                            exit(errno);
                        }
                    } else {
                        wait(NULL);
                    }
                }
            }
            else {
                exit(errno);
            }
        }
        else {
            exit(errno);
        }
    }
}
