#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>

static int send_request(const char *path)
{
	int ret = -1;
	int sockfd;
	struct sockaddr_in server_addr;
	struct hostent *server;
	int res;
	FILE *fhd;
	char buffer_date[2048];
	ssize_t nb;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(sockfd >= 0);

	server = gethostbyname("localhost");
	assert(server != NULL);

	server_addr.sin_family = AF_INET;
	memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	server_addr.sin_port = htons(12345);

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

	close(sockfd);
	ret = 0;

	printf("======================================\n");

	return ret;
}

int main(void)
{
	int ret = EXIT_FAILURE;

	send_request("/date");
	send_request("/status");
	send_request("/status?field_a=1");
	send_request("/question");
	send_request("/response?happy=1&hungry=0");
	send_request("/response");
	send_request("/response?happy=1&hungry=1");
	send_request("/response?happy=1&hungry=1&happy=0");
	send_request("/notfound");
	send_request("/quit");

	ret = 0;
	
	return ret;
}
