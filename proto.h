#ifndef PROTO_H_
#define PROTO_H_

#define EOF_IN_STREAM -2

int get_delim(int fd, char ** line_ptr, char del);
int get_dispatch(int fd, char ** prefix_ptr, char ** message_ptr);

#endif // PROTO_H_
