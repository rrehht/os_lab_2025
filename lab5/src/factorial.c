#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

long long k;
long long mod;
int pnum;

long long result = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *calculate(void *arg) {
    int id = *(int *)arg;

    long long part = 1;

    for (long long i = id + 1; i <= k; i += pnum) {
        part = (part * i) % mod;
    }

    pthread_mutex_lock(&mutex);
    result = (result * part) % mod;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s -k NUMBER --pnum=NUMBER --mod=NUMBER\n", argv[0]);
        return 1;
    }

    k = -1;
    mod = 0;
    pnum = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            k = atoll(argv[++i]);
        } else if (strncmp(argv[i], "--pnum=", 7) == 0) {
            pnum = atoi(argv[i] + 7);
        } else if (strncmp(argv[i], "--mod=", 6) == 0) {
            mod = atoll(argv[i] + 6);
        } else {
            printf("Unknown argument: %s\n", argv[i]);
            return 1;
        }
    }

    if (k < 0 || pnum <= 0 || mod <= 0) {
        printf("Invalid arguments\n");
        return 1;
    }

    pthread_t *threads = malloc(pnum * sizeof(pthread_t));
    int *ids = malloc(pnum * sizeof(int));

    if (threads == NULL || ids == NULL) {
        printf("Memory allocation error\n");
        free(threads);
        free(ids);
        return 1;
    }

    result = 1 % mod;

    for (int i = 0; i < pnum; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, calculate, &ids[i]);
    }

    for (int i = 0; i < pnum; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("%lld! mod %lld = %lld\n", k, mod, result);

    pthread_mutex_destroy(&mutex);
    free(threads);
    free(ids);

    return 0;
}