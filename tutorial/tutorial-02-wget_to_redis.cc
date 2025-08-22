#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "wfcoro/event.h"
#include "wfcoro/global.h"
#include "workflow/WFTaskFactory.h"

using namespace protocol;

void handle_response(std::string http_url, std::string redis_url,
					 WFHttpTask *task)
{
	HttpResponse *resp = task->get_resp();
	int state = task->get_state();
	int error = task->get_error();

	if (state != WFT_STATE_SUCCESS)
	{
		fprintf(stderr, "Http task error: state = %d, error = %d\n", state,
				error);
		return;
	}

	const void *body;
	size_t body_len;

	resp->get_parsed_body(&body, &body_len);
	if (body_len == 0)
	{
		fprintf(stderr, "Empty http body!");
		return;
	}

	wfcoro::Event redis_ev;
	int redis_state;
	int redis_error;
	protocol::RedisValue value;

	auto callback = [&, redis_ev](WFRedisTask *task) {
		redis_state = task->get_state();
		redis_error = task->get_error();

		if (redis_state == WFT_STATE_SUCCESS)
			task->get_resp()->get_result(value);

		redis_ev.signal();
	};

	std::string data((const char *)body, body_len);
	WFRedisTask *redis_task;
	redis_task = WFTaskFactory::create_redis_task(redis_url, 0, callback);
	redis_task->get_req()->set_request("SET", {http_url, data});
	redis_task->start();

	// When using wfcoro, the callback function of the network task runs in the
	// coroutine, and waiting with wfcoro::Event will not block the thread.
	redis_ev.wait();

	if (redis_state == WFT_STATE_SUCCESS)
	{
		if (value.is_error())
		{
			fprintf(stderr, "Redis error %s\n", value.string_view()->c_str());
		}
		else
		{
			fprintf(stderr, "Redis set success key = %s value size = %zu\n",
					http_url.c_str(), body_len);
		}
	}
	else
	{
		fprintf(stderr, "Redis set error: state = %d, error = %d\n",
				redis_state, redis_error);
	}
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		fprintf(stderr, "USAGE: %s <http URL> <redis URL>\n", argv[0]);
		return 1;
	}

	// Attach wfcoro to Workflow with default config.
	wfcoro::global_init(wfcoro::CoroConfig{});

	std::string http_url, redis_url;

	http_url = argv[1];
	if (strncasecmp(argv[1], "http://", 7) != 0 &&
		strncasecmp(argv[1], "https://", 8) != 0)
	{
		http_url = "http://" + http_url;
	}

	redis_url = argv[2];
	if (strncasecmp(argv[2], "redis://", 8) != 0 &&
		strncasecmp(argv[2], "rediss://", 9) != 0)
	{
		redis_url = "redis://" + redis_url;
	}

	WFHttpTask *http_task;
	wfcoro::Event ev;

	auto callback = [=](WFHttpTask *task) {
		handle_response(http_url, redis_url, task);
		ev.signal();
	};

	http_task = WFTaskFactory::create_http_task(http_url, 2, 0, callback);

	HttpRequest *req = http_task->get_req();
	req->add_header_pair("Accept", "*/*");
	req->add_header_pair("User-Agent", "Wget/1.14 (linux-gnu)");
	req->add_header_pair("Connection", "close");

	/* Limit the http response size to 20M. */
	http_task->get_resp()->set_size_limit(20 * 1024 * 1024);

	/* no more than 30 seconds receiving http response. */
	http_task->set_receive_timeout(30 * 1000);

	http_task->start();
	ev.wait();

	return 0;
}
