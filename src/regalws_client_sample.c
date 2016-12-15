#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>

static int send_request(int port, const char *path)
{
	int ret = -1;
	int sockfd;
	struct sockaddr_in server_addr;
	struct hostent *server;
	int res;
	FILE *fhd = NULL;
	char buffer_date[2048];
	ssize_t nb;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(sockfd >= 0);

	server = gethostbyname("localhost");
	assert(server != NULL);

	server_addr.sin_family = AF_INET;
	memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	server_addr.sin_port = htons(port);

	res = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	assert(res == 0);

	fhd = fdopen(sockfd, "r+");
	assert(fhd != NULL);

	snprintf(buffer_date, sizeof(buffer_date), "GET %s HTTP/1.1\r\nHost: localhost\r\nUser-Agent: home-made\r\n\r\n", path);
	nb = write(sockfd, buffer_date, strlen(buffer_date));
	assert(nb == strlen(buffer_date));
//	printf("write()=%ld\n", nb);

	while (1) {
		nb = read(sockfd, buffer_date, sizeof(buffer_date));
//		printf("read()=%ld\n", nb);
		assert(nb >= 0);
		if (nb == 0)
			break;
		buffer_date[nb] = 0;
		printf("%s", buffer_date);
	}

	fclose(fhd);

	close(sockfd);
	ret = 0;

	printf("======================================\n");

	return ret;
}

int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;
	int port;

	if (argc < 2) {
		goto out;
	}
	port = atoi(argv[1]);

	send_request(port, "/date");
	send_request(port, "/status");
	send_request(port, "/status?field_a=1");
	send_request(port, "/question");
	send_request(port, "/response?happy=1&hungry=0");
	send_request(port, "/response");
	send_request(port, "/response?happy=1&hungry=1");
	send_request(port, "/response?happy=1&hungry=1&happy=0");
	send_request(port, "/notfound");
	send_request(port, "/quit");

	ret = 0;

out:
	return ret;
}
