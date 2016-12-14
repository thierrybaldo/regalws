#include "regalws.h"

#include <string.h>
#include <time.h>

static int handle_command_default(struct regalws_request *request, struct regalws_response *response)
{
	response->http_protocol = request->http_protocol;
	response->http_code = 200;
	response->content_type = "text/html";
	fprintf(response->fhd, "<html><head><title>Regalws</title><body>Regalws</body></html>\r\n");

	return 0;
}

static int handle_command_date(struct regalws_request *request, struct regalws_response *response)
{
	char date_str[128];
	struct tm tm;
	time_t now = time(NULL);

	tm = *localtime(&now);

	strftime(date_str, sizeof(date_str), "%H:%M:%S %F", &tm);

	response->http_protocol = request->http_protocol;
	response->http_code = 200;
	response->content_type = "text/html";
	fprintf(response->fhd, "<html><head><title>Date</title><body>Date %s<br></body></html>\r\n", date_str);

	return 0;
}

static int handle_command_status(struct regalws_request *request, struct regalws_response *response)
{
	response->http_protocol = request->http_protocol;
	response->http_code = 200;
	response->content_type = "text/html";

	fprintf(response->fhd, "<html><head><title>Status</title><body>");
	fprintf(response->fhd, "Status field_a=%s<br>", regalws_get_arg(request, "field_a", "0"));
	fprintf(response->fhd, "User-Agent=%s<br>", regalws_get_header(request, "User-Agent", "(unknown)"));
	fprintf(response->fhd, "</body></html>\r\n");

	return 0;
}

static int handle_command_question(struct regalws_request *request, struct regalws_response *response)
{
	response->http_protocol = request->http_protocol;
	response->http_code = 200;
	response->content_type = "text/html";

	fprintf(response->fhd, "<html><head><title>Question</title><body>");
	fprintf(response->fhd, "<form action=\"response\" method=\"get\">");
	fprintf(response->fhd, "<input type=\"checkbox\" name=\"happy\" value=\"1\" id=\"happy\">");
	fprintf(response->fhd, "<label for=\"happy\">Happy</label>");
	fprintf(response->fhd, "</input>");
	fprintf(response->fhd, "<input type=\"checkbox\" name=\"hungry\" value=\"1\" id=\"hungry\">");
	fprintf(response->fhd, "<label for=\"hungry\">Hungry</label>");
	fprintf(response->fhd, "</input>");
	fprintf(response->fhd, "<input type=\"submit\"/>");
	fprintf(response->fhd, "</form>");
	fprintf(response->fhd, "</body></html>\r\n");

	return 0;
}

static int handle_command_response(struct regalws_request *request, struct regalws_response *response)
{
	response->http_protocol = request->http_protocol;
	response->http_code = 200;
	response->content_type = "text/html";

	fprintf(response->fhd, "<html><head><title>Response</title><body>");
	fprintf(response->fhd, "Happy=%s<br>", regalws_get_arg(request, "happy", "0"));
	fprintf(response->fhd, "Hungry=%s<br>", regalws_get_arg(request, "hungry", "0"));
	fprintf(response->fhd, "</body></html>\r\n");

	return 0;
}

static int handle_command_quit(struct regalws_request *request, struct regalws_response *response)
{
	response->http_protocol = request->http_protocol;
	response->http_code = 200;
	response->content_type = "text/html";

	fprintf(response->fhd, "<html><head><title>Quit</title><body>Quit<br></body></html>\r\n");

	response->to_quit = 1;

	return 0;
}

static int handle_default(struct regalws_request *request, struct regalws_response *response)
{
	response->http_protocol = request->http_protocol;
	response->http_code = 404;
	response->content_type = "text/html";

	fprintf(response->fhd, "<html><head><title>Not found</title><body>Not found</body></html>\r\n");

	return 0;
}

int regalws_handle(struct regalws_request *request, struct regalws_response *response)
{
	int ret;
	printf("path=<%s>\n", request->path);

	if (!strcmp(request->path, "/")) {
		ret = handle_command_default(request, response);
	} else if (!strcmp(request->path, "/date")) {
		ret = handle_command_date(request, response);
	} else if (!strcmp(request->path, "/status")) {
		ret = handle_command_status(request, response);
	} else if (!strcmp(request->path, "/question")) {
		ret = handle_command_question(request, response);
	} else if (!strcmp(request->path, "/response")) {
		ret = handle_command_response(request, response);
	} else if (!strcmp(request->path, "/quit")) {
		ret = handle_command_quit(request, response);
	} else {
		ret = handle_default(request, response);
	}

	return ret;
}
