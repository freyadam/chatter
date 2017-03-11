
#ifndef USERS_HEADER_H_
#define	USERS_HEADER_H_

int load_users_from_file(char * filename);
void list_users();
int insert_user(char * users_file, char * username, char * passwd);
int user_present(char * username, char * passwd);
int username_present(char * username, char * passwd);

struct user_pass {
		char * username;
		char * passwd;
		struct user_pass * next;
};

typedef struct user_pass user_pass_str;

char * user_file;
extern pthread_mutex_t users_mx;
extern struct user_pass * users;

#endif // USERS_HEADER_H_
