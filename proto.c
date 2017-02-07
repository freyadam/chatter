
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

  free(*line_ptr);

  int position, line_len, err_read;
  line_len = 10;
  position = 0;
  char * line = malloc( sizeof(char) * line_len );
  char c;
  while( (err_read = read(fd, &c, 1)) == 1 ){

    if( position >= line_len - 1){
      line_len += 10;
      line = realloc(line,  sizeof(char) * line_len );
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
    err(1,"read");

  *line_ptr = line;

  if( err_read == 0 ){
    if( position >= line_len - 1){
      line_len += 1;
      line = realloc(line, sizeof(char) * line_len );
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

  printf("prefix_len: %d, prefix: '%s'\n", (int)prefix_len, prefix);

  if( prefix_len == 0)
    return EOF_IN_STREAM;
  if( prefix_len != 3)
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

    printf("Msg\n");

    // get length of message
    int err_arg = get_delim(fd, message_ptr, DELIMITER);
    if ( err_arg == -1){
      return -1;
    }
    else if ( err_arg == 0)
      return EOF_IN_STREAM;
    
    int msg_length = atoi(*message_ptr);

    printf("Len: %d\n", msg_length);

    free(*message_ptr);
    *message_ptr = malloc( sizeof(char) * msg_length );
    if( message_ptr == NULL )
      err(1,"malloc");

    // get the actual message
    err_arg = read(fd, *message_ptr, msg_length+1);
    if ( err_arg != msg_length+1) {
      printf("A __ %d\n", err_arg);
      return -1;
    }
    else if ( (*message_ptr)[err_arg-1] != DELIMITER){
      printf("B __ %d __ %c\n", err_arg, (*message_ptr)[err_arg-1]);
      return -1;
    }

    (*message_ptr)[msg_length] = '\0';
    printf("Content: %s\n", *message_ptr);

    *prefix_ptr = prefix;
    // message_ptr already set previously

  } else { // UNKNOWN PREFIX

    return -1;

  }

  return EXIT_SUCCESS;
}
