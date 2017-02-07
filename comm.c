
#include "system_headers.h"
#include "comm.h"
#include "proto.h"

#define ORIGINAL_FDS_SIZE 10

void * run_comm_thread(void * arg_struct){

  struct new_thread_args * args = (struct new_thread_args *) arg_struct;
  
  int comm_fd = args->comm_fd;
  int priority_fd = args->priority_fd; 
  int client_fd = args->client_fd; 

  free(args);
  
  int err_dispatch;
  int fds_size = 3; // priority channel + thread communication channel
  struct pollfd * fds = malloc( sizeof(struct pollfd) * fds_size); 
      
  // initialize pollfd for priority channel
  fds[0].fd = priority_fd;
  fds[0].events = POLLIN;
  fds[0].revents = 0;

  // initialize pollfd for thread communication channel
  fds[1].fd = comm_fd;
  fds[1].events = POLLIN;
  fds[1].revents = 0;

  // initialize pollfd for thread communication channel
  fds[2].fd = client_fd;
  fds[2].events = POLLIN;
  fds[2].revents = 0;

  printf("Polling...\n");
  
  char * message, * prefix;
  message = NULL;
  prefix = NULL;
  char * priority_message;
  int err_poll, j;

  while( true ){

    if( (err_poll = poll( fds,fds_size, -1)) < 0)
      errx(1,"poll");
    
    // priority channel
    if( fds[0].revents & POLLIN ){
      priority_message = malloc( 10 * sizeof(char));
      int k = read(fds[0].fd, priority_message, 4);
      priority_message[k] = '\0';;
      printf("Priority message received: %s\n", priority_message);      
      if( strcmp(priority_message,"END") == 0){
        for( j = 2; j < fds_size; j++){ // send end to all clients
          send_end(fds[j].fd);
        }
        pthread_exit(NULL);
      }
      free(priority_message);
    }      

    /*
    if( fds[1].revents & POLLIN){

      // add new client

    }
    */

    
    // client test
    if( fds[2].revents & POLLIN ){
      
      prefix = NULL; message = NULL;
      
      err_dispatch = get_dispatch(fds[2].fd, &prefix, &message);
      if( err_dispatch == -1) // something went wrong
        errx(1, "get_dispatch");
      else if( err_dispatch == EOF_IN_STREAM) // EOF

        // end fds[2];
        fds[2].events = 0;

      else{ // everything worked well --> valid message


        printf("Ok\n");

        if( strcmp(prefix, "ERR" ) == 0){
          
          send_message(fds[2].fd, "ERR");

        } else if( strcmp(prefix, "EXT" ) == 0){

          // back to menu
          send_message(fds[2].fd, "EXT");

        } else if( strcmp(prefix, "END" ) == 0){

          // end connection
          send_message(fds[2].fd, "END");          

        } else if( strcmp(prefix, "CMD" ) == 0){

          // perform cmd
          send_message(fds[2].fd, "CMD");          
          
        } else if( strcmp(prefix, "MSG" ) == 0){
          
          // send message to others
          send_message(fds[2].fd, "MSG");          
        }
       
        // release allocated resources
        if( prefix != NULL)
          free(prefix); 
        if( message != NULL)
          free(message);

      }

    }
    
  }
  
  pthread_exit(NULL);

}

int add_client(struct pollfd ** fds, int fds_size, int fd){

  *fds = (struct pollfd *) realloc(*fds, sizeof(struct pollfd) * (fds_size++) );
  if(*fds == NULL)
    err(1,"realloc");

  (*fds)[fds_size].fd = fd;
  (*fds)[fds_size].events = POLLIN;
  (*fds)[fds_size].revents = 0;

  return EXIT_SUCCESS;
}

int delete_client(struct pollfd * fds, int fds_size, int client_no){

  int i;
  for( i = client_no+1; i < fds_size; i++){
    fds[i-1] = fds[i];
  }
  
  fds_size--;

  return EXIT_SUCCESS;
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
