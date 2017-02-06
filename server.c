#include <assert.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>    
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "proto.h"

#define ORIGINAL_FDS_SIZE 10

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
  int client_fd;
};


// return file descriptor on which the server is already listening (on port server_port)
int get_listening_socket(int server_port){

  int fd, yes = 1;  
  char * server_port_string = malloc( 5 * sizeof(char) );
  struct addrinfo hints, * result, * addr_info;
    
  bzero(&hints, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  sprintf( server_port_string, "%d", server_port);

  if ( getaddrinfo(NULL, server_port_string, &hints, &result) != 0)
    err(1,"getaddrinfo");

  for( addr_info = result; addr_info != NULL; addr_info = addr_info->ai_next){

    if ( (fd = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol)) == -1)
      continue;
    
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes,
                   sizeof(int)) == -1) 
      err(1, "setsockopt");
    

    if( bind(fd, addr_info->ai_addr, addr_info->ai_addrlen) == -1 ){
      close(fd);
      continue;
    }
    
    break;

  }

  freeaddrinfo(result);

  if( addr_info == NULL )
    err(1, "no valid gettaddrinfo result");

  if( listen(fd, SOMAXCONN) == -1)
    err(1, "listen");

  return fd;

}

void block_signals(){

  // set signal mask
  sigset_t signal_set;
  sigfillset(&signal_set);
  sigdelset(&signal_set, SIGKILL);
  sigdelset(&signal_set, SIGSTOP);

  if( pthread_sigmask(SIG_SETMASK, &signal_set ,NULL) != 0)
    err(1, "pthread_sigmask");

}

void * run_comm_thread(void * arg_struct){

  struct new_thread_args * args = (struct new_thread_args *) arg_struct;
  
  int comm_fd = args->comm_fd;
  int priority_fd = args->priority_fd; 
  int client_fd = args->client_fd; 

  free(args);
  
  int fds_size = ORIGINAL_FDS_SIZE;
  int fds_current = 2; // priority channel + thread communication channel
  struct pollfd * fds = malloc( sizeof(struct pollfd) * fds_size);
  
  // initialize pollfd for priority channel
  fds[0].fd = priority_fd;
  fds[0].events = POLLIN;
  fds[0].revents = 0;

  // initialize pollfd for thread communication channel
  fds[1].fd = comm_fd;
  fds[1].events = POLLIN;
  fds[1].revents = 0;
  

  char * message = malloc( 10 * sizeof(char));
  int err_poll;
  while( true ){
    if( (err_poll = poll( fds, fds_size, -1)) < 0)
      errx(1,"poll");
    
    // priority channel
    if( fds[1].revents & POLLIN ){
      int k = read(fds[1].fd, message, 4);
      message[k] = 0;
      printf("Priority message received: %s\n", message);      
      if( strcmp(message,"END") == 0)
        pthread_exit(NULL);
    }      
    
    printf("Single cycle finished\n");
  }
  

  pthread_exit(NULL);
}

void create_comm_thread(int client_fd, char * name){

  int comm_pipe[2], priority_pipe[2];
  struct thread_data * thr_data = malloc( sizeof(struct thread_data) );
  bzero(thr_data, sizeof(struct thread_data));

  // ----- INITIALIZE STRUCT -----

  if( pipe(comm_pipe) == -1)
    err(1, "pipe");
  thr_data->comm_fd = comm_pipe[1];
  
  if( pipe(priority_pipe) == -1)
    err(1, "pipe");
  thr_data->priority_fd = priority_pipe[1];
    
  thr_data->name = name;

  thr_data->next = NULL;

  printf("Initialized\n");

  // ----- APPEND TO LIST -----


  // get pointer to the last element of thread_list
  struct thread_data * thr_ptr;
  if( thread_list == NULL ){
    thread_list = thr_data;
    thr_ptr = thread_list;
  } else {
    for( thr_ptr = thread_list; thr_ptr->next != NULL; thr_ptr = thr_ptr->next){}
    thr_ptr->next = thr_data;
  }

  printf("Appended\n");

  // ----- START NEW THREAD -----
  
  struct new_thread_args * args = malloc( sizeof(struct new_thread_args) );
  args->comm_fd = comm_pipe[0];
  args->priority_fd = priority_pipe[0]; 
  args->client_fd = client_fd;

  if( pthread_create(&(thr_ptr->id), NULL, &run_comm_thread, (void *) args) != 0)
    errx(1, "pthread_create");


}

/*
void run_accept_thread(server_port){

  int fd, new_client;

  //change signal mask to let SIGINT through

  // set signal mask
  sigset_t signal_set;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGINT);
  if( pthread_sigmask(SIG_UNBLOCK, &signal_set ,NULL) != 0)
    err(1, "pthread_sigmask");

  while (true){
    fd = get_listening_socket(server_port);
    new_client = accept(fd, NULL, 0);
    
    // create communication thread
    create_comm_thread(new_client, "Prototype");

    printf("Thread created\n");

  }

}

void * create_accept_thread(){}
*/

void run_server(int server_port){

  block_signals();  

  int fd = get_listening_socket(server_port);
  int new_client = accept(fd, NULL, 0);

  // create communication thread
  create_comm_thread(new_client, "Prototype");

  printf("Thread created\n");

  // catch incoming signals  
  sigset_t signal_set;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGINT);
  int received_signal;
  if( sigwait(&signal_set, &received_signal) != 0)
    err(1, "sigwait");

  assert( received_signal == SIGINT);

  // kill everyone else  
  struct thread_data * thr_ptr;  
  for( thr_ptr = thread_list; thr_ptr != NULL; thr_ptr = thr_ptr->next){
    printf("Cycle\n");
    write( thr_ptr->comm_fd, "END", 3);
  }
  
  // join with other threads
  for( thr_ptr = thread_list; thr_ptr != NULL; thr_ptr = thr_ptr->next)
    pthread_join(thr_ptr->id, NULL); // pthread_join returns error but correctly returns result (???)

  printf("Exiting...\n");

  exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
  

  /*
  int opt, server_port;
  char * command_list, * user_list, * port_arg;    
  command_list = NULL;
  user_list = NULL;

  // ----- GET OPTIONS -----
  while((opt = getopt(argc, argv, "c:u:")) != -1)
    switch(opt) {
    case 'c': 
      command_list = malloc( (strlen(optarg) + 1) * sizeof(char) );
      if( command_list == NULL )
        errx(1, "command_list malloc");
      strcpy(command_list, optarg); break;
    case 'u':
      user_list = malloc( (strlen(optarg) + 1) * sizeof(char) );
      if( user_list == NULL )
        errx(1, "user_list malloc");
      strcpy(user_list, optarg); break;
    case '?': 
      fprintf(stderr,
              "Usage: %s [-c command_list] [-u user_list] <port_number> \n",
              basename(argv[0]));
      exit(1);
    }
  port_arg = argv[optind];

  if( port_arg == NULL ){
     fprintf(stderr,
             "Usage: %s [-c command_list] [-u user_list] <port_number> \n",
             basename(argv[0]));
     exit(1);
   }
   
  server_port = atoi(port_arg);  
  if( server_port <= 1024 )
    errx(1, "Port number has to be bigger than 1024.");

  // ----- PRINT SETTINGS -----
  printf("PORT: %d\n", server_port);
  if( command_list != NULL )
    printf("CMD: %s\n", command_list);
  if( user_list != NULL )
    printf("USER: %s\n", user_list);

  */

  // ----- INIT -----
  thread_list = NULL;
  int server_port = 4444;

  run_server(server_port);

  return EXIT_SUCCESS;
}
