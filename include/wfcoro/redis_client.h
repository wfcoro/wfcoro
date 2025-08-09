#ifndef WFCORO_REDIS_CLIENT_H
#define WFCORO_REDIS_CLIENT_H

#include <string>
#include <vector>

#include "wfcoro/network.h"
#include "workflow/WFTaskFactory.h"

namespace wfcoro
{

struct RedisClientParams
{
	int retry_max = 0;
	int send_timeout = -1;
	int receive_timeout = -1;
	int keep_alive_timeout = -1;

	std::string url;
};

using RedisResult = NetworkResult<protocol::RedisResponse>;

class RedisClient
{
public:
	RedisClient(const RedisClientParams& params) : params(params)
	{
	}

	RedisResult execute(const std::string& cmd,
						const std::vector<std::string>& args)
	{
		WFRedisTask *task;
		task = WFTaskFactory::create_redis_task(params.url, params.retry_max,
												nullptr);

		task->set_send_timeout(params.send_timeout);
		task->set_receive_timeout(params.receive_timeout);
		task->set_keep_alive(params.keep_alive_timeout);
		task->get_req()->set_request(cmd, args);

		return wait_network_task(task);
	}

private:
	RedisClientParams params;
};

} // namespace wfcoro

#endif
