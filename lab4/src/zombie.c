#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        printf("Child finished\n");
        exit(0);
    } else {
        printf("Parent is waiting...\n");
        sleep(20);
        wait(NULL);
        printf("Zombie removed\n");
    }

    return 0;
}