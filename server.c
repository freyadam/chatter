#include <stdio.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

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

int main(int argc, char *argv[])
{
  

  int server_port = 4444;

  /*
  int opt, server_port;
  char * command_list, * user_list, * port_arg;    
  command_list = NULL;
  user_list = NULL;
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

  printf("PORT: %d\n", server_port);
  if( command_list != NULL )
    printf("CMD: %s\n", command_list);
  if( user_list != NULL )
    printf("USER: %s\n", user_list);

  */

  int fd = get_listening_socket(server_port);
  int new_fd = accept(fd, NULL, 0);

  write(new_fd,"Hey!\n", 5);

  close(fd), close(new_fd);

  return EXIT_SUCCESS;
}
