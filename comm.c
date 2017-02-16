
#include "system_headers.h"
#include "comm.h"
#include "proto.h"
#include "thread_common.h"

void * run_comm_thread(void * arg_struct){

  struct new_thread_args * args = (struct new_thread_args *) arg_struct;
  
  int comm_fd = args->comm_fd;
  int priority_fd = args->priority_fd; 
  char * room_name = args->data_ptr->name;

  free(args);
  
  int fds_size = 2; // priority channel + thread communication channel
  struct pollfd * fds = malloc( sizeof(struct pollfd) * fds_size); 
      
  // initialize pollfd for priority channel
  init_pollfd_record( &fds, 0, priority_fd);

  // initialize pollfd for thread communication channel
  init_pollfd_record( &fds, 1, comm_fd);
  
  int err_poll, client_no;

  while( true ){

    if( (err_poll = poll( fds,fds_size, -1)) < 0)
      errx(1,"poll");
    
    // priority channel
    if( fds[0].revents & POLLIN ){

      process_priority_request( fds, fds_size, room_name );

    }      

    // communication between threads
    if( fds[1].revents & POLLIN){

      process_comm_request(&fds, &fds_size, room_name);

    }    
    
    // client threads
    for( client_no = 2; client_no < fds_size; client_no++){      

      if( fds[client_no].revents & POLLIN ){
      
        
        process_client_request(&fds, &fds_size, client_no);

      }

    }
    
  }
  
  pthread_exit(NULL);

}

static void process_comm_request(struct pollfd ** fds, int * fds_size, char * room_name){

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
  add_client(fds, fds_size, new_fd, room_name);

  free(new_username);

  // release allocated resources
  if( prefix != NULL)
    free(prefix); 
  if( message != NULL)
    free(message);

}

static void process_client_request(struct pollfd ** fds, int * fds_size, int client_no){

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

int add_client(struct pollfd ** fds_ptr, int * fds_size, int fd, char * room_name){

  char * name = malloc(100);
  int i;

  *fds_ptr = (struct pollfd *) realloc(*fds_ptr, sizeof(struct pollfd) * ((*fds_size)+1) );
  if(*fds_ptr == NULL)
    err(1,"realloc");

  init_pollfd_record( fds_ptr, *fds_size, fd);
  
  // send chatroom info to new user
  sprintf( name, "----- Connected to room %s -----", room_name);
  send_message((*fds_ptr)[*fds_size].fd, name);
  if( *fds_size > 2 ){

    send_message((*fds_ptr)[*fds_size].fd, "Current users:");
    for(i = 2; i < *fds_size; i++){
      sprintf(name, "%d", i);
      send_message((*fds_ptr)[*fds_size].fd, name);
    }

  }


  send_message((*fds_ptr)[*fds_size].fd, "Commands:");
  send_message((*fds_ptr)[*fds_size].fd, "/cmd task ... Perform task on server.");
  send_message((*fds_ptr)[*fds_size].fd, "/ext ... Exit to menu.");
  send_message((*fds_ptr)[*fds_size].fd, "/end ... End connection.");


  // send message to others as well
  for(i = 2; i < *fds_size; i++){
    sprintf(name, "New user connected: %d", (*fds_ptr)[*fds_size].fd);
    send_message((*fds_ptr)[i].fd, name);
  }

  free(name);

  (*fds_size)++;

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
  args->data_ptr = thr_data;

  if( pthread_create(&(thr_ptr->id), NULL, &run_comm_thread, (void *) args) != 0)
    errx(1, "pthread_create");


}
