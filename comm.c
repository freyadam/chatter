
#include "system_headers.h"
#include "comm.h"
#include "proto.h"
#include "thread_common.h"
#include "commands.h"

static void process_comm_request(struct pollfd ** fds, char *** names, int * fds_size, char * room_name);
static void process_client_request(struct pollfd ** fds, char *** names, int * fds_size, int client_no);

void poll_cycle( struct pollfd ** fds_ptr, char *** names_ptr, int * fds_size_ptr, char * room_name ){

  int err_poll, client_no;

  if( (err_poll = poll( (*fds_ptr),(*fds_size_ptr), -1)) < 0)
    errx(1,"poll");
    
  // priority channel
  if( (*fds_ptr)[0].revents & POLLIN ){

    process_priority_request( (*fds_ptr), (*fds_size_ptr), room_name );

  }      

  // communication between threads
  if( (*fds_ptr)[1].revents & POLLIN){

    process_comm_request( fds_ptr, names_ptr, fds_size_ptr, room_name);

  }    
    
  // client threads
  for( client_no = 2; client_no < (*fds_size_ptr); client_no++){      

    if( (*fds_ptr)[client_no].revents & POLLIN ){
      
        
      process_client_request( fds_ptr, names_ptr, fds_size_ptr, client_no);

    }

  }

}

void * run_comm_thread(void * arg_struct){

  struct new_thread_args * args = (struct new_thread_args *) arg_struct;
  
  int comm_fd = args->comm_fd;
  int priority_fd = args->priority_fd; 
  char * room_name = args->data_ptr->name;

  free(args);
  
  int fds_size = 2; // priority channel + thread communication channel
  struct pollfd * fds = malloc( sizeof(struct pollfd) * fds_size);  
  char ** names = malloc( sizeof(char *) * fds_size);

  // initialize pollfd for priority channel
  init_pollfd_record( &fds, 0, priority_fd);
  names[0] = "priority";

  // initialize pollfd for thread communication channel
  init_pollfd_record( &fds, 1, comm_fd);
  names[1] = "comm";
  
  while( true ){

    poll_cycle(&fds, &names, &fds_size, room_name);
    
  }
  
  pthread_exit(NULL);

}

static void process_comm_request(struct pollfd ** fds, char *** names, int * fds_size, char * room_name){

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

  if( strcmp(prefix, "MSG") == 0){
    new_fd = strtol(message, NULL, 10);
    if( new_fd == 0 && errno == EINVAL )
      err(1, "strtol");
  } else
    errx(1,"new_client");

  printf("New client: %s -- its fd: %d\n", new_username, new_fd);  

  // add the new client
  add_client(fds, names, fds_size, new_fd, new_username, room_name);

  //free(new_username);

  // release allocated resources
  if( prefix != NULL)
    free(prefix); 
  if( message != NULL)
    free(message);

}


static void process_client_request(struct pollfd ** fds, char *** names, int * fds_size, int client_no){

  int err_no, i, client_fd;
  char * prefix, * message, * resend_msg;
  prefix = NULL; message = NULL;
  client_fd = (*fds)[client_no].fd;
      
  err_no = get_dispatch(client_fd, &prefix, &message);
  if( err_no == -1) // something went wrong
    errx(1, "get_dispatch");
  else if( err_no == EOF_IN_STREAM) // EOF

    // end (*fds)[client_no];
    (*fds)[client_no].events = 0;

  else{ // everything worked well --> valid message

    printf("Dispatch received\n");

    if( strcmp(prefix, "ERR" ) == 0){
          
      send_message(client_fd, "Error message received.");

    } else if( strcmp(prefix, "EXT" ) == 0){

      // back to menu
      transfer_client(thread_list->comm_fd, fds, names, fds_size, client_no);

    } else if( strcmp(prefix, "END" ) == 0){

      // end connection
      printf("Closing connection for client %d\n", client_no);
      delete_client(fds, names, fds_size, client_no);     

    } else if( strcmp(prefix, "CMD" ) == 0){

      printf("%s\n", message);

      // perform cmd
      char * cmd = get_command(message);
      if( cmd == NULL ){
        send_message(client_fd, "Command not found.");
      } else {
        perform_command(client_fd, cmd, (*names)[client_no]);
      }

    } else if( strcmp(prefix, "MSG" ) == 0){
                
      // send message to all the other clients
      resend_msg = malloc( 1 + 4 + 2 + strlen(message) + 1 );
      if( resend_msg == NULL)
        err(1,"malloc");
      err_no = sprintf( resend_msg, "<%s> %s", (*names)[client_no], message);
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

void send_info_to_new_user(struct pollfd ** fds_ptr, char *** names, int * fds_size, char * room_name){

  char * name = malloc(100);
  int i, fd;
  fd = (*fds_ptr)[*fds_size].fd;

  sprintf( name, "----- Connected to room %s -----", room_name);
  send_message(fd, name);
  if( *fds_size > 2 ){

    send_message(fd, "Current users:");
    for(i = 2; i < *fds_size; i++){
      sprintf(name, "%s", (*names)[i]);
      send_message(fd, name);
    }

  }

  send_message(fd, "Commands:");
  send_message(fd, "/cmd task ... Perform task on server.");
  send_message(fd, "/ext ... Exit to menu.");
  send_message(fd, "/end ... End connection.");

  free(name);

}

int add_client(struct pollfd ** fds_ptr, char *** names, int * fds_size, int fd, char * user_name, char * room_name){

  char * name = malloc(50 + strlen(user_name));
  int i;

  *fds_ptr = (struct pollfd *) realloc(*fds_ptr, sizeof(struct pollfd) * ((*fds_size)+1) );
  if(*fds_ptr == NULL)
    err(1,"realloc");

  *names = (char **) realloc(*names, sizeof( char *) * ((*fds_size)+1) );
  if(*names == NULL)
    err(1,"realloc");

  init_pollfd_record( fds_ptr, *fds_size, fd);
  (*names)[*fds_size] = user_name;
  
  // send chatroom info to new user
  send_info_to_new_user(fds_ptr, names, fds_size, room_name);

  // send message to others as well
  for(i = 2; i < *fds_size; i++){
    sprintf(name, "New user connected: %s", user_name);
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

  // ----- START NEW THREAD -----
  
  struct new_thread_args * args = malloc( sizeof(struct new_thread_args) );
  args->comm_fd = comm_pipe[0];
  args->priority_fd = priority_pipe[0]; 
  args->data_ptr = thr_data;

  if( pthread_create(&(thr_ptr->id), NULL, &run_comm_thread, (void *) args) != 0)
    errx(1, "pthread_create");

}
