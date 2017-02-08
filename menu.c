
#include "system_headers.h"
#include "menu.h"
#include "proto.h"
#include "comm.h"

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
