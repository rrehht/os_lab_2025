#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdint.h>
#include <pthread.h>

#include "utils.h"

struct Server {
  char ip[255];
  int port;
};

//добавили структуру(данные для каждого потока клиента)
struct ServerArgs {
    struct Server server;
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
    uint64_t result;
};


bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }

  if (errno != 0)
    return false;

  *val = i;
  return true;
}

//перенесли подключение, send() и recv() из main() в отдельную функцию, чтобы каждый сервер обслуживался своим потоком
void *ServerThread(void *arg) {
    struct ServerArgs *args = (struct ServerArgs *)arg;

    struct hostent *hostname = gethostbyname(args->server.ip);
    if (hostname == NULL) {
        fprintf(stderr, "gethostbyname failed\n");
        return NULL;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(args->server.port);
    memcpy(&server.sin_addr, hostname->h_addr_list[0],
           hostname->h_length);

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
        fprintf(stderr, "Socket creation failed\n");
        return NULL;
    }

    if (connect(sck, (struct sockaddr *)&server,
                sizeof(server)) < 0) {
        fprintf(stderr, "Connection failed\n");
        close(sck);
        return NULL;
    }

    char task[sizeof(uint64_t) * 3];

    memcpy(task, &args->begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t),
           &args->end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t),
           &args->mod, sizeof(uint64_t));

    if (send(sck, task, sizeof(task), 0) != sizeof(task)) {
        fprintf(stderr, "Send failed\n");
        close(sck);
        return NULL;
    }

    char response[sizeof(uint64_t)];

    if (recv(sck, response, sizeof(response), MSG_WAITALL)
        != sizeof(response)) {
        fprintf(stderr, "Receive failed\n");
        close(sck);
        return NULL;
    }

    memcpy(&args->result, response, sizeof(uint64_t));

    close(sck);
    return arg;
}

int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers[255] = {'\0'}; // TODO: explain why 255

  while (true) {

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        ConvertStringToUI64(optarg, &k);
        // TODO: your code here
        break;
      case 1:
        ConvertStringToUI64(optarg, &mod);
        // TODO: your code here
        break;
      case 2:
        // TODO: your code here
        snprintf(servers, sizeof(servers), "%s", optarg);
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  //дополнили
  if (k == (uint64_t)-1 || mod == 0 || mod == (uint64_t)-1 || !strlen(servers)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  // клиент берёт адреса не из самого кода, а из текстового файла
  FILE *file = fopen(servers, "r");

  if (file == NULL) {
      fprintf(stderr, "Cannot open servers file\n");
      return 1;
  }

  unsigned int servers_num = 0;
  struct Server *to = NULL;
  struct Server temp;

  while (fscanf(file, "%254[^:]:%d\n", temp.ip, &temp.port) == 2) {
      if (temp.port <= 0 || temp.port > 65535) {
          fprintf(stderr, "Invalid server port\n");
          fclose(file);
          free(to);
          return 1;
      }

      struct Server *new_to = realloc(
          to, sizeof(struct Server) * (servers_num + 1));

      if (new_to == NULL) {
          fprintf(stderr, "Memory allocation failed\n");
          fclose(file);
          free(to);
          return 1;
      }

      to = new_to;
      to[servers_num++] = temp;
  }

  fclose(file);

  if (servers_num == 0) {
      fprintf(stderr, "No servers found\n");
      free(to);
      return 1;
  }

  //клиент запустит все соединения параллельно, дождётся результатов и перемножит их по модулю
  pthread_t threads[servers_num];
  struct ServerArgs args[servers_num];

  uint64_t part = k / servers_num;
  uint64_t remainder = k % servers_num;
  uint64_t current = 1;

  for (unsigned int i = 0; i < servers_num; i++) {
      uint64_t size = part + (i < remainder ? 1 : 0);

      args[i].server = to[i];
      args[i].begin = current;
      args[i].end = current + size - 1;
      args[i].mod = mod;
      args[i].result = 1;

      current += size;

      if (pthread_create(&threads[i], NULL,
                        ServerThread, &args[i]) != 0) {
          fprintf(stderr, "pthread_create failed\n");
          free(to);
          return 1;
      }
  }

  uint64_t answer = 1;

  for (unsigned int i = 0; i < servers_num; i++) {
      void *thread_result = NULL;

      pthread_join(threads[i], &thread_result);

      if (thread_result == NULL) {
          fprintf(stderr, "Server failed\n");
          free(to);
          return 1;
      }

      answer = MultModulo(answer, args[i].result, mod);
  }

  printf("answer: %llu\n", (unsigned long long)answer);

  free(to);

  return 0;
}
