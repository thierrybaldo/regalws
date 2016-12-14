#ifndef REGALWS_H
#define REGALWS_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

enum regalws_http_protocol {
	REGALWS_HTTP_PROTOCOL_1_0 = 0,
	REGALWS_HTTP_PROTOCOL_1_1 = 1,
	REGALWS_HTTP_PROTOCOL_2_0 = 2,
};

struct regalws_list {
    struct regalws_list *next, *prev;
};

struct regalws_key {
	char *key;
	char *value;
	struct regalws_list list;
};

struct regalws_request {
	enum regalws_http_protocol http_protocol;
	char path[FILENAME_MAX];
	char *url;
	int argument_count;
	struct regalws_list arguments;
	int header_count;
	struct regalws_list headers;
};

struct regalws_response {
	enum regalws_http_protocol http_protocol;
	int http_code;
	char *content_type;
	FILE *fhd;
	int to_quit;
};

int regalws_handle(struct regalws_request *request, struct regalws_response *response);

const char *regalws_get_arg(struct regalws_request *request, const char *key, const char *default_value);

const char *regalws_get_header(struct regalws_request *request, const char *key, const char *default_value);

#ifdef __cplusplus
}
#endif

#endif /* REGALWS_H */
