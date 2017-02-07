

#include "system_headers.h"
#include "accept.h"
#include "comm.h"

void accept_signal_handler(int sig){

  pthread_exit(NULL);

}

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


void * run_accept_thread(void * arg){

  int fd, new_client, server_port;
  server_port = (int)arg;

  //change signal mask to let SIGUSR1 through
  // set signal mask
  sigset_t signal_set;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGUSR1);
  if( pthread_sigmask(SIG_UNBLOCK, &signal_set ,NULL) != 0)
    err(1, "pthread_sigmask");

  //set sigaction
  struct sigaction act;
  act.sa_handler = &accept_signal_handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = NULL;
  if( sigaction(SIGUSR1, &act, NULL) == -1)
    err(1, "sigaction");

  fd = get_listening_socket(server_port);

  while(true){

    new_client = accept(fd, NULL, 0);
    
    // create communication thread
    create_comm_thread(new_client, "Prototype");

    printf("Thread created\n");

  }

  return NULL;
}

pthread_t  create_accept_thread(int server_port){

  pthread_t accept_thread;

  assert( sizeof(void *) >= sizeof(int) );

  pthread_create( &accept_thread, NULL, &run_accept_thread, (void *)server_port);

  return accept_thread;
}


