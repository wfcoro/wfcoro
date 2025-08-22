#include <stdio.h>
#include <string.h>
#include <string>

#include "workflow/HttpUtil.h"
#include "workflow/WFFacilities.h"
#include "workflow/WFTaskFactory.h"

#include "wfcoro/global.h"

#define REDIRECT_MAX 5
#define RETRY_MAX 2

WFFacilities::WaitGroup wait_group(1);

void wget_callback(WFHttpTask *task)
{
	protocol::HttpRequest *req = task->get_req();
	protocol::HttpResponse *resp = task->get_resp();
	int state = task->get_state();
	int error = task->get_error();

	if (state != WFT_STATE_SUCCESS)
	{
		const char *err = WFGlobal::get_error_string(state, error);
		fprintf(stderr, "Failed %s\n", err);
		return;
	}

	fprintf(stderr, "%s %s %s\r\n", req->get_method(), req->get_http_version(),
			req->get_request_uri());

	std::string name;
	std::string value;
	protocol::HttpHeaderCursor req_cursor(req);

	while (req_cursor.next(name, value))
		fprintf(stderr, "%s: %s\r\n", name.c_str(), value.c_str());
	fprintf(stderr, "\r\n");

	/* Print response header. */
	fprintf(stderr, "%s %s %s\r\n", resp->get_http_version(),
			resp->get_status_code(), resp->get_reason_phrase());

	protocol::HttpHeaderCursor resp_cursor(resp);
	while (resp_cursor.next(name, value))
		fprintf(stderr, "%s: %s\r\n", name.c_str(), value.c_str());
	fprintf(stderr, "\r\n");

	/* Print response body. */
	const void *body;
	size_t body_len;

	resp->get_parsed_body(&body, &body_len);
	fwrite(body, 1, body_len, stdout);
	fflush(stdout);

	wait_group.done();
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "USAGE: %s <http URL>\n", argv[0]);
		return 1;
	}

	// Attach wfcoro to workflow, but we can still use task and series,
	// existing code don't need any changes if it does not use wfcoro.
	wfcoro::global_init(wfcoro::CoroConfig{});

	std::string url = argv[1];
	if (strncasecmp(argv[1], "http://", 7) != 0 &&
		strncasecmp(argv[1], "https://", 8) != 0)
	{
		url = "http://" + url;
	}

	WFHttpTask *task;
	task = WFTaskFactory::create_http_task(url, REDIRECT_MAX, RETRY_MAX,
										   wget_callback);
	protocol::HttpRequest *req = task->get_req();
	req->add_header_pair("Accept", "*/*");
	req->add_header_pair("User-Agent", "Wget/1.14 (linux-gnu)");
	req->add_header_pair("Connection", "close");
	task->start();

	wait_group.wait();
	return 0;
}
