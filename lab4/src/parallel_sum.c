#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>

#include "sum.h"
#include "utils.h"

void *ThreadSum(void *arg) {
    struct SumArgs *args = (struct SumArgs *)arg;
    long long *result = malloc(sizeof(long long));
    *result = Sum(args);
    return result;
}

int main(int argc, char **argv) {
    int threads_num = 0, seed = 0, array_size = 0;

    for (int i = 1; i < argc; i += 2) {
        if (i + 1 >= argc) return 1;

        if (strcmp(argv[i], "--threads_num") == 0)
            threads_num = atoi(argv[i + 1]);
        else if (strcmp(argv[i], "--seed") == 0)
            seed = atoi(argv[i + 1]);
        else if (strcmp(argv[i], "--array_size") == 0)
            array_size = atoi(argv[i + 1]);
    }

    if (threads_num <= 0 || seed <= 0 ||
        array_size <= 0 || threads_num > array_size) {
        printf("Invalid arguments\n");
        return 1;
    }

    int *array = malloc(sizeof(int) * array_size);
    GenerateArray(array, array_size, seed);

    pthread_t threads[threads_num];
    struct SumArgs args[threads_num];

    struct timeval start, finish;
    gettimeofday(&start, NULL);

    for (int i = 0; i < threads_num; i++) {
        args[i].array = array;
        args[i].begin = i * array_size / threads_num;
        args[i].end = (i + 1) * array_size / threads_num;

        pthread_create(&threads[i], NULL, ThreadSum, &args[i]);
    }

    long long total = 0;

    for (int i = 0; i < threads_num; i++) {
        long long *result;
        pthread_join(threads[i], (void **)&result);
        total += *result;
        free(result);
    }

    gettimeofday(&finish, NULL);

    double time = (finish.tv_sec - start.tv_sec) * 1000.0;
    time += (finish.tv_usec - start.tv_usec) / 1000.0;

    printf("Total: %lld\n", total);
    printf("Elapsed time: %f ms\n", time);

    free(array);
    return 0;
}