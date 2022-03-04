#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


int clients = 0, fd_max = 0;
int	idx[65536];
char *msg[65536];
char rbuf[1025], wbuf[42];
fd_set rfds, wfds, fds;


int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}



char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void	fatal() {
	write(2, "Fatal error\n", 12);
	exit(1);
}

void	notify(int from, char *s) {
	for (int fd = 0; fd <= fd_max; fd++) {
		if (FD_ISSET(fd, &wfds) && fd != from)
			send(fd, s, strlen(s), 0);
	}
}


void	deliver(int fd) {
	char *s;

	while (extract_message(&(msg[fd]), &s)) {
		sprintf(wbuf, "client %d: ", idx[fd]);
		notify(fd, wbuf);
		notify(fd, s);
		free(s);
		s = NULL;
	}
}

void	add_client(int fd) {
	fd_max = fd > fd_max ? fd : fd_max;
	idx[fd] = clients++;
	msg[fd] = NULL;
	FD_SET(fd, &fds);
	sprintf(wbuf, "server: client %d just arrived\n", idx[fd]);
	notify(fd, wbuf);
}

void	remove_client(int fd) {
	sprintf(wbuf, "client %d just left the server\n", idx[fd]);
	notify(fd, wbuf);
	free(msg[fd]);
	msg[fd] = NULL;
	FD_CLR(fd, &fds);
	close(fd);
}

int main(int args, char **argv) {
	int sockfd, connfd, len;
	struct sockaddr_in servaddr, cli; 

	if (args != 2)
		fatal();
	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
		fatal();

	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1])); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		fatal();
	if (listen(sockfd, 10) != 0)
		fatal();
	// main loop
	FD_ZERO(&fds);
	FD_SET(fd_max, &fds);
	while (1) {
		rfds = wfds = fds;
		if (select(fd_max + 1, &rfds, &wfds, NULL, NULL) < 0)
			continue;
		for (int i = 0; i < fd_max ; i++) {
			if (FD_ISSET(i, &rfds)) {
				if (i == sockfd) {
					// accept connection and send it to all clients
					socklen_t		addr_len = sizeof(servaddr);
					int				client = accept(sockfd, NULL, NULL);
					if (client >= 0) {
						add_client(client);
					}
				} else {
					int readed = recv(i, rbuf, 1024, 0);
					if (readed <= 0) {
						remove_client(i);
						break ;
					}
					rbuf[readed] = '\0';
					msg[i] = str_join(msg[i], rbuf);
					deliver(i);
				}
			}
		}
	}
	return (0);
}