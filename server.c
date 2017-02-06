#include "system_headers.h"
#include "proto.h"
#include "accept.h"
#include "menu.h"
#include "comm.h"
#include "signal.h"

struct thread_data * thread_list;

void block_signals(){

  // set signal mask
  sigset_t signal_set;
  sigfillset(&signal_set);
  sigdelset(&signal_set, SIGKILL);
  sigdelset(&signal_set, SIGSTOP);

  if( pthread_sigmask(SIG_SETMASK, &signal_set ,NULL) != 0)
    err(1, "pthread_sigmask");

}

void run_server(int server_port){

  block_signals();  

  int fd = get_listening_socket(server_port);
  int new_client = accept(fd, NULL, 0);

  // create communication thread
  create_comm_thread(new_client, "Prototype");

  printf("Thread created\n");

  run_signal_thread();
}

void print_server_settings(int server_port, char * command_list, char * user_list){
  
  printf("PORT: %d\n", server_port);
  if( command_list != NULL )
    printf("CMD: %s\n", command_list);
  if( user_list != NULL )
    printf("USER: %s\n", user_list);

}

int main(int argc, char *argv[])
{
  

  /*
  int opt, server_port;
  char * command_list, * user_list, * port_arg;    
  command_list = "./commands"; // default
  user_list = "./users"; // default

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
  print_server_settings(server_port, command_list, user_list);

  */

  // ----- INIT -----
  thread_list = NULL;
  int server_port = 4444;

  run_server(server_port);

  return EXIT_SUCCESS;
}
