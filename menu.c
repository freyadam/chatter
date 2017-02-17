
#include "system_headers.h"
#include "menu.h"
#include "proto.h"
#include "thread_common.h"

static void process_comm_request(struct pollfd ** fds, int * fds_size, char * room_name);
static void process_client_request(struct pollfd ** fds, int * fds_size, int client_no);

void * run_menu_thread( void * arg_struct ){

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

      process_priority_request( fds, fds_size, room_name);

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

void create_menu_thread(){
 
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
    
  thr_data->name = "Menu";

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

  if( pthread_create(&(thr_ptr->id), NULL, &run_menu_thread, (void *) args) != 0)
    errx(1, "pthread_create");

}


int add_client_to_menu(struct pollfd ** fds_ptr, int * fds_size, int fd){

  // SUPPOSE THAT NAME IS SHORTER THAN 100 CHARACTERS
  char * name = malloc(100);

  *fds_ptr = (struct pollfd *) realloc(*fds_ptr, sizeof(struct pollfd) * ((*fds_size)+1) );
  if(*fds_ptr == NULL)
    err(1,"realloc");

  init_pollfd_record( fds_ptr, *fds_size, fd);
  
  // send chatroom info to new user
  sprintf( name, "----- Connected to Menu -----");
  send_message(fd, name);

  // list rooms 
  send_message(fd, "Rooms:");

  // skip menu and iterate over all rooms 
  int counter = 1;  
  struct thread_data * thr_ptr;
  for( thr_ptr = thread_list->next; thr_ptr != NULL; thr_ptr = thr_ptr->next){
    sprintf(name, "%d - %s", counter++, thr_ptr->name);
    send_message(fd, name);
  }

  // list options
  send_message(fd, "Options:");

  sprintf(name, "u - Create new user");
  send_message(fd, name);

  sprintf(name, "c - Create new chatroom");
  send_message(fd, name);
  
  // how to pick your setting
  send_message(fd, "To select your option simply type its associated number or letter.");

  free(name);

  (*fds_size)++;

  return EXIT_SUCCESS;

}

static void process_comm_request(struct pollfd ** fds, int * fds_size, char * room_name){

  int new_fd;
  char * prefix, * message, * new_username;
  prefix = NULL; message = NULL;

  // add new client
  if( get_dispatch((*fds)[1].fd, &prefix, &message) != EXIT_SUCCESS)
    errx(1,"get_dispatch");

  if( strcmp(prefix, "MSG") != 0){
    printf("Malformed dispatch from room %s\n", room_name);
    return;
  }

  new_username = message;
  message = NULL;
      
  if( get_dispatch((*fds)[1].fd, &prefix, &message) != EXIT_SUCCESS)
    errx(1,"get_dispatch");

  if( strcmp(prefix, "MSG") == 0)
    new_fd = atoi(message);
  else {
    printf("Malformed dispatch from room %s\n", room_name);
    return;
  }

  printf("New client: %s -- %d\n", new_username, new_fd);

  // add the new client
  add_client_to_menu(fds, fds_size, new_fd);

  free(new_username);  

  // release allocated resources
  if( prefix != NULL)
    free(prefix); 
  if( message != NULL)
    free(message);

}


static void process_client_request(struct pollfd ** fds, int * fds_size, int client_no){

  int err_no, client_fd;
  char * prefix, * message;
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
      send_message(client_fd, "EXT: Nowhere to exit to from menu.");

    } else if( strcmp(prefix, "END" ) == 0){

      // end connection
      printf("Closing connection for client %d\n", client_no);
      delete_client(fds, fds_size, client_no);     

    } else if( strcmp(prefix, "CMD" ) == 0){

      // perform cmd
      send_message(client_fd, "CMD: This function is disabled in menu.");          
          
    } else if( strcmp(prefix, "MSG" ) == 0){
                
      if( strcmp(message, "u" ) == 0){

        send_message(client_fd, "Creating new user is not implemented yet.");          

      } else if( strcmp(message, "c" ) == 0){

        send_message(client_fd, "Creating new chatroom is not implemented yet.");          

      } else { // enter chat room
        
        errno = 0;
        int chat_no = strtol(message, NULL, 10);

        if( errno != 0 || chat_no <= 0 ){
          send_message(client_fd, "Invalid message - no option selected."); 
          return;
        }

        // select which chat room to enter
        struct thread_data * thr_ptr;
        for( thr_ptr = thread_list->next; thr_ptr != NULL && chat_no-- > 1; thr_ptr = thr_ptr->next){}
        
        if( chat_no >= 1 ){
          send_message(client_fd, "Such a room does not exist, the number is too high."); 
          return;
        }

        // transfer client to the chat room
        printf("%s\n", thr_ptr->name);
        if( transfer_client(thr_ptr->comm_fd, fds, fds_size, client_no) != EXIT_SUCCESS )
          errx(1, "transfer_client");

      }

    }
       
    // release allocated resources
    if( prefix != NULL)
      free(prefix); 
    if( message != NULL)
      free(message);

  }

}

