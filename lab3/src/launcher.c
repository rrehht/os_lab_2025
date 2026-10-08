#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        execl("./sequential_min_max", "sequential_min_max", "42", "10", (char *)NULL);

        perror("execl");
        exit(1);
    }

    wait(NULL);
    printf("Child process finished\n");

    return 0;
}