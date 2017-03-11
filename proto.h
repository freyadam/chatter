#ifndef PROTO_H_
#define	PROTO_H_

#define	EOF_IN_STREAM -2
#define	DELIMITER ' '
#define	MAX_MSG_LEN_SIZE 6
#define MSG_PREFIX_LEN 17

int get_delim(int fd, char ** line_ptr, char del);
int get_dispatch(int fd, char ** prefix_ptr, char ** message_ptr);
int get_message(int fd, char ** contents_ptr);

int send_dispatch(int fd, char * dispatch);
int send_message(int fd, char * message);
int send_command(int fd, char * cmd);
int send_error(int fd);
int send_exit(int fd);
int send_end(int fd);
int send_end_to_all(struct pollfd * fds, int fds_size);
int send_message_to_all(struct pollfd * fds, int fds_size,
int exception, char * message);
int send_message_from_file(int fd, char * file_path);

#endif // PROTO_H_
