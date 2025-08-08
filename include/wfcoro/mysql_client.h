#ifndef WFCORO_MYSQL_CLIENT_H
#define WFCORO_MYSQL_CLIENT_H

#include <string>

#include "wfcoro/network.h"
#include "workflow/MySQLResult.h"
#include "workflow/WFMySQLConnection.h"
#include "workflow/WFTaskFactory.h"

namespace wfcoro
{

struct MySQLClientParams
{
	int retry_max = 0;
	int send_timeout = -1;
	int receive_timeout = -1;
	int keep_alive_timeout = -1;

	/* example: mysql://username:passwd@127.0.0.1/dbname?character_set=utf8
	 * IP string is recommmended in url. When using a domain name, the first
	 * address resovled will be used. Don't use upstream name as a host. */
	std::string url;
};

using MySQLResult = NetworkResult<protocol::MySQLResponse>;

class MySQLClient
{
public:
	MySQLClient(const MySQLClientParams& params) : params(params)
	{
	}

	MySQLResult query(const std::string& query)
	{
		WFMySQLTask *task;
		task = WFTaskFactory::create_mysql_task(params.url, params.retry_max,
												nullptr);

		task->set_send_timeout(params.send_timeout);
		task->set_receive_timeout(params.receive_timeout);
		task->set_keep_alive(params.keep_alive_timeout);
		task->get_req()->set_query(query);

		return wait_network_task(task);
	}

private:
	MySQLClientParams params;
};

class MySQLConnection
{
public:
	MySQLConnection(const MySQLClientParams& params, int unique_id)
		: params(params), conn(unique_id)
	{
		conn.init(params.url);
	}

	MySQLResult query(const std::string& query)
	{
		WFMySQLTask *task = conn.create_query_task(query, nullptr);

		task->set_send_timeout(params.send_timeout);
		task->set_receive_timeout(params.receive_timeout);
		task->set_keep_alive(params.keep_alive_timeout);

		return wait_network_task(task);
	}

	/* If you don't disconnect manually, the TCP connection will be
	 * kept alive after this object is deleted, and maybe reused by
	 * another WFMySQLConnection object with same id and url. */
	MySQLResult disconnect()
	{
		WFMySQLTask *task = conn.create_disconnect_task(nullptr);
		return wait_network_task(task);
	}

private:
	MySQLClientParams params;
	WFMySQLConnection conn;
};

} // namespace wfcoro

#endif
