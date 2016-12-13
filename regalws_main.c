#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>

#include "regalws.h"
#include "regalws_process.h"

int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;
	struct regalws_server server;
	struct regalws_client client;
	int res;
	int to_quit = 0;
	int port;

	if (argc < 2) {
		goto out;
	}

	port = atoi(argv[1]);

	res = regalws_start_server(&server, port);
	if (res < 0) {
		goto out;
	}

	while (!to_quit) {
		res = regalws_wait_client(&client, &server);
		if (res < 0) {
			goto out;
		}

		res = regalws_process_client(&client, &to_quit);
		if (res < 0) {
			fprintf(stderr, "regalws_process_client() error\n");
		}

		res = regalws_close_client(&client);
		if (res < 0) {
			goto out;
		}

	}

	ret = 0;

out:

	return ret;
}
