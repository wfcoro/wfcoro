#include <cstdio>
#include <string>
#include <vector>

#include "wfcoro/global.h"
#include "wfcoro/redis_client.h"

bool show_redis_result(const wfcoro::RedisResult& res)
{
	if (res.state != WFT_STATE_SUCCESS)
	{
		const char *err = WFGlobal::get_error_string(res.state, res.error);
		fprintf(stderr, "Error: %s\n", err);
		return false;
	}

	protocol::RedisValue value;
	res.resp.get_result(value);

	std::string str = value.debug_string();
	fprintf(stdout, "Redis response: %s\n", str.c_str());

	return true;
}

void use_redis_client(wfcoro::RedisClient& cli, const std::string& key,
					  const std::string& value)
{
	wfcoro::RedisResult res;

	// SET key value
	res = cli.execute("SET", {key, value});
	if (!show_redis_result(res))
		return;

	// GET key
	res = cli.execute("GET", {key});
	if (!show_redis_result(res))
		return;

	// DEL key
	res = cli.execute("DEL", {key});
	if (!show_redis_result(res))
		return;

	printf("Redis operations completed.\n");
}

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		fprintf(stderr, "USAGE: %s redis_url key value\n", argv[0]);
		return 1;
	}

	wfcoro::CoroConfig cfg;
	cfg.worker_threads = 4;
	wfcoro::global_init(cfg);

	std::string url, key, value;

	url.assign(argv[1]);
	if (strncasecmp(argv[1], "redis://", 8) != 0 &&
		strncasecmp(argv[1], "rediss://", 9) != 0)
	{
		url = "redis://" + url;
	}

	key.assign(argv[2]);
	value.assign(argv[3]);

	wfcoro::RedisClientParams params;
	params.url = url;
	params.retry_max = 1;
	params.receive_timeout = 1000;

	wfcoro::RedisClient cli(params);
	use_redis_client(cli, key, value);

	return 0;
}
