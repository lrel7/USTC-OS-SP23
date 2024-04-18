#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#define TASK_COMM_LEN 16
#define NUM_OUTPUT 20
#define NS 1000000000.000

int *pids, *last_pids = NULL;
char* commands;
long* states;
unsigned long long *runtimes, *last_runtimes = NULL;
float* cpus;
int* idxs;
int num_p = 0, malloc_num_p, last_num_p = 0;

int compare(const void* a, const void* b) {
    // return (*(int*)a - *(int*)b);
    return (cpus[*(int*)a] < cpus[*(int*)b]);
}

int main(int argc, char* argv[]) {
    int cycle = 1;  // seconds to refresh

    /* Parse args */
    if ((argc > 1 && strcmp(argv[1], "-d")) || argc == 2 || argc > 3) {
        printf("Usage: %s [-d <seconds>]\n", argv[0]);
        return 1;
    }
    if (argc == 3) {
        cycle = atoi(argv[2]);
    }

    while (1) {
        system("clear");

        syscall(332, &num_p);       // get number of processes
        malloc_num_p = num_p + 10;  // leave space for possible new processes

        /* Get info from kernel */
        pids = (int*)realloc(pids, malloc_num_p * sizeof(int));
        commands = (char*)realloc(commands, malloc_num_p * TASK_COMM_LEN * sizeof(char));
        states = (long*)realloc(states, malloc_num_p * sizeof(long));
        runtimes = (unsigned long long*)realloc(runtimes, malloc_num_p * sizeof(unsigned long long));
        syscall(333, pids, commands, states, runtimes, &num_p);

        /* Get CPU util */
        cpus = (float*)realloc(cpus, num_p * sizeof(float));
        for (int i = 0; i < num_p; i++) {
            cpus[i] = runtimes[i] / NS / cycle;

            if (last_pids) {
                int last_i = 0;
                while (last_i < last_num_p) {
                    if (last_pids[last_i] == pids[i]) {  // process exists before
                        cpus[i] = (runtimes[i] - last_runtimes[last_i]) / NS / cycle * 100;
                        break;
                    }
                    last_i++;
                }
            }
        }

        /* Sort */
        idxs = (int*)realloc(idxs, num_p * sizeof(int));
        for (int i = 0; i < num_p; i++) {
            idxs[i] = i;
        }
        qsort(idxs, num_p, sizeof(int), compare);

        /* Print */
        printf("%-7s%-18s%-11s%-8s%-8s\n", "PID", "COMM", "ISRUNNING", "%CPU", "TIME");
        for (int i = 0; i < NUM_OUTPUT && i < num_p; i++) {
            int idx = idxs[i];
            printf("%-7d%-18s%-11d%-8.2f%-8.2f\n", pids[idx], commands + idx * TASK_COMM_LEN, !states[idx], cpus[idx], runtimes[idx] / NS);
        }

        /* Update variables */
        last_pids = (int*)realloc(last_pids, num_p * sizeof(int));
        last_runtimes = (unsigned long long*)realloc(last_runtimes, num_p * sizeof(unsigned long long));
        memcpy(last_pids, pids, num_p * sizeof(int));
        memcpy(last_runtimes, runtimes, num_p * sizeof(unsigned long long));
        last_num_p = num_p;

        sleep(cycle);
    }

    /* Free arrays */
    free(pids);
    free(commands);
    free(states);
    free(runtimes);
    free(cpus);
    free(idxs);
    free(last_pids);
    free(last_runtimes);
    return 0;
}