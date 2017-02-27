
#include "common.h"
#include "users.h"
#include "rooms.h"
#include "commands.h"

int main(int argc, char *argv[])
{

  /*
  users = NULL;
  char * user_file = "users";


  pthread_mutexattr_t mx_attr;
  if( pthread_mutexattr_init(&mx_attr) != 0)
    err(1,"pthread_mutexattr_init");
  if( pthread_mutexattr_settype(&mx_attr, PTHREAD_MUTEX_RECURSIVE) != 0)
    err(1,"pthread_mutexattr_settype");
  if( pthread_mutex_init(&users_mx, &mx_attr) != 0)
    err(1,"pthread_mutex_init");

  load_users_from_file(user_file);

  insert_user(user_file, "usr5", "past");
  list_users();

  if( user_present("usr2", "pas") )
    printf("Ok\n");
  else
    printf("Not present\n");

  if( user_present("usr5", "past") )
    printf("Ok\n");
  else
    printf("Not present\n");

  if( user_present("usr2", "past") )
    printf("Ok\n");
  else
    printf("Not present\n");
  */

  /*
  load_commands("commands");
  list_commands();
  printf("Cmd %s\n", get_command("ls"));

  if( get_command("a") == NULL )
    printf("None!\n");
  */

  return 0;
}

