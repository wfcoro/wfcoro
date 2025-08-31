#include <stdio.h>
#include <stdlib.h>
#include <utility>

#include "workflow/HttpUtil.h"
#include "workflow/WFFacilities.h"
#include "workflow/WFHttpServer.h"

#include "wfcoro/event.h"
#include "wfcoro/global.h"

void process(WFHttpTask *proxy_task)
{
	auto *req = proxy_task->get_req();
	std::string url = req->get_request_uri();
	bool keep_alive = req->is_keep_alive();

	WFHttpTask *http_task = WFTaskFactory::create_http_task(url, 0, 0, nullptr);

	const void *body;
	size_t len;

	/* Copy user's request to the new task's reuqest using std::move() */
	req->set_request_uri(http_task->get_req()->get_request_uri());
	req->get_parsed_body(&body, &len);
	req->append_output_body_nocopy(body, len);
	*http_task->get_req() = std::move(*req);

	/* also, limit the remote webserver response size. */
	http_task->get_resp()->set_size_limit(8 * 1024 * 1024);

	wfcoro::Event ev;
	int state, error;
	protocol::HttpResponse resp;

	http_task->set_callback([&, ev](WFHttpTask *task) {
		state = task->get_state();
		error = task->get_error();
		resp = std::move(*task->get_resp());

		// Wakeup ev.wait.
		ev.signal();
	});

	// Start http task.
	http_task->start();

	// Waiting for http task to finish.
	ev.wait();

	// http_task is destroyed at this moment.

	protocol::HttpResponse *proxy_resp = proxy_task->get_resp();
	if (state == WFT_STATE_SUCCESS)
	{
		const void *body;
		size_t len;

		resp.get_parsed_body(&body, &len);
		resp.append_output_body_nocopy(body, len);
		*proxy_resp = std::move(resp);

		if (!keep_alive)
			proxy_resp->set_header_pair("Connection", "close");

		fprintf(stderr, "%s success, status=%s bodylen=%zu\n", url.c_str(),
				proxy_resp->get_status_code(), len);
	}
	else
	{
		const char *errstr = WFGlobal::get_error_string(state, error);
		fprintf(stderr, "%s failed, state=%d error=%d err=%s\n", url.c_str(),
				state, error, errstr);

		proxy_resp->set_status_code("404");
		proxy_resp->append_output_body_nocopy("<html>404 Not Found.</html>",
											  27);
	}
}

int main(int argc, char *argv[])
{
	unsigned short port;

	if (argc != 2)
	{
		fprintf(stderr, "USAGE: %s <port>\n", argv[0]);
		return 1;
	}

	port = atoi(argv[1]);

	struct WFServerParams params = HTTP_SERVER_PARAMS_DEFAULT;
	params.request_size_limit = 8 * 1024 * 1024;

	WFHttpServer server(&params, process);

	if (server.start(port) == 0)
	{
		printf("Server started on port %hu. Press Enter to exit.\n", port);
		getchar();
		server.stop();
	}
	else
	{
		perror("Cannot start server");
		return 1;
	}

	return 0;
}
