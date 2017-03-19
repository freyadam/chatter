
#include "common.h"
#include "proto.h"

// get next delimited part of text
// without the delimiter (which will be consumed)
int get_delim(int fd, char ** line_ptr, char del) {

	if (*line_ptr != NULL) {
		free(*line_ptr);
	}

	int position, line_len, err_read;
	line_len = 10;
	position = 0;
	char * line = malloc(line_len);
	char c;

	while ((err_read = read(fd, &c, 1)) == 1) {

		if (position >= line_len - 1) {
			line_len += 1;
			line = realloc(line, line_len);
			if (line == NULL) {
				free(line);
				return (-1);
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
			line_len += 1;
			line = realloc(line, line_len);
			if (line == NULL) {
				free(line);
				return (-1);
			}
		}

		line[++position] = '\0';
		return (0);
	}

	return (position-1);	// don't include the null character
}

enum dispatch_t get_dispatch(int fd, char ** message_ptr) {

	*message_ptr = NULL;

	char * prefix = NULL;
	ssize_t prefix_len;

	// get prefix
	prefix_len = get_delim(fd, &prefix, DELIMITER);

	if (prefix_len == 0) {
		free(prefix);
		return (EOF_STREAM);
	} else if (prefix_len == -1) {
		free(prefix);
		return (FAILURE);
	} else if (prefix_len != 3) {
		free(prefix);
		return (FAILURE);
	}

	if (strcmp(prefix, "ERR") == 0) { // ERROR

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
		int err_arg = get_delim(fd, message_ptr, DELIMITER);
		if (err_arg == -1)
				return (FAILURE);
		else if (err_arg == 0)
				return (EOF_STREAM);

		// message_ptr already set in get_delim
		return (CMD);

	} else if (strcmp(prefix, "MSG") == 0) { // MESSAGE

		free(prefix);

		// get length of message
		int err_arg = get_delim(fd, message_ptr, DELIMITER);

		if (err_arg == -1) {
				return (FAILURE);

		} else if (err_arg == 0)
				return (EOF_STREAM);

		int msg_length = strtol(*message_ptr, NULL, 10);
		if (msg_length == 0 && errno == EINVAL)
		return (FAILURE);

		free(*message_ptr);

		*message_ptr = malloc(msg_length+1);
		if (message_ptr == NULL)
		return (FAILURE);

		// get the actual message
		int chars_read = 0;
		while (chars_read != msg_length+1) {
			err_arg = read(fd, (*message_ptr) + chars_read,
		msg_length+1-chars_read);

			if (err_arg == -1) {
				return (FAILURE);
			}

			chars_read += err_arg;
		}

		if ((*message_ptr)[msg_length] != DELIMITER) {
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

int get_message(int fd, char ** contents_ptr) {

	*contents_ptr = NULL;

	enum dispatch_t type = get_dispatch(fd, contents_ptr);

				switch (type) {
				case FAILURE:
					return (-1);
				case EOF_STREAM:
					return (EOF_IN_STREAM);
				case MSG:
					break;
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

	// dispatch: 'MSG MSG_LEN MESSAGE_ITSELF \0'
	// computation below corresponds to elements from dispatch,
	// summed from left to right
	int dispatch_len = 3 + 1 + MAX_MSG_LEN_SIZE
		+ 1 + strlen(message) + 1 + 1;
	char * dispatch = malloc(dispatch_len);

	int result = snprintf(dispatch, dispatch_len, "MSG %d %s ",
		(int)strlen(message), message);
	if (result < 0 || result > dispatch_len)
			return (-1);

	result = send_dispatch(fd, dispatch);

	free(dispatch);

	return (result);
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
	if (length_of_file == -1)
		err(1, "lseek");
	if (lseek(fildes, 0, SEEK_SET) == -1) // set file offset to start again
		err(1, "lseek");

	if (length_of_file >= 10000) {
		send_message(fd,
		"Failed to send message from file, file was too long.");
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
