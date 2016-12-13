#include <uriparser/Uri.h>
#include <http_parser.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include <errno.h>

#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#include "regalws.h"
#include "regalws_process.h"

static inline void list_init(struct regalws_list *list)
{
        list->next = list;
        list->prev = list;
}

static inline void __list_add(struct regalws_list *new,
                              struct regalws_list *prev,
                              struct regalws_list *next)
{
        next->prev = new;
        new->next = next;
        new->prev = prev;
        prev->next = new;
}

static inline void list_add_tail(struct regalws_list *new, struct regalws_list *head)
{
        __list_add(new, head->prev, head);
}

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#define list_entry(ptr, type, member) \
        container_of(ptr, type, member)

#define list_last_entry(ptr, type, member) \
        list_entry((ptr)->prev, type, member)

#define list_for_each(pos, head) \
        for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head) \
        for (pos = (head)->next, n = pos->next; pos != (head); \
                pos = n, n = pos->next)

static const char *regalws_get(struct regalws_list *list, const char *key, const char *default_value)
{
	struct regalws_list *element;
	struct regalws_key *entry;

	list_for_each(element, list) {
		entry = list_entry(element, struct regalws_key, list);
		if (!strcmp(entry->key, key)) {
			return entry->value;
		}
	}

	return default_value;
}

const char *regalws_get_arg(struct regalws_request *request, const char *key, const char *default_value)
{
	return regalws_get(&request->arguments, key, default_value);
}

const char *regalws_get_header(struct regalws_request *request, const char *key, const char *default_value)
{
	return regalws_get(&request->headers, key, default_value);
}

