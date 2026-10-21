#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <getopt.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdint.h>

#include "pthread.h"

#include "utils.h"

struct FactorialArgs {
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
};


uint64_t Factorial(const struct FactorialArgs *args) {
  uint64_t ans = 1;

 //дописали цикл
 for (uint64_t i = args->begin; i <= args->end; i++) {
        ans = MultModulo(ans, i, args->mod);
    }

  return ans;
}

//дополнили
void *ThreadFactorial(void *args) {
    struct FactorialArgs *fargs = (struct FactorialArgs *)args;

    uint64_t *result = malloc(sizeof(uint64_t));
    if (result == NULL) {
        return NULL;
    }

    *result = Factorial(fargs);

    return result;
}

int main(int argc, char **argv) {
  int tnum = -1;
  int port = -1;

  while (true) {

    static struct option options[] = {{"port", required_argument, 0, 0},
                                      {"tnum", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        port = atoi(optarg);
        // TODO: your code here
        break;
      case 1:
        tnum = atoi(optarg);
        // TODO: your code here
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Unknown argument\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  //добавили защиту от нулевого количества потоков или неправильного порта
  if (port <= 0 || port > 65535 || tnum <= 0) {
    fprintf(stderr, "Using: %s --port 20001 --tnum 4\n", argv[0]);
    return 1;
  }

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    fprintf(stderr, "Can not create server socket!");
    return 1;
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons((uint16_t)port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  int opt_val = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

  int err = bind(server_fd, (struct sockaddr *)&server, sizeof(server));
  if (err < 0) {
    fprintf(stderr, "Can not bind to socket!");
    return 1;
  }

  err = listen(server_fd, 128);
  if (err < 0) {
    fprintf(stderr, "Could not listen on socket\n");
    return 1;
  }

  printf("Server listening at %d\n", port);

  while (true) {
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);

    if (client_fd < 0) {
      fprintf(stderr, "Could not establish new connection\n");
      continue;
    }

    while (true) {
      int buffer_size = sizeof(uint64_t) * 3;
      char from_client[buffer_size];
      int read = recv(client_fd, from_client, buffer_size, 0);

      if (!read)
        break;
      if (read < 0) {
        fprintf(stderr, "Client read failed\n");
        break;
      }
      if (read < buffer_size) {
        fprintf(stderr, "Client send wrong data format\n");
        break;
      }

      pthread_t threads[tnum];

      uint64_t begin = 0;
      uint64_t end = 0;
      uint64_t mod = 0;
      memcpy(&begin, from_client, sizeof(uint64_t));
      memcpy(&end, from_client + sizeof(uint64_t), sizeof(uint64_t));
      memcpy(&mod, from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));

      fprintf(stdout, "Receive: %llu %llu %llu\n",
        (unsigned long long)begin,
        (unsigned long long)end,
        (unsigned long long)mod);

      //добавили обработку ошибок
      if (begin == 0 || end < begin || mod == 0) {
        fprintf(stderr, "Invalid factorial arguments\n");
        break;
      }

      struct FactorialArgs args[tnum];
      //дописали
      uint64_t count = end - begin + 1;
      uint64_t part = count / tnum;
      uint64_t remainder = count % tnum;

      uint64_t current = begin;

      for (int i = 0; i < tnum; i++) {
         uint64_t size = part + ((uint64_t)i < remainder ? 1 : 0);

          args[i].begin = current;
          args[i].end = current + size - 1;
          args[i].mod = mod;

          current += size;

          if (pthread_create(&threads[i], NULL,
                            ThreadFactorial, &args[i]) != 0) {
              fprintf(stderr, "pthread_create failed\n");
              return 1;
          }
      }

      uint64_t total = 1;
      //дописали
      for (int i = 0; i < tnum; i++) {
          void *thread_result = NULL;

          if (pthread_join(threads[i], &thread_result) != 0 ||
              thread_result == NULL) {
              fprintf(stderr, "pthread_join failed\n");
              return 1;
          }

          uint64_t result = *(uint64_t *)thread_result;

          total = MultModulo(total, result, mod);

          free(thread_result);
      }

     printf("Total: %llu\n", (unsigned long long)total);

      char buffer[sizeof(total)];
      memcpy(buffer, &total, sizeof(total));
      err = send(client_fd, buffer, sizeof(total), 0);
      if (err < 0) {
        fprintf(stderr, "Can't send data to client\n");
        break;
      }
    }
    

    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
  }

  return 0;
}
