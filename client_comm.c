
#include "client_comm.h"
#include "system_headers.h"

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
