#include "system_headers.h"
#include "proto.h"
#include "client_comm.h"

int main(int argc, char *argv[])
{
  
  
  char * server_address = "127.0.0.1";
  int server_port = 4444;
  
  /*
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
  */

  
  int fd = get_connected_socket(server_address, server_port);

  /*
  char * msg = malloc(4);
  read(fd, msg, 3);
  msg[3] = 0;

  printf("Msg: %s", msg);  
  */

  return EXIT_SUCCESS;
}
