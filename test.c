
#include "system_headers.h"
#include "users.h"


int main(int argc, char *argv[])
{

  users = NULL;

  char * user_file = "users";

  load_users_from_file(user_file);
  list_users();
  
  insert_user(user_file, "usr5", "past");
  load_users_from_file(user_file);

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

    return 0;
}

