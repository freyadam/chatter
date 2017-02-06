
#include "system_headers.h"
#include "comm.h"

#define ORIGINAL_FDS_SIZE 10

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
