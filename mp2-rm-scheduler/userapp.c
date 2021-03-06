#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#define PROC_FILE "/proc/mp2/status"
#define true 1
#define false 0

typedef unsigned int    uint;

void write_to_file(char *s) {
    FILE *fp = fopen(PROC_FILE, "w");
    if (fp == NULL) {
        printf("[WRITE] fail to open file: %s\n", PROC_FILE);
        exit(1);
    }
    fprintf(fp, "%s", s);
    fclose(fp);
}

int check_exist(uint pid) {
    char* line = NULL;
    size_t len = 0;
    uint pid_p;
    int exist = false;
    FILE *fp = fopen(PROC_FILE, "r");
    if (fp == NULL) {
        printf("[READ] fail to open file: %s\n", PROC_FILE);
        exit(1);
    }

    while (getline(&line, &len, fp) != -1) {
        sscanf(line, "%d,", &pid_p);
        if (pid_p == pid) {
            exist = true;
            break;
        }
    }
    fclose(fp);

    return exist;
}

void sched_register(uint pid, uint period, uint computation) {
    char str[128];
    sprintf(str, "R,%d,%d,%d", pid, period, computation);
    write_to_file(str);
    printf("[Registered] pid: %d, period: %d, computation: %d\n", pid, period, computation);
}

void sched_yield(uint pid) {
    char str[32];
    sprintf(str, "Y,%d", pid);
    write_to_file(str);
}

void sched_degister(uint pid) {
    char s[64];
    sprintf(s, "D,%d", pid);
    write_to_file(s);
    printf("[Deregistered] pid: %d\n", pid);
}

unsigned long factorial(uint n) {
    unsigned long ans = 1;
    for (int i = 1; i <= n; i++) {
        ans *= i;
    }
    return ans;
}

unsigned long to_millisecond(struct timeval *t) {
    return t->tv_sec * 1000 + (t->tv_usec / 1000);
}

unsigned long get_time_interval(struct timeval *start, struct timeval *end) {
    unsigned long start_ms = to_millisecond(start);
    unsigned long end_ms = to_millisecond(end);
    return end_ms - start_ms;
}

int main(int argc, char* argv[]) {
    uint pid, n, period, computation;
    struct timeval start, end, compute_time, loop_start, loop_end;
    int iter = 10;
    unsigned long total_time, average_time, actual_period, actual_compute_time;

    pid = getpid();

    if (argc == 1) {
        n = 45000000;
        period = 1000;
        computation = 155;
    } else if (argc == 4) {
        n = atoi(argv[1]);
        period = atoi(argv[2]);
        computation = atoi(argv[3]);
    } else {
        printf("Invalid arguments\n");
        printf("Usage: ./userapp <factorial n> <period> <computation>\n");
        printf("\tExample: ./userapp 45000000 1000 155\n");
        exit(1);
    }

    sched_register(pid, period, computation);
    if (!check_exist(pid)) {
        printf("pid doesn't exist, exit...\n");
        exit(1);
    }

    sched_yield(pid);

    gettimeofday(&loop_start, NULL);
    for (int i = 0; i < iter; i++) {
        gettimeofday(&start, NULL);
        factorial(n);
        gettimeofday(&compute_time, NULL);
        sched_yield(pid);
        gettimeofday(&end, NULL);
        actual_period = get_time_interval(&start, &end);
        actual_compute_time = get_time_interval(&start, &compute_time);
        printf("Pid: %6d, actual period: %6lu, expect period: %6d, actual compute: %6lu, expect compute: %6d\n",
               pid, actual_period, period, actual_compute_time, computation);
    }
    gettimeofday(&loop_end, NULL);
    total_time = to_millisecond(&loop_end) - to_millisecond(&loop_start);
    average_time = total_time / iter;
    printf("average period: %lu, total time: %lu, iter: %d\n", average_time, total_time, iter);

    sched_degister(pid);

    return 0;
}
