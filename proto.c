
#include "common.h"
#include "proto.h"

// get next delimited part of text
// without the delimiter (which will be consumed)
int get_delim(int fd, char ** line_ptr, char del, int max_len) {

	int position, line_len, err_read;
	line_len = 10;
	position = 0;
	char * line_new, * line = malloc(line_len);
	char c;

	assert(*line_ptr == NULL);

	while ((err_read = read(fd, &c, 1)) == 1) {

		if (position >= line_len - 1) {
			line_len++;

			if (line_len >= max_len) { // message is too long
				line_len--;
				break;
			}

			line_new = realloc(line, line_len);
			if (line_new == NULL) {
				free(line);
				return (-1);
			} else {
				line = line_new;
			}
		}

		line[position++] = c;
		if (c == del) {
			line[position-1] = '\0';
			break;
		}

	}

	if (err_read == -1) {
		free(line);
		return (-1);
	}

	*line_ptr = line;

	if (err_read == 0) {
		if (position >= line_len - 1) {
			line_len++;
			line_new = realloc(line, line_len);
			if (line_new == NULL) {
				free(line);
				return (-1);
			} else {
				line = line_new;
			}
		}

		line[++position] = '\0';
		return (0);
	}

	return (position-1);	// don't include the null character
}


// get next delimited part of text
// without the delimiter (which will be consumed)
// message will be saved inside 'space' which needs to
// have at least size max_len
int get_delim_no_alloc(int fd, char ** space_ptr, char del, int max_len) {

	int position, err_read;
	position = 0;
	char c, * space = *space_ptr;

	while ((err_read = read(fd, &c, 1)) == 1) {

		// current message would be writen outside allocated space
		// not counting necessary end char
		if (position >= max_len - 1) {
			break;
		}

		space[position++] = c;
		if (c == del) {
			space[position-1] = '\0';
			break;
		}

	}

	if (err_read == -1) {
					return (-1);
	}

	if (err_read == 0) {
					space[++position] = '\0';
					return (0);
	}

	return (position-1);	// don't include the null character
}

enum dispatch_t get_dispatch(int fd, char ** message_ptr) {

	char * prefix = NULL;
	ssize_t prefix_len;

	assert(*message_ptr == NULL);

	*message_ptr = NULL;

	// get prefix
	prefix_len = get_delim(fd, &prefix, DELIMITER, 4);

	if (prefix_len == 0) {

		free(prefix);
		return (EOF_STREAM);

	} else if (prefix_len == -1) {

		free(prefix);
		return (FAILURE);

	} else if (prefix_len != 3) {

		free(prefix);
		return (FAILURE);

	} else if (strcmp(prefix, "ERR") == 0) { // ERROR

		free(prefix);
		*message_ptr = NULL;
		return (ERR);

	} else if (strcmp(prefix, "EXT") == 0) { // EXIT

		free(prefix);
		*message_ptr = NULL;
		return (EXT);

	} else if (strcmp(prefix, "END") == 0) { // END OF TRANSMISSION

		free(prefix);
		*message_ptr = NULL;
		return (END);

	} else if (strcmp(prefix, "CMD") == 0) { // COMMAND

		free(prefix);

		// get the actual command
		int err_arg = get_delim(fd, message_ptr,
		DELIMITER, MAX_MSG_LEN);
		if (err_arg == -1) {
			return (FAILURE);
		} else if (err_arg == 0) {
			free(*message_ptr);
			return (EOF_STREAM);
		}

		// message_ptr already set in get_delim
		return (CMD);

	} else if (strcmp(prefix, "MSG") == 0) { // MESSAGE

		free(prefix);

		// get length of message
		int err_arg = get_delim(fd, message_ptr, DELIMITER, 10);

		if (err_arg == -1) {
			return (FAILURE);
		} else if (err_arg == 0) {
			free(*message_ptr);
			return (EOF_STREAM);
		}

		int msg_length = strtol(*message_ptr, NULL, 10);
		free(*message_ptr);

		if (msg_length == 0 && errno == EINVAL) {
			return (FAILURE);
		} else if (msg_length > MAX_MSG_LEN) {
			return (FAILURE);
		}

		*message_ptr = malloc(msg_length+1);
		if (message_ptr == NULL)
		return (FAILURE);

		// get the actual message
		int chars_read = 0;
		while (chars_read != msg_length+1) {
			err_arg = read(fd, (*message_ptr) + chars_read,
		msg_length+1-chars_read);

			if (err_arg == -1) {
				free(*message_ptr);
				return (FAILURE);
			}

			chars_read += err_arg;
		}

		if ((*message_ptr)[msg_length] != DELIMITER) {
			free(*message_ptr);
			return (FAILURE);
		}

		(*message_ptr)[msg_length] = '\0';

		// message_ptr already set previously

		return (MSG);

	} else { // UNKNOWN PREFIX

		free(prefix);
		return (FAILURE);

	}

