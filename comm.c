
#include "system_headers.h"
#include "comm.h"
#include "proto.h"

#define ORIGINAL_FDS_SIZE 10

void * run_comm_thread(void * arg_struct){

  struct new_thread_args * args = (struct new_thread_args *) arg_struct;
  
  int comm_fd = args->comm_fd;
  int priority_fd = args->priority_fd; 
  //int client_fd = args->client_fd; 

  free(args);
  
  int err_dispatch;
  int fds_size = 2; // priority channel + thread communication channel
  struct pollfd * fds = malloc( sizeof(struct pollfd) * fds_size); 
      
  // initialize pollfd for priority channel
  fds[0].fd = priority_fd;
  fds[0].events = POLLIN;
  fds[0].revents = 0;

  // initialize pollfd for thread communication channel
  fds[1].fd = comm_fd;
  fds[1].events = POLLIN;
  fds[1].revents = 0;

  printf("Polling...\n");
  
  char * message, * prefix, * new_username;
  message = NULL;
  prefix = NULL;
  int err_poll, j, new_fd, client_no;

  while( true ){

    printf("Entering poll...\n");


    if( (err_poll = poll( fds,fds_size, -1)) < 0)
      errx(1,"poll");
    
    // priority channel
    if( fds[0].revents & POLLIN ){
      prefix = NULL; message = NULL;
      get_dispatch(fds[0].fd, &prefix, &message);
      printf("Priority message received: %s\n", prefix);      
      if( strcmp(prefix,"END") == 0){
        for( j = 2; j < fds_size; j++){ // send end to all clients
          send_end(fds[j].fd);
        }
        free(prefix); free(message);
        pthread_exit(NULL);
      }
      free(prefix); free(message);
    }      

    
    if( fds[1].revents & POLLIN){

      prefix = NULL; message = NULL;

      // add new client
      if( get_dispatch(fds[1].fd, &prefix, &message) != EXIT_SUCCESS)
        errx(1,"get_dispatch");

      if( strcmp(prefix, "MSG") != 0)
        errx(1,"new_client");

      new_username = message;
      message = NULL;
      
      if( get_dispatch(fds[1].fd, &prefix, &message) != EXIT_SUCCESS)
        errx(1,"get_dispatch");

      if( strcmp(prefix, "MSG") == 0)
        new_fd = atoi(message);
      else
        errx(1,"new_client");

      printf("New client: %s -- %d\n", new_username, new_fd);

      // add the new client
      add_client(&fds, &fds_size, new_fd);

      printf("New fds_size: %d, fd[2]: %d\n", fds_size, fds[2].fd);

      free(new_username);

      // release allocated resources
      if( prefix != NULL)
        free(prefix); 
      if( message != NULL)
        free(message);

    }    
    
    // client test
    for( client_no = 2; client_no < fds_size; client_no++){      

      if( fds[client_no].revents & POLLIN ){
      
        prefix = NULL; message = NULL;
      
        err_dispatch = get_dispatch(fds[client_no].fd, &prefix, &message);
        if( err_dispatch == -1) // something went wrong
          errx(1, "get_dispatch");
        else if( err_dispatch == EOF_IN_STREAM) // EOF

          // end fds[client_no];
          fds[client_no].events = 0;

        else{ // everything worked well --> valid message


          printf("Ok\n");

          if( strcmp(prefix, "ERR" ) == 0){
          
            send_message(fds[client_no].fd, "ERR");

          } else if( strcmp(prefix, "EXT" ) == 0){

            // back to menu
            send_message(fds[client_no].fd, "EXT");

          } else if( strcmp(prefix, "END" ) == 0){

            // end connection
            printf("Ending connection for client %d\n", client_no);
            delete_client(&fds, &fds_size, client_no);     

          } else if( strcmp(prefix, "CMD" ) == 0){

            // perform cmd
            send_message(fds[client_no].fd, "CMD");          
          
          } else if( strcmp(prefix, "MSG" ) == 0){
          
            // send message to others
            send_message(fds[client_no].fd, "MSG");          
          }
       
          // release allocated resources
          if( prefix != NULL)
            free(prefix); 
          if( message != NULL)
            free(message);

        }

      }
    }
    
  }
  
  pthread_exit(NULL);

}

int add_client(struct pollfd ** fds_ptr, int * fds_size, int fd){

  *fds_ptr = (struct pollfd *) realloc(*fds_ptr, sizeof(struct pollfd) * ((*fds_size)) );
  if(*fds_ptr == NULL)
    err(1,"realloc");

  (*fds_ptr)[*fds_size].fd = fd;
  (*fds_ptr)[*fds_size].events = POLLIN;
  (*fds_ptr)[*fds_size].revents = 0;
  
  (*fds_size)++;

  return EXIT_SUCCESS;
}

int delete_client(struct pollfd ** fds, int * fds_size, int client_no){

  int i;
  for( i = client_no+1; i < *fds_size; i++){
    (*fds)[i-1] = (*fds)[i];
  }
  
  (*fds_size)--;

  return EXIT_SUCCESS;
}

void create_comm_thread(char * name){

  int client_fd = 0;
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
  if( pthread_mutex_lock(&thr_list_mx) != 0)
    errx(1, "pthread_mutex_unlock");

  struct thread_data * thr_ptr;
  if( thread_list == NULL ){
    thread_list = thr_data;
    thr_ptr = thread_list;
  } else {
    for( thr_ptr = thread_list; thr_ptr->next != NULL; thr_ptr = thr_ptr->next){}
    thr_ptr->next = thr_data;
  }

  if( pthread_mutex_unlock(&thr_list_mx) != 0)
    errx(1, "pthread_mutex_unlock");

  printf("Appended\n");

  // ----- START NEW THREAD -----
  
  struct new_thread_args * args = malloc( sizeof(struct new_thread_args) );
  args->comm_fd = comm_pipe[0];
  args->priority_fd = priority_pipe[0]; 
  args->client_fd = client_fd;

  if( pthread_create(&(thr_ptr->id), NULL, &run_comm_thread, (void *) args) != 0)
    errx(1, "pthread_create");


}
