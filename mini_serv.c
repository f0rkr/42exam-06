#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <fcntl.h>

typedef struct s_server {
	int					sockfd;
	int 				port;
	struct sockaddr_in	address;
}			t_server;

int		check_args(int args)
{
	if (args != 2)
		return (0);
	return (1);
}

void	print_error(const char *error) {
	write(1, error, strlen(error));
	return ;
}
t_server	*init_socket(char **argv)
{
	t_server	*socket;

	socket = (t_server *)malloc(sizeof(t_server) * 1);
	if (socket == NULL)
		return (socket);
	socket->port = atoi(argv[1]);
	memset(&(socket->address), 0, sizeof(socket->address));
	memset(&(socket->address.sin_zero), 0, sizeof(socket->address.sin_zero));	
	socket->address.sin_family = AF_INET;
	socket->address.sin_port = htons(socket->port);
	socket->address.sin_addr.s_addr = INADDR_ANY;
	return (socket);
}

int		main(int args, char **argv)
{
	t_server	*sock;

	// Check arguments and exit if any error
	if (!check_args(args))
	{
		print_error("Wrong number of arguments\n");
		return (1);
	}

	sock = init_socket(argv);
	if (sock == NULL)
	{
		print_error("Fatal error\n");
		return (1);
	}
	
	// Create server socket and bind to selected port and address	
	sock->sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock->sockfd < 0)
	{
		print_error("Fatal error\n");
		return (1);
	}

	if (fcntl(sock->sockfd, F_SETFL, O_NONBLOCK) < 0)
	{
		print_error("Fatal error\n");
		return (1);
	}

	if (bind(sock->sockfd, (const struct sockaddr *)&(sock->address), (socklen_t)sizeof(sock->address)) < 0)
	{
		print_error("Fatal error\n");
		return (1);
	}
	if (listen(sock->sockfd, 1024) < 0)
	{
		print_error("Fatal error\n");
		return (1);
	}

	// Start select server
	while (1) {
		int cfd = accept(sock->sockfd, 0, 0);
		
		send(cfd, "HELLO\n", strlen("HELLO\n"), 0);

		close(cfd);
	}
	free(sock);	
	return (0);
}
