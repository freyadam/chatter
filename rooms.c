
#include "common.h"
#include "comm.h"


int load_rooms_aux(char * filename){
  
  // open file
  FILE * file = fopen(filename, "r");
  if( file == NULL ){
    printf("File %s could not be opened\n", filename);
    exit(1);
  }
  
  size_t len = 0;
  char * name, * line = NULL;

  // create threads
  errno = 0;
  while( getline(&line, &len, file) != -1){

    if( line[0] == '#' ) // comment
      continue;

    if( line[strlen(line)-1] == '\n' )
      line[strlen(line)-1] = '\0';

    name = malloc( strlen(line) + 1 );
    if( name == NULL )
      errx(1, "malloc");

    strcpy(name, line);
    
    create_comm_thread(name);

  }

  if( errno != 0 )
    err(1,"getline");

  free(line);

  return 0;
}

int load_rooms(char * filename){

  pthread_mutex_lock(&thr_list_mx);

  int result = load_rooms_aux(filename);

  pthread_mutex_unlock(&thr_list_mx);

  return result;
}

void list_rooms_aux(){
  
  printf("Threads/Rooms:\n");

  struct thread_data * thread = thread_list;
  while( thread != NULL ){
    printf("%s\n", thread->name);

    thread = thread->next;
  }

}

void list_rooms(){

  pthread_mutex_lock(&thr_list_mx);

  list_rooms_aux();

  pthread_mutex_unlock(&thr_list_mx);

}

int room_present_aux(char * name){

  struct thread_data * thread = thread_list;
  while( thread != NULL ){
    
    if( strcmp(name, thread->name) == 0 )
      return true;

    thread = thread->next;
  }
  
  return false;
}

int room_present(char * name){

  pthread_mutex_lock(&thr_list_mx);

  int result = room_present_aux(name);

  pthread_mutex_unlock(&thr_list_mx);
  
  return result;

}

int insert_room_aux(char * filename, char * name){

  // find if a room is already present
  if( room_present(name) )
    return 1;
  
  // add room to file
  int file_fd;
  if( (file_fd = open(filename, O_WRONLY | O_APPEND, 0666)) == -1 )
    err(1,"open -- room file");

  write(file_fd, name, strlen(name));
  write(file_fd, "\n", 1);

  close(file_fd);

  // add thread for list
  create_comm_thread(name);

  return 0;
}

int insert_room(char * filename, char * name){

  pthread_mutex_lock(&thr_list_mx);

  int result = insert_room_aux(filename, name);

  pthread_mutex_unlock(&thr_list_mx);

  return result;

}
