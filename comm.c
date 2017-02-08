
#include "system_headers.h"
#include "comm.h"
#include "proto.h"

void init_pollfd_record(struct pollfd ** fds_ptr, int no_of_record, int fd){

  (*fds_ptr)[no_of_record].fd = fd;
  (*fds_ptr)[no_of_record].events = POLLIN;
  (*fds_ptr)[no_of_record].revents = 0;

}

void * run_comm_thread(void * arg_struct){

  struct new_thread_args * args = (struct new_thread_args *) arg_struct;
  
  int comm_fd = args->comm_fd;
  int priority_fd = args->priority_fd; 

  free(args);
  
  int fds_size = 2; // priority channel + thread communication channel
  struct pollfd * fds = malloc( sizeof(struct pollfd) * fds_size); 
      
  // initialize pollfd for priority channel
  init_pollfd_record( &fds, 0, priority_fd);

  // initialize pollfd for thread communication channel
  init_pollfd_record( &fds, 1, comm_fd);

  printf("Polling...\n");
  
  int err_poll, client_no;

  while( true ){

    printf("Poll\n");


    if( (err_poll = poll( fds,fds_size, -1)) < 0)
      errx(1,"poll");
    
    // priority channel
    if( fds[0].revents & POLLIN ){

      process_priority_request( fds, fds_size );

    }      

    // communication between threads
    if( fds[1].revents & POLLIN){

      process_comm_request(&fds, &fds_size);

    }    
    
    // client threads
    for( client_no = 2; client_no < fds_size; client_no++){      

      if( fds[client_no].revents & POLLIN ){
      
        printf("Entered block %d\n", client_no);
        
        process_client_request(&fds, &fds_size, client_no);

      }

    }
    
  }
  
  pthread_exit(NULL);

}

void process_priority_request(struct pollfd * fds, int fds_size){

  char * prefix, * message;
  int client_no;
  prefix = NULL; message = NULL;
  get_dispatch(fds[0].fd, &prefix, &message);

  if( message != NULL )
    printf("Priority message received: %s _ %s\n", prefix, message);      
  else 
    printf("Priority message received: %s\n", prefix);      

  if( strcmp(prefix,"END") == 0){
    for( client_no = 2; client_no < fds_size; client_no++){ // send end to all clients
      send_end(fds[client_no].fd);
    }
    free(prefix); free(message);
    pthread_exit(NULL);
  }
  free(prefix); free(message);

}

void process_comm_request(struct pollfd ** fds, int * fds_size){

  int new_fd;
  char * prefix, * message, * new_username;
  prefix = NULL; message = NULL;

  // add new client
  if( get_dispatch((*fds)[1].fd, &prefix, &message) != EXIT_SUCCESS)
    errx(1,"get_dispatch");

  if( strcmp(prefix, "MSG") != 0)
    errx(1,"new_client");

  new_username = message;
  message = NULL;
      
  if( get_dispatch((*fds)[1].fd, &prefix, &message) != EXIT_SUCCESS)
    errx(1,"get_dispatch");

  if( strcmp(prefix, "MSG") == 0)
    new_fd = atoi(message);
  else
    errx(1,"new_client");

  printf("New client: %s -- %d\n", new_username, new_fd);

  // add the new client
  add_client(fds, fds_size, new_fd);

  free(new_username);

  // release allocated resources
  if( prefix != NULL)
    free(prefix); 
  if( message != NULL)
    free(message);

}

void process_client_request(struct pollfd ** fds, int * fds_size, int client_no){

  int err_no, i;
  char * prefix, * message, * resend_msg;
  prefix = NULL; message = NULL;
      
  err_no = get_dispatch((*fds)[client_no].fd, &prefix, &message);
  if( err_no == -1) // something went wrong
    errx(1, "get_dispatch");
  else if( err_no == EOF_IN_STREAM) // EOF

    // end (*fds)[client_no];
    (*fds)[client_no].events = 0;

  else{ // everything worked well --> valid message

    printf("Dispatch received\n");

    if( strcmp(prefix, "ERR" ) == 0){
          
      send_message((*fds)[client_no].fd, "Error message received.");

    } else if( strcmp(prefix, "EXT" ) == 0){

      // back to menu
      send_message((*fds)[client_no].fd, "EXT: This function is currently disabled.");

    } else if( strcmp(prefix, "END" ) == 0){

      // end connection
      printf("Closing connection for client %d\n", client_no);
      delete_client(fds, fds_size, client_no);     

    } else if( strcmp(prefix, "CMD" ) == 0){

      // perform cmd
      send_message((*fds)[client_no].fd, "CMD: This function is currently disabled.");          
          
    } else if( strcmp(prefix, "MSG" ) == 0){
          
      //send_message((*fds)[client_no].fd, "MSG received");          
      
      // send message to all the other clients
      resend_msg = malloc( sizeof(char) * ( 1 + 4 + 2 + strlen(message) + 1 ));
      if( resend_msg == NULL)
        err(1,"malloc");
      err_no = sprintf( resend_msg, "<%d> %s", client_no, message);
      for (i = 2; i < *fds_size; i++) {
        if( i != client_no )
          send_message((*fds)[i].fd, resend_msg);      
      }


    }
       
    // release allocated resources
    if( prefix != NULL)
      free(prefix); 
    if( message != NULL)
      free(message);

  }

}

int add_client(struct pollfd ** fds_ptr, int * fds_size, int fd){

  char * usr_name = malloc(30);
  int i;

  *fds_ptr = (struct pollfd *) realloc(*fds_ptr, sizeof(struct pollfd) * ((*fds_size)+1) );
  if(*fds_ptr == NULL)
    err(1,"realloc");

  init_pollfd_record( fds_ptr, *fds_size, fd);
  
  // send chatroom info to new user
  send_message((*fds_ptr)[*fds_size].fd, "----- Connected to room -----");
  if( *fds_size > 2 ){

    send_message((*fds_ptr)[*fds_size].fd, "Current users:");
    for(i = 2; i < *fds_size; i++){
      sprintf(usr_name, "%d", i);
      send_message((*fds_ptr)[*fds_size].fd, usr_name);
    }

  }

  // send message to others as well
  for(i = 2; i < *fds_size; i++){
    sprintf(usr_name, "New user connected: %d", (*fds_ptr)[*fds_size].fd);
    send_message((*fds_ptr)[i].fd, usr_name);
  }

  free(usr_name);

  (*fds_size)++;

  return EXIT_SUCCESS;
}

int delete_client(struct pollfd ** fds, int * fds_size, int client_no){

  close(  (*fds)[client_no].fd  );

  int i;
  for( i = client_no+1; i < *fds_size; i++){
    (*fds)[i-1] = (*fds)[i];
  }
  

  (*fds_size)--;

  *fds = realloc(*fds, sizeof(struct pollfd) * (*fds_size));
  if( *fds == NULL)
    err(1, "realloc");

  return EXIT_SUCCESS; 
}

void create_comm_thread(char * name){

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

  if( pthread_create(&(thr_ptr->id), NULL, &run_comm_thread, (void *) args) != 0)
    errx(1, "pthread_create");


}