	// program should never reach this part
	free(prefix);
	assert(false);
	return (FAILURE);

}

enum dispatch_t get_dispatch_no_alloc(int fd, char ** space_ptr, int max_len) {

	ssize_t prefix_len;

	// get prefix
				assert(max_len >= 5);
	prefix_len = get_delim_no_alloc(fd, space_ptr, DELIMITER, 5);

	if (prefix_len == 0) {

		return (EOF_STREAM);

	} else if (prefix_len == -1) {

		return (FAILURE);

	} else if (prefix_len != 3) {

		return (FAILURE);

	} else if (strcmp(*space_ptr, "ERR") == 0) { // ERROR

		return (ERR);

	} else if (strcmp(*space_ptr, "EXT") == 0) { // EXIT

		return (EXT);

	} else if (strcmp(*space_ptr, "END") == 0) { // END OF TRANSMISSION

		return (END);

	} else if (strcmp(*space_ptr, "CMD") == 0) { // COMMAND

		// get the actual command
		int err_arg = get_delim_no_alloc(fd, space_ptr,
		DELIMITER, max_len);
		if (err_arg == -1) {
			return (FAILURE);
		} else if (err_arg == 0) {
			return (EOF_STREAM);
		}

		return (CMD);

	} else if (strcmp(*space_ptr, "MSG") == 0) { // MESSAGE

		// get length of message

		int err_arg = get_delim_no_alloc(fd, space_ptr, DELIMITER, 10);

		if (err_arg == -1) {
			return (FAILURE);
		} else if (err_arg == 0) {
			return (EOF_STREAM);
		}

		int msg_length = strtol(*space_ptr, NULL, 10);

		if (msg_length == 0 && errno == EINVAL) {
			return (FAILURE);
		} else if (msg_length > max_len-1) {
			return (FAILURE);
		}

		// get the actual message
		int chars_read = 0;
		while (chars_read != msg_length+1) {
			err_arg = read(fd, (*space_ptr) + chars_read,
		msg_length+1-chars_read);

			if (err_arg == -1) {
				return (FAILURE);
			}

			chars_read += err_arg;
		}

		if ((*space_ptr)[msg_length] != DELIMITER) {
			return (FAILURE);
		}

		(*space_ptr)[msg_length] = '\0';

		// message_ptr already set previously

		return (MSG);

	} else { // UNKNOWN PREFIX
		return (FAILURE);
	}

	// program should never reach this part
	assert(false);
	return (FAILURE);

}


int get_message(int fd, char ** contents_ptr) {

	assert(*contents_ptr == NULL);

	enum dispatch_t type = get_dispatch(fd, contents_ptr);

				switch (type) {
				case FAILURE:
					return (-1);
				case EOF_STREAM:
					return (EOF_IN_STREAM);
				case MSG:
					break;
				case CMD:
					free(*contents_ptr);
					return (-1);
				default:
					return (-1);
				}

	return (0);

}

