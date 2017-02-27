# README #

This is a simple chatting service that features a client and a server. Multiple clients can connect to a single server, each server can hold multiple chatrooms. 

### Features: ###

- Authentication via username/password pair
- Client can run commands on server
- Client can create new chatrooms
- Client can create new users

### Compilation ###

Project is by default built by running `make all` in main directory.

Alternatively you can build only server by running `make server` for server binary. Same applies for building client: `make client`.

Both binaries are stored in `bin` directory as `server` and `client` respectively.

### Running server ###

Server can be executed by running command `./bin/server <port>` where port means the number of the listening port of the new server. Also there are several optional parameters that the user can define. 

- `-c file` Specify the file in which commands that can be performed by client are stored. By default it is `./commands`. Each command is defined on a single line in a format `name_of_command command_itself`. Name and actual command are delimited by a space but other spaces are part of the command. Lines that start with `#` are ignored.

- `-u file` File with a list of username/password pairs. Each line holds a single pair and they are delimited by a space. Neither username nor password can contain any other spaces. `#` on the beginning of a line marks a comment and such a line is ignored.

- `-r file` Address of a file in which a list of rooms to be run is defined. Each chatroom name is on its own line. Again, lines beginning with `#` are ignored.

### Running client ###

Client can be executed by a command `./bin/client <address> <port>` where both address and port belong to server that the client connects to. Port of a client is selected automatically. Both IPv4 and IPv6 addresses are supported.

### Client usage ###

User controls the interaction simply by writing on command line and confirming his actions by pressing Enter. That means that each line represents a single message for the server. Some lines have special meaning. For example `/end` closes the connection and client while `/cmd rest_of_line` tries to perform a command on server.