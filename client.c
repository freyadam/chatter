#include <stdio.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>

int main(int argc, char *argv[])
{
  
  int server_port;
  char * server_address;

  if( argc != 3)
    errx(1, "Usage: %s <server_address> <port_number>\n", 
         argv[0]);
  
  server_address = argv[1];

  server_port = atoi(argv[2]);  
  if( server_port <= 1024 )
    errx(1, "Port number has to be bigger than 1024.");

  printf("Address: %s\n", server_address);
  printf("Port: %d\n", server_port);

  return EXIT_SUCCESS;
}
