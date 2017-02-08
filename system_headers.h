#ifndef SYSTEM_HEADERS_H_
#define SYSTEM_HEADERS_H_

#include <stdio.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <termios.h>
#include <signal.h>
#include <poll.h>
#include <stdbool.h>
#include <assert.h>
#include <pthread.h>
#include <fcntl.h>

#define DELIMITER ' '

struct thread_data {
  pthread_t id;
  int comm_fd;
  int priority_fd;
  char * name;
  struct thread_data * next;
} * thread_list;

struct new_thread_args {
  int comm_fd;
  int priority_fd;
};

struct thread_data * thread_list;
pthread_mutex_t thr_list_mx;

#endif // SYSTEM_HEADERS_H_
