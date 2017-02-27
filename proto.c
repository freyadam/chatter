
#include "system_headers.h"
#include "proto.h"

// test snippet:
/*
  int fd = open("test_file", O_RDONLY);
  char * line;
  int err;
  while( ( err = get_delim(fd, &line, ' ')) != -1){
  printf("%s\n", line);
  if( err == EOF_IN_STREAM )
  break;
  }
*/

// get next delimited part of text without the delimititer (which will be consumed)
int get_delim(int fd, char ** line_ptr, char del){

  if( (*line_ptr) != NULL) 
    free(*line_ptr);

  int position, line_len, err_read;
  line_len = 10;
  position = 0;
  char * line = malloc( line_len );
  char c;

  while( (err_read = read(fd, &c, 1)) == 1 ){

    if( position >= line_len - 1){
      line_len += 10;
      line = realloc(line, line_len );
      if( line == NULL )
        return -1;
    }

    line[position++] = c;
    if ( c == del ){
      line[position-1] = '\0';
      break;
    }        

  }
  
  if( err_read == -1 )
    return -1;

  *line_ptr = line;

  if( err_read == 0 ){
    if( position >= line_len - 1){
      line_len += 1;
      line = realloc(line, line_len );
      if( line == NULL )
        return -1;
    }

    line[++position] = '\0';
    return 0;
  }

  return position-1;  // don't include the null character 
}

int get_dispatch(int fd, char ** prefix_ptr, char ** message_ptr){
  
  assert( EOF_IN_STREAM != -1);
  assert( EOF_IN_STREAM != EXIT_SUCCESS);

  char * prefix = NULL;
  ssize_t prefix_len;

  // get prefix
  prefix_len = get_delim(fd, &prefix, DELIMITER);

  //printf("prefix_len: %d, prefix: '%s'\n", (int)prefix_len, prefix);

  if( prefix_len == 0)
    return EOF_IN_STREAM;
  else if( prefix_len == -1)
    return -1;
  else if( prefix_len != 3)
    return -1;

  if( strcmp(prefix, "ERR") == 0){ // ERROR
    
    *prefix_ptr = prefix;
    *message_ptr = NULL;

  } else if( strcmp(prefix, "EXT") == 0){ // EXIT

    *prefix_ptr = prefix;
    *message_ptr = NULL;

  } else if( strcmp(prefix, "END") == 0){ // END OF TRANSMISSION

    *prefix_ptr = prefix;
    *message_ptr = NULL;
    
  } else if( strcmp(prefix, "CMD") == 0){ // COMMAND
    
    // get the actual command 
    int err_arg = get_delim(fd, message_ptr, DELIMITER);
    if ( err_arg == -1 )
      return -1;
    else if ( err_arg == 0 )
      return EOF_IN_STREAM;
        
    *prefix_ptr = prefix;
    // message_ptr already set in get_delim

  } else if( strcmp(prefix, "MSG") == 0){ // MESSAGE

    // get length of message
    int err_arg = get_delim(fd, message_ptr, DELIMITER);    

    if ( err_arg == -1){
      return -1;
    }
    else if ( err_arg == 0)
      return EOF_IN_STREAM;
    

    int msg_length = strtol(*message_ptr, NULL, 10);
    if( msg_length == 0 && errno == EINVAL )
      err(1, "strtol");

    free(*message_ptr);

    *message_ptr = malloc( msg_length+1 );
    if( message_ptr == NULL )
      err(1,"malloc");

    // get the actual message
    err_arg = read(fd, *message_ptr, msg_length+1);
    if ( err_arg != msg_length+1) {
      return -1;
    }
    else if ( (*message_ptr)[msg_length] != DELIMITER){
      return -1;
    }

    (*message_ptr)[msg_length] = '\0';

    *prefix_ptr = prefix;
    // message_ptr already set previously


  } else { // UNKNOWN PREFIX

    return -1;

  }

  return EXIT_SUCCESS;
}

int get_message(int fd, char ** contents_ptr){

  if( *contents_ptr != NULL ){
    free( *contents_ptr );
  }

  *contents_ptr = NULL;
  char * prefix = NULL;
  
  int err = get_dispatch(fd, &prefix, contents_ptr);

  if( err == -1 || err == EOF_IN_STREAM )
    return err;

  if( strcmp(prefix, "MSG") != 0 )
    return -1;

  return EXIT_SUCCESS;

}

int send_dispatch(int fd, char * dispatch){
  
  int result = write(fd, dispatch, strlen(dispatch));
  
  if( result != strlen(dispatch))
    return EXIT_FAILURE;
  
  return EXIT_SUCCESS;
}

int send_message(int fd, char * message){
  
  char * dispatch = malloc(3 + 1 + MAX_MSG_LEN_SIZE + 1 + strlen(message) + 1 + 1);

  int result = sprintf(dispatch, "MSG %d %s ", (int)strlen(message), message);
  if( result < 0)
    return EXIT_FAILURE;

  result = send_dispatch(fd, dispatch);  

  free(dispatch);

  return result;
}

int send_command(int fd, char * cmd){

  char * dispatch = malloc( (3 + 1 + strlen(cmd) + 1 + 1));
  
  int result = sprintf(dispatch, "CMD %s ", cmd);
  if( result < 0)
    return EXIT_FAILURE;

  result = send_dispatch(fd, dispatch);

  free(dispatch);

  return result;
}

int send_error(int fd){

  return send_dispatch(fd, "ERR ");

}

int send_exit(int fd){

  return send_dispatch(fd, "EXT ");

}

int send_end(int fd){

  return send_dispatch(fd, "END ");

}

int send_end_to_all(struct pollfd * fds, int fds_size){
  
  int i, result;
  for( i = 0; i < fds_size; i++ ){

    result = send_end(fds[i].fd);
    if( result == EXIT_FAILURE )
      printf("Client %d could not receive 'end of transmission' message.\n", i);
  }

  return EXIT_SUCCESS;
}

int send_message_to_all(struct pollfd * fds, int fds_size, int exception, char * message){

  int i, result;
  for( i = 0; i < fds_size; i++ ){

    if( i != exception ){
      result = send_message(fds[i].fd, message);
      if( result == EXIT_FAILURE )
        printf("Client %d could not receive message.\n", i);
    }
  }

  return EXIT_SUCCESS;
}


int send_message_from_file(int fd, char * file_path){

  int fildes = open(file_path, O_RDONLY, 0666);
    
  // get file length
  int length_of_file = (int)lseek(fildes, 0, SEEK_END);
  if( length_of_file == -1 )
    err(1,"lseek");
  if( lseek(fildes, 0, SEEK_SET) == -1 ) // set file offset to start again
    err(1,"lseek");

  // write header
  char * prefix = malloc( strlen("MSG") + strlen(" ") + 10 + strlen(" "));
  sprintf(prefix, "MSG %d ", length_of_file);
  send_dispatch(fd, prefix);
  free(prefix);

  // write contents of file
  int buffer_size, rd;
  buffer_size = 2000;
  char * buffer = malloc( buffer_size + 1);

  while( (rd = read(fildes, buffer, buffer_size)) > 0 ){
    
    buffer[rd] = '\0';
    if( send_dispatch(fd, buffer) != EXIT_SUCCESS)
      errx(1, "send_dispatch -- send_message_from_file");

  }

  if( rd == -1 )
    err(1,"read");

  free(buffer);

  // finish message
  if( send_dispatch(fd, " ") != EXIT_SUCCESS)
    errx(1, "send_dispatch -- send_message_from_file");

  return EXIT_SUCCESS;

}
