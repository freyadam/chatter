
#include "common.h"
#include "commands.h"
#include "proto.h"

int load_commands_aux(char * filename){

  
  FILE * file = fopen(filename, "r");
  if( file == NULL )
    err(1,"fopen");

  char * save_ptr, * cmd_name, * cmd, * line = NULL;
  size_t len = 0;  

  struct command_str * new;
  while( commands != NULL ){ // delete current command list

    new = commands->next;

    free(commands);

    commands = new;
  }

  struct command_str * end_of_cmd_list = commands;

  errno = 0;
  while( getline(&line, &len, file) != -1){
    
    if( line[0] == '#' ) // comment
      continue;

    cmd_name = NULL;
    cmd = NULL;
    save_ptr = NULL;

    cmd_name = strtok_r(line, " ", &save_ptr);
    if( cmd_name == NULL || cmd_name[0] == '\0' ){
      printf("Malformed line in commands file, skipping...\n");
      continue;
    }

    cmd = line + strlen(cmd_name) + 1;
    if( cmd == NULL || cmd[0] == '\0' ){
      printf("Malformed line in commands file, skipping...\n");
      continue;
    }

    if( cmd[ strlen(cmd) - 1] == '\n' ){
      cmd[ strlen(cmd) - 1] = '\0';
    }

    // copy name and actual command into their own storage places
    char * name_storage, * cmd_storage;
    name_storage = malloc( strlen(cmd_name) + 1);
    cmd_storage = malloc( strlen(cmd) + 1);
    if( name_storage == NULL || cmd_storage == NULL )
      errx(1,"malloc");

    strcpy(name_storage, cmd_name);
    strcpy(cmd_storage, cmd);

    struct command_str * new 
      =  (struct command_str *) malloc( sizeof(struct command_str) );
    new->name = name_storage;
    new->command = cmd_storage;
    new->next = NULL;

    if(commands == NULL){
      commands = new;
      end_of_cmd_list = new;
    }
    else {
      end_of_cmd_list->next = new;
      end_of_cmd_list = new;
    }
    
  }
  
  if( errno != 0 )
    err(1,"getline");

  free(line);
  
  return 0;
}

int load_commands(char * filename){

  pthread_mutex_lock(&commands_mx);

  int result = load_commands_aux(filename);

  pthread_mutex_unlock(&commands_mx);

  return result;

}

void list_commands_aux(){

  struct command_str * cmd = commands;
  while( cmd != NULL ){

    printf("%s -> '%s'\n", cmd->name, cmd->command);

    cmd = cmd->next;
  }

}

void list_commands(){

  pthread_mutex_lock(&commands_mx);

  list_commands_aux();

  pthread_mutex_unlock(&commands_mx);

}

int perform_command_aux(int fd, char * cmd, char * room_name){

  char * temp = malloc( strlen("/tmp/") + 6 + strlen(room_name) + 1);
  sprintf(temp, "/tmp/%d_%s", getpid(), room_name);

  // combination of pid and room name is unique  
  char * arg = malloc( strlen(cmd) + 3 + strlen(temp) + 1);
  sprintf(arg, "%s > %s", cmd, temp);  

  if( system(arg) == -1)
    send_message(fd, "Error: command failed");
  else{    
    send_message_from_file(fd, temp);
  }

  // remove temp file
  unlink(temp);

  free(arg);
  free(temp);

  return 0;
}

int perform_command(int fd, char * cmd, char * room_name){

  pthread_mutex_lock(&commands_mx);

  int result = perform_command(fd, cmd, room_name);

  pthread_mutex_unlock(&commands_mx);

  return result;

}

char * get_command_aux(char * cmd_name){

  struct command_str * cmd = commands;
  while( cmd != NULL ){

    if( strcmp(cmd_name, cmd->name) == 0 )
      return cmd->command;

    cmd = cmd->next;
  }

  return NULL;
}

char * get_command(char * cmd_name){

  pthread_mutex_lock(&commands_mx);

  char * result = get_command(cmd_name);

  pthread_mutex_unlock(&commands_mx);

  return result;

}