int regalws_start_server(struct regalws_server *server, int port)
{
	int ret = -1;
	int option;
	int res;

	assert(server != NULL);

	server->sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (server->sockfd < 0) {
		goto out;
	}

	memset(&server->addr, 0, sizeof(server->addr));
	server->addr.sin_family = AF_INET;
	server->addr.sin_addr.s_addr = INADDR_ANY;
	server->addr.sin_port = htons(port);

	option = 1;
	setsockopt(server->sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	res = bind(server->sockfd, (struct sockaddr *) &server->addr, sizeof(server->addr));
	if (res < 0) {
		goto out;
	}

	res = listen(server->sockfd, 5);
	if (res < 0) {
		goto out;
	}

	ret = 0;
out:
	return ret;
}

int regalws_wait_client(struct regalws_client *client, struct regalws_server *server)
{
	int ret = -1;
	socklen_t length;

	assert(client != NULL);
	assert(server != NULL);

	length = sizeof(&client->addr);
	client->sockfd = accept(server->sockfd, (struct sockaddr *)&client->addr, &length);
	if (client->sockfd < 0) {
		goto out;
	}

	ret = 0;

out:
	return ret;
}

int regalws_close_client(struct regalws_client *client)
{
	if (client == NULL || client->sockfd < 0) {
		return -EINVAL;
	}
	close(client->sockfd);

	return 0;
}

int regalws_close_server(struct regalws_server *server)
{
	if (server == NULL || server->sockfd < 0) {
		return -EINVAL;
	}
	close(server->sockfd);

	return 0;
}

static int read_buffer_in(struct regalws_client *client, char *buffer, size_t *size_p)
{
	int ret = -1;
	int nb;
	char *p;
	size_t size = *size_p;

	p = buffer;

	while (size > 0) {
		nb = recv(client->sockfd, p, size, 0);
		if (nb <= 0) {
			goto out;
		}
		size -= nb;
		if (strstr(p, "\r\n\r\n")) {
			break;
		}
	}
	*size_p = size;
	ret = 0;
out:
	return ret;
}

static int parse_args(const char *arguments, struct regalws_request *request)
{
	int ret = -1;
	UriQueryListA *list = NULL;
	UriQueryListA *element;
	struct regalws_key *argument;
	int count;
	int i;

	if (uriDissectQueryMallocA(&list, &count, arguments, arguments+strlen(arguments)) != 0) {
		goto out;
	}

	element = list;
	for (i = 0; i < count; i++) {
		argument = malloc(sizeof(*argument));
		if (argument == NULL) {
			goto out;
		}
		argument->key = strdup(element->key);
		argument->value = strdup(element->value);
		list_add_tail(&argument->list, &request->arguments);
		request->argument_count++;
		element = element->next;
	}
	assert(request->argument_count == count);
	ret = 0;
out:
	if (list != NULL) {
		uriFreeQueryListA(list);
	}
	return ret;
}

static int send_all(int fd, const void *buffer, size_t size)
{
	int ret = -1;
	ssize_t nb;
	const uint8_t *p;

	p = buffer;

	while (size > 0) {
		nb = send(fd, p, size, 0);
		if (nb < 0) {
			goto out;
		}
		p += nb;
		size -= nb;
	}
	ret = 0;
out:
	return ret;
}

static const char *get_http_str(int code)
{
	switch(code) {
		case 200:
			return "OK";
		case 404:
			return "Not Found";
	}
	return "UNKNOWN";
}

static int on_message_begin(http_parser* parser)
{
	printf("%s()\n", __func__);
	return 0;
}

static int on_url(http_parser *parser, const char *at, size_t length)
{
	int ret = -1;
	struct regalws_request *request = parser->data;
	char *p;
	int len;

	printf("%s(", __func__);
	fwrite(at, 1, length, stdout);
	printf(", length=%ld)\n", length);
	request->url = malloc(length+1);
	if (request->url == NULL) {
		goto out;
	}
	memcpy(request->url, at, length);
	request->url[length] = 0;

	p = strchr(request->url, '?');
	if (p == NULL) {
		len = MIN(sizeof(request->path), strlen(request->url));
		memcpy(request->path, request->url, len);
		request->path[len] = 0;
	} else {
		len = MIN(sizeof(request->path), p - request->url);
		memcpy(request->path, request->url, len);
		request->path[len] = 0;
		parse_args(p+1, request);
	}

	ret = 0;
out:
	return ret;
}

static int on_status_complete(http_parser* parser)
{
	printf("%s()\n", __func__);
	return 0;
}

static int on_header_field(http_parser *parser, const char *at, size_t length)
{
	int ret = -1;
	struct regalws_request *request = parser->data;
	struct regalws_key *header;

#if 0
	printf("%s(", __func__);
	fwrite(at, 1, length, stdout);
	printf(")\n");
#endif

	header = malloc(sizeof(*header));
	if (header == NULL) {
		goto out;
	}
	header->key = malloc(length+1);
	if (header->key == NULL) {
		free(header);
		goto out;
	}
	memcpy(header->key, at, length);
	header->key[length] = 0;
	header->value = NULL;
	list_add_tail(&header->list, &request->headers);
	request->header_count++;

	ret = 0;

out:
	return ret;
}

static int on_header_value(http_parser *parser, const char *at, size_t length)
{
	int ret = -1;
	struct regalws_request *request = parser->data;
	struct regalws_key *header;

#if 0
	printf("%s(", __func__);
	fwrite(at, 1, length, stdout);
	printf(")\n");
#endif

	header = list_last_entry(&request->headers, struct regalws_key, list);
	if (header != NULL && header->value == NULL) {
		header->value = malloc(length+1);
		if (header->value == NULL) {
			goto out;
		}
		memcpy(header->value, at, length);
		header->value[length] = 0;
		ret = 0;
	}

out:
	return ret;
}

static int on_headers_complete(http_parser* parser)
{
	int ret = -1;
	struct regalws_request *request = parser->data;
	printf("%s() %s %d.%d\n", __func__, http_method_str(parser->method), parser->http_major, parser->http_minor);
	if (parser->http_major == 1 && parser->http_minor == 0)
		request->http_protocol = REGALWS_HTTP_PROTOCOL_1_0;
	else if (parser->http_major == 1 && parser->http_minor == 1)
		request->http_protocol = REGALWS_HTTP_PROTOCOL_1_1;
	else if (parser->http_major == 2 && parser->http_minor == 0)
		request->http_protocol = REGALWS_HTTP_PROTOCOL_2_0;
	else {
		goto out;
	}
	ret = 0;
out:
	return ret;
}

static int on_body(http_parser *parser, const char *at, size_t length)
{
	printf("%s()\n", __func__);
	printf("  [");
	fwrite(at, 1, length, stdout);
	printf("]\n");
	return 0;
}

static int on_message_complete(http_parser* parser)
{
	printf("%s()\n", __func__);
	return 0;
}

int regalws_process_client(struct regalws_client *client, int *to_quit)
{
	int ret = -1;
	char buffer_in[2048];
	size_t buffer_in_size;
	struct regalws_request request;
	struct regalws_response response;
	struct http_parser parser;
	http_parser_settings settings = {
		.on_message_begin    = on_message_begin,
		.on_url              = on_url,
		.on_status_complete  = on_status_complete,
		.on_header_field     = on_header_field,
		.on_header_value     = on_header_value,
		.on_headers_complete = on_headers_complete,
		.on_body             = on_body,
		.on_message_complete = on_message_complete,
	};
	size_t size;
	FILE *http_fhd = NULL;
	char *http_ptr= NULL;
	size_t http_loc;
	char *buffer_ptr = NULL;
	size_t buffer_loc;
	char http_protocol[] = "X.X";
	struct regalws_list *element;
	struct regalws_list *tmp;
	struct regalws_key *entry;

	memset(&request, 0, sizeof(request));
	memset(&response, 0, sizeof(response));

	list_init(&request.arguments);
	list_init(&request.headers);

	buffer_in_size = sizeof(buffer_in) - 1;
	if (read_buffer_in(client, buffer_in, &buffer_in_size) < 0) {
		fprintf(stderr, "read_buffer_in() error\n");
		goto out;
	}
	buffer_in[buffer_in_size] = 0;
#if 0
	printf("[%s]\n", buffer_in);
#endif

	http_parser_init(&parser, HTTP_REQUEST);
	parser.data = &request;
	size = http_parser_execute(&parser, &settings, buffer_in, buffer_in_size);
	printf("http_parser_execute()=%ld\n", size);
	printf("url=%s\n", request.url);
	printf("header_count=%d\n", request.header_count);
	printf("argument_count=%d\n", request.argument_count);

	response.fhd = open_memstream(&buffer_ptr, &buffer_loc);
	if (response.fhd == NULL) {
		goto out;
	}

	if (regalws_handle(&request, &response) < 0) {
		goto out;
	}
	*to_quit = response.to_quit;
	fflush(response.fhd);

	if (response.http_protocol == REGALWS_HTTP_PROTOCOL_1_0) {
		strcpy(http_protocol, "1.0");
	} else if (request.http_protocol == REGALWS_HTTP_PROTOCOL_1_1) {
		strcpy(http_protocol, "1.1");
	} else if (request.http_protocol == REGALWS_HTTP_PROTOCOL_2_0) {
		strcpy(http_protocol, "2.0");
	} else {
		goto out;
	}
	http_fhd = open_memstream(&http_ptr, &http_loc);
	if (http_fhd == NULL) {
		goto out;
	}
	fprintf(http_fhd, "HTTP/%s %d %s\r\n", http_protocol, response.http_code, get_http_str(response.http_code));
	fprintf(http_fhd, "Content-Type: %s\r\n\r\n", response.content_type);
	fflush(http_fhd);

	if (send_all(client->sockfd, http_ptr, http_loc) < 0) {
		goto out;
	}

	if (send_all(client->sockfd, buffer_ptr, buffer_loc) < 0) {
		goto out;
	}

	ret = 0;

out:
	free(request.url);
	list_for_each_safe(element, tmp, &request.headers) {
		entry = list_entry(element, struct regalws_key, list);
		free(entry->key);
		free(entry->value);
		free(entry);
	}

	if (http_fhd != NULL)
		fclose(http_fhd);
	free(http_ptr);

	if (response.fhd != NULL)
		fclose(response.fhd);
	free(buffer_ptr);

	list_for_each_safe(element, tmp, &request.arguments) {
		entry = list_entry(element, struct regalws_key, list);
		free(entry->key);
		free(entry->value);
		free(entry);
	}

	return ret;
}
