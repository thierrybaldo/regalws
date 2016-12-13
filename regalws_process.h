#ifndef REGALWS_PROCESS_H
#define REGALWS_PROCESS_H

#include <netdb.h>
#include <netinet/in.h>

struct regalws_server {
	int sockfd;
	struct sockaddr_in addr;
};

struct regalws_client {
	int sockfd;
	struct sockaddr_in addr;
};

int regalws_start_server(struct regalws_server *server, int port);

int regalws_wait_client(struct regalws_client *client, struct regalws_server *server);

int regalws_close_client(struct regalws_client *client);

int regalws_close_server(struct regalws_server *server);

int regalws_process_client(struct regalws_client *client, int *to_quit);

#endif /* REGALWS_PROCESS_H */