int send_dispatch(int fd, char * dispatch) {

	int sum, result;
	sum = 0;

	while (sum < strlen(dispatch)) {

		result = write(fd, dispatch + sum, strlen(dispatch));

		if (result == -1)
			return (-1);

		sum += result;

	}

	return (0);
}

int send_message(int fd, char * message) {

	char msg_len_str[MAX_MSG_LEN_SIZE];

	if (send_dispatch(fd, "MSG ") == -1)
		return (-1);

	int result = snprintf(msg_len_str, MAX_MSG_LEN_SIZE, "%d ",
    (int)strlen(message));
	if (result < 0 || result > MAX_MSG_LEN_SIZE)
		return (-1);

	if (send_dispatch(fd, msg_len_str) == -1)
		return (-1);

	if (send_dispatch(fd, message) == -1)
		return (-1);

	if (send_dispatch(fd, " ") == -1)
		return (-1);
	else
		return (0);

}

int send_message_f(int fd, char * format, ...) {

	char formatted_message[MAX_MSG_LEN];

	va_list args;
	va_start(args, format);

	int chars_written = vsnprintf(formatted_message,
		MAX_MSG_LEN, format, args);
	if (chars_written < 0 || chars_written > MAX_MSG_LEN)
		return (-1);

	send_message(fd, formatted_message);

	va_end(args);

	return (0);
}

int send_command(int fd, char * cmd) {

	char * dispatch = malloc((3 + 1 + strlen(cmd) + 1 + 1));

	int cmd_len = 3 + 1 + strlen(cmd) + 2;
	int result = snprintf(dispatch, cmd_len,
		"CMD %s ", cmd);
	if (result < 0 || result > cmd_len)
			return (-1);

	result = send_dispatch(fd, dispatch);

	free(dispatch);

	return (result);
}

int send_error(int fd) {

		return (send_dispatch(fd, "ERR "));

}

int send_exit(int fd) {

		return (send_dispatch(fd, "EXT "));

}

int send_end(int fd) {

		return (send_dispatch(fd, "END "));

}

int send_end_to_all(struct pollfd * fds, int fds_size) {

	int i, result;
	for (i = 0; i < fds_size; i++) {

		result = send_end(fds[i].fd);
		if (result == -1)
			printf(
		"Client %d could not receive 'end of transmission' message.\n",
		i);
	}

	return (0);
}

int send_message_to_all(struct pollfd * fds, int fds_size,
		int exception, char * message) {

	int i, result;
	for (i = 0; i < fds_size; i++) {

		if (i != exception) {
			result = send_message(fds[i].fd, message);
			if (result == -1)
				printf("Client %d could not receive message.\n",
		i);
		}
	}

	return (0);
}


int send_message_from_file(int fd, char * file_path) {

	int fildes = open(file_path, O_RDONLY, 0666);

	// get file length
	int length_of_file = (int)lseek(fildes, 0, SEEK_END);

	printf("len of file: %d\n", length_of_file);

	if (length_of_file == -1)
		err(1, "lseek");
	if (lseek(fildes, 0, SEEK_SET) == -1) // set file offset to start again
		err(1, "lseek");

	if (length_of_file >= MAX_MSG_LEN) {
		send_message(fd,
		"Failed to send message from file, file was too long.");
		close(fildes);
		return (-1);
	}

	// write header
	char prefix[MSG_PREFIX_LEN];
	snprintf(prefix, MSG_PREFIX_LEN, "MSG %d ", length_of_file);
	send_dispatch(fd, prefix);

	// write contents of file
	int buffer_size, rd;
	buffer_size = 100;
	char * buffer = malloc(buffer_size + 1);

	while ((rd = read(fildes, buffer, buffer_size - 1)) > 0) {

		buffer[rd] = '\0';
		if (send_dispatch(fd, buffer) != 0)
			errx(1, "send_dispatch -- send_message_from_file");

	}

	if (rd == -1)
		err(1, "read");

	free(buffer);

	close(fildes);

	// finish message
	if (send_dispatch(fd, " ") != 0)
		errx(1, "send_dispatch -- send_message_from_file");

	return (0);

}
