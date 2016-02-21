#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <glob.h>
#include <sys/times.h>
#include <math.h>

pid_t pid;

void alarm_handler(int signal);

int main(int argc, char* argv[]) {
    // Check argument
    if (argc != 3) {
        printf("%s: wrong number of arguments\n", argv[0]);
        return EINVAL; // Invalid arguments
    }

    if (strcmp(argv[1], "FIFO") == 0) {
        FILE *fp;
        if ((fp = fopen(argv[2], "r")) != NULL) {
            char line[512];
            setenv("PATH", "/bin:/usr/bin:.", 1);
            while (fgets(line, 512, fp) != NULL) {
                line[strlen(line) - 1] = '\0'; // remove the new-line character
                char *token;
                int i = 0;
                // format the command
                char *command_array[130]; // last will be NULL
                for (token = strtok(line, " "); token != NULL; token = strtok(NULL, " ")) {
                    command_array[i++] = token;
                }
                int time_limit = atoi(command_array[i - 1]);
                command_array[i - 1] = NULL;
                if ((pid = fork()) == 0) {
                    glob_t glob_result;
                    int i = 0;
                    // Expand wildchar
                    while (command_array[i] != NULL) {
                        // DOOFFS - reserve some place at front
                        // NOCHECK - put the no-match pattern as it is
                        // MARK - add a slash at directory name
                        glob(command_array[i], GLOB_APPEND | GLOB_NOCHECK, NULL, &glob_result);
                        i++;
                    }

                    execvp(command_array[0], glob_result.gl_pathv);
                    if (errno == 2) {
                        printf("%s: command not found\n", command_array[0]);
                        exit(errno);
                    } else {
                        printf("%s: unknow error\n", command_array[0]);
                        exit(errno);
                    }
                    exit(0);
                } else {
                    pid_t child_pid;
                    double previous_cutime, previous_cstime, current_cutime, current_cstime;
                    if (time_limit >= 0) {
                        signal(SIGALRM, alarm_handler);
                        alarm(time_limit);
                    }
                    child_pid = wait(NULL);
                    alarm(0);

                    struct tms timebuf;
                    double ticks_per_sec = (double) sysconf(_SC_CLK_TCK);
                    times(&timebuf);
                    current_cutime = timebuf.tms_cutime / ticks_per_sec;
                    current_cstime = timebuf.tms_cstime / ticks_per_sec;
                    printf("<<Process %d>>\n", child_pid);
                    printf("time elapsed: %f\n", fabs((current_cutime + current_cstime) - (previous_cutime + previous_cstime)));  // fabs since when 0, it will have a minus sign
                    printf("user time   : %f\n", fabs(current_cutime - previous_cutime));
                    printf("system time : %f\n", fabs(current_cstime - previous_cstime));

                    previous_cutime += (current_cutime - previous_cutime);
                    previous_cstime += (current_cstime - previous_cstime);
                }
            }

            fclose(fp);
            return 0;
        } else {
            perror(NULL);
            exit(errno);
        }
    }

    if (strcmp(argv[1], "PARA") == 0) {
        FILE *fp;
        if ((fp = fopen(argv[2], "r")) != NULL) {
            char *line[10];
            setenv("PATH", "/bin:/usr/bin:.", 1);
            int i;
            for (i = 0; i < 10; i++) {
                line[i] = (char *) malloc(512 * sizeof(char));
            }
            i = 0;
            // Read all the lines
            while (fgets(line[i], 512, fp) != NULL) {
                i++;
            };
            line[i] = NULL;

            i = 0;
            pid_t main_pid = getpid();
            char *p = malloc(sizeof(char) * 512);
            while (line[i] != NULL && (p = strcpy(p, line[i])) != NULL) {
                if (! fork()) { // monitor process, child
                    p[strlen(p) - 1] = '\0'; // remove the new-line character
                    char *token;
                    int i = 0;
                    // format the command
                    char *command_array[130]; // last will be NULL
                    for (token = strtok(p, " "); token != NULL; token = strtok(NULL, " ")) {
                        command_array[i++] = token;
                    }
                    int time_limit = atoi(command_array[i - 1]);
                    command_array[i - 1] = NULL;
                    if ((pid = fork()) == 0) {
                        glob_t glob_result;
                        int i = 0;
                        // Expand wildchar
                        while (command_array[i] != NULL) {
                            // DOOFFS - reserve some place at front
                            // NOCHECK - put the no-match pattern as it is
                            // MARK - add a slash at directory name
                            glob(command_array[i], GLOB_APPEND | GLOB_NOCHECK, NULL, &glob_result);
                            i++;
                        }

                        execvp(command_array[0], glob_result.gl_pathv);
                        if (errno == 2) {
                            printf("%s: command not found\n", command_array[0]);
                            exit(errno);
                        } else {
                            printf("%s: unknow error\n", command_array[0]);
                            exit(errno);
                        }
                        exit(0);
                    } else {
                        pid_t child_pid;
                        double previous_cutime, previous_cstime, current_cutime, current_cstime;
                        if (time_limit >= 0) {
                            signal(SIGALRM, alarm_handler);
                            alarm(time_limit);
                        }
                        child_pid = wait(NULL);
                        alarm(0);

                        struct tms timebuf;
                        double ticks_per_sec = (double) sysconf(_SC_CLK_TCK);
                        times(&timebuf);
                        current_cutime = timebuf.tms_cutime / ticks_per_sec;
                        current_cstime = timebuf.tms_cstime / ticks_per_sec;
                        printf("<<Process %d>>\n", child_pid);
                        printf("time elapsed: %f\n", fabs((current_cutime + current_cstime) - (previous_cutime + previous_cstime)));  // fabs since when 0, it will have a minus sign
                        printf("user time   : %f\n", fabs(current_cutime - previous_cutime));
                        printf("system time : %f\n", fabs(current_cstime - previous_cstime));

                        previous_cutime += (current_cutime - previous_cutime);
                        previous_cstime += (current_cstime - previous_cstime);
                    }
                    break;
                } else {
                    i++;
                    continue;
                }
            }

            if (getpid() == main_pid) {
                while (wait(NULL) != -1);
            }

            fclose(fp);
            return 0;
        } else {
            perror(NULL);
            exit(errno);
        }
    }
}

void alarm_handler(int signal) {
    kill(pid, SIGTERM);
}
