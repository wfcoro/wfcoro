#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "wfcoro/global.h"
#include "wfcoro/mysql_client.h"
#include "wfcoro/wait_group.h"

using namespace protocol;

volatile bool stop_flag = false;

bool get_next_cmd(std::string& cmd)
{
	int len;
	char query[4096];

	fprintf(stderr, "mysql> ");
	while ((fgets(query, 4096, stdin)) && stop_flag == false)
	{
		len = strlen(query);
		if (len > 0 && query[len - 1] == '\n')
			query[len - 1] = '\0';

		if (strncmp(query, "quit", len) == 0 ||
			strncmp(query, "exit", len) == 0)
		{
			fprintf(stderr, "Bye\n");
			return false;
		}

		if (len == 0 || strncmp(query, "\0", len) == 0)
		{
			fprintf(stderr, "mysql> ");
			continue;
		}

		cmd = query;
		return true;
	}

	return false;
}

bool show_mysql_result(wfcoro::MySQLResult& result)
{
	MySQLResponse& resp = result.resp;
	MySQLResultCursor cursor(&resp);
	const MySQLField *const *fields;
	std::vector<MySQLCell> arr;

	if (result.state != WFT_STATE_SUCCESS)
	{
		fprintf(stderr, "error msg: %s\n",
				WFGlobal::get_error_string(result.state, result.error));
		return false;
	}

	do
	{
		if (cursor.get_cursor_status() != MYSQL_STATUS_GET_RESULT &&
			cursor.get_cursor_status() != MYSQL_STATUS_OK)
		{
			break;
		}

		fprintf(stderr, "---------------- RESULT SET ----------------\n");

		if (cursor.get_cursor_status() == MYSQL_STATUS_GET_RESULT)
		{
			fprintf(stderr, "cursor_status=%d field_count=%u rows_count=%u\n",
					cursor.get_cursor_status(), cursor.get_field_count(),
					cursor.get_rows_count());

			// nocopy api
			fields = cursor.fetch_fields();
			for (int i = 0; i < cursor.get_field_count(); i++)
			{
				if (i == 0)
				{
					fprintf(stderr, "db=%s table=%s\n",
							fields[i]->get_db().c_str(),
							fields[i]->get_table().c_str());
					fprintf(stderr, "  ---------- COLUMNS ----------\n");
				}
				fprintf(stderr, "  name[%s] type[%s]\n",
						fields[i]->get_name().c_str(),
						datatype2str(fields[i]->get_data_type()));
			}
			fprintf(stderr, "  _________ COLUMNS END _________\n\n");

			while (cursor.fetch_row(arr))
			{
				fprintf(stderr, "  ------------ ROW ------------\n");
				for (size_t i = 0; i < arr.size(); i++)
				{
					fprintf(stderr, "  [%s][%s]", fields[i]->get_name().c_str(),
							datatype2str(arr[i].get_data_type()));
					if (arr[i].is_string())
					{
						std::string res = arr[i].as_string();
						if (res.length() == 0)
							fprintf(stderr, "[\"\"]\n");
						else
							fprintf(stderr, "[%s]\n", res.c_str());
					}
					else if (arr[i].is_int())
					{
						fprintf(stderr, "[%d]\n", arr[i].as_int());
					}
					else if (arr[i].is_ulonglong())
					{
						fprintf(stderr, "[%llu]\n", arr[i].as_ulonglong());
					}
					else if (arr[i].is_float())
					{
						const void *ptr;
						size_t len;
						int data_type;
						arr[i].get_cell_nocopy(&ptr, &len, &data_type);
						size_t pos;
						for (pos = 0; pos < len; pos++)
							if (*((const char *)ptr + pos) == '.')
								break;
						if (pos != len)
							pos = len - pos - 1;
						else
							pos = 0;
						fprintf(stderr, "[%.*f]\n", (int)pos,
								arr[i].as_float());
					}
					else if (arr[i].is_double())
					{
						const void *ptr;
						size_t len;
						int data_type;
						arr[i].get_cell_nocopy(&ptr, &len, &data_type);
						size_t pos;
						for (pos = 0; pos < len; pos++)
							if (*((const char *)ptr + pos) == '.')
								break;
						if (pos != len)
							pos = len - pos - 1;
						else
							pos = 0;
						fprintf(stderr, "[%.*lf]\n", (int)pos,
								arr[i].as_double());
					}
					else if (arr[i].is_date())
					{
						fprintf(stderr, "[%s]\n", arr[i].as_string().c_str());
					}
					else if (arr[i].is_time())
					{
						fprintf(stderr, "[%s]\n", arr[i].as_string().c_str());
					}
					else if (arr[i].is_datetime())
					{
						fprintf(stderr, "[%s]\n", arr[i].as_string().c_str());
					}
					else if (arr[i].is_null())
					{
						fprintf(stderr, "[NULL]\n");
					}
					else
					{
						std::string res = arr[i].as_binary_string();
						if (res.length() == 0)
							fprintf(stderr, "[\"\"]\n");
						else
							fprintf(stderr, "[%s]\n", res.c_str());
					}
				}
				fprintf(stderr, "  __________ ROW END __________\n");
			}
		}
		else if (cursor.get_cursor_status() == MYSQL_STATUS_OK)
		{
			fprintf(stderr, "  OK. %llu ", cursor.get_affected_rows());
			if (cursor.get_affected_rows() == 1)
				fprintf(stderr, "row ");
			else
				fprintf(stderr, "rows ");
			fprintf(stderr, "affected. %d warnings. insert_id=%llu. %s\n",
					cursor.get_warnings(), cursor.get_insert_id(),
					cursor.get_info().c_str());
		}

		fprintf(stderr, "________________ RESULT SET END ________________\n\n");
	}
	while (cursor.next_result_set());

	if (resp.get_packet_type() == MYSQL_PACKET_ERROR)
	{
		fprintf(stderr, "ERROR. error_code=%d %s\n", resp.get_error_code(),
				resp.get_error_msg().c_str());
	}
	else if (resp.get_packet_type() == MYSQL_PACKET_OK)
	{
		fprintf(stderr, "OK. %llu ", resp.get_affected_rows());
		if (resp.get_affected_rows() == 1)
			fprintf(stderr, "row ");
		else
			fprintf(stderr, "rows ");
		fprintf(stderr, "affected. %d warnings. insert_id=%llu. %s\n",
				resp.get_warnings(), resp.get_last_insert_id(),
				resp.get_info().c_str());
	}

	return true;
}

void use_mysql_client(wfcoro::MySQLClient& cli)
{
	wfcoro::MySQLResult res;

	std::string cmd;
	while (get_next_cmd(cmd))
	{
		res = cli.query(cmd);

		if (!show_mysql_result(res))
			break;
	}
}

void sighandler(int signo)
{
	stop_flag = true;
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr,
				"USAGE: %s <url>\n"
				"      url format: "
				"mysql://root:password@host:port/dbname?character_set=charset\n"
				"      example: mysql://root@test.mysql.com/test\n",
				argv[0]);
		return 0;
	}

	wfcoro::CoroConfig cfg;
	cfg.worker_threads = 4;
	wfcoro::global_init(cfg);

	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);

	wfcoro::MySQLClientParams params;
	params.url = argv[1];

	if (strncasecmp(argv[1], "mysql://", 8) != 0 &&
		strncasecmp(argv[1], "mysqls://", 9) != 0)
	{
		params.url = "mysql://" + params.url;
	}

	wfcoro::MySQLClient cli(params);

	wfcoro::WaitGroup wg(1);
	wfcoro::schedule([&cli, wg] {
		use_mysql_client(cli);
		wg.done();
	});

	wg.wait();
	return 0;
}
