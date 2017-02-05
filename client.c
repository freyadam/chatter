#include <stdio.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <termios.h>

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

int main(int argc, char *argv[])
{
  
  /*
  char * server_address = "127.0.0.1";
  int server_port = 4444;
  */
  
  int server_port, username_len, password_len;
  size_t buffer_size;
  char *server_address, *username, *password;
  struct termios old_flags, new_flags;

  if( argc != 3)
    errx(1, "Usage: %s <server_address> <port_number>\n", 
         argv[0]);
  
  server_address = argv[1];

  server_port = atoi(argv[2]);  
  if( server_port <= 1024 )
    errx(1, "Port number has to be bigger than 1024.");

  buffer_size = 0;
  printf("Enter your username: ");
  if( ( username_len = getline(&username, &buffer_size, stdin)) == -1)
    err(1, "getline");
  username[username_len-1] = 0;
  
  tcgetattr(0, &old_flags);
  new_flags = old_flags;
  new_flags.c_lflag &= ~ECHO;
  new_flags.c_lflag |= ECHONL;
  
  tcsetattr(0, TCSANOW, &new_flags);
  
  buffer_size = 0;
  printf("Enter your password: ");
  if( ( password_len = getline(&password, &buffer_size, stdin) == -1))
    err(1, "getline");

  tcsetattr(0, TCSANOW, &old_flags);  
  

  printf("Address: %s\n", server_address);
  printf("Port: %d\n", server_port);
  printf("Username: %s\n", username);  
  printf("Password: %s\n", password);  
  

  /*
  int fd = get_connected_socket(server_address, server_port);

  char * line = malloc( 30 * sizeof(char) );

  ssize_t k = read(fd, line, 10);

  printf("Size: %d\n", k);

  line[k] = 0;

  printf("Read: %s", line);
  */

  return EXIT_SUCCESS;
}
