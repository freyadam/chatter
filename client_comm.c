
#include "system_headers.h"
#include "proto.h"
#include "client_comm.h"

int get_connected_socket(char * server_address, int server_port){

  int fd;
  char * server_port_string = malloc( 5 * sizeof(char));
  struct addrinfo hints, * result, * addr_info;

  bzero(&hints, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  sprintf( server_port_string, "%d", server_port);

  if ( getaddrinfo(server_address, server_port_string, &hints, &result) != 0)
    err(1,"getaddrinfo");
  
  for( addr_info = result; addr_info != NULL; addr_info = addr_info->ai_next){

    if ( (fd = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol)) == -1)
      continue;

    if (connect(fd, addr_info->ai_addr, addr_info->ai_addrlen) == -1) {
      close(fd);
      continue;
    }

    break;

  }
  
  freeaddrinfo(result);

  if( addr_info == NULL )
    err(1, "no valid gettaddrinfo result");
  
  return fd;

}

int run_client(char * server_address, int server_port){

  ssize_t line_len;
  size_t allocated_len;
  char * line;
  int fd = get_connected_socket(server_address, server_port);  

  printf("We're connected!\n");
  // get info from server

  while( true ){

    allocated_len = 0; line = NULL;

    if( (line_len = getline(&line, &allocated_len, stdin)) == -1){
      printf("Failed to read line. Exiting...\n");
      return EXIT_FAILURE;
    }

    // get rid of \n on the end of the line
    printf("L: %d ___ %s\n", (int)line_len, line);
    line[line_len-1] = '\0';

    if( process_client_request(fd, &line) == EXIT_FAILURE )
      return EXIT_FAILURE;

  }  

  return EXIT_SUCCESS;
}

int process_client_request(int fd, char ** line){
  
  return send_message(fd, *line);

}

