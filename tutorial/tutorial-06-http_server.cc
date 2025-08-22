#include <stdio.h>

#include "wfcoro/global.h"
#include "wfcoro/sleep.h"
#include "workflow/WFHttpServer.h"

void http_process(WFHttpTask *task)
{
	// Server process is also in coroutine, so we can use nonblock sleep here.
	wfcoro::sleep(1.0);

	protocol::HttpResponse *resp = task->get_resp();
	resp->append_output_body("<html>Hello World After 1 Second!</html>\n");
}

int main()
{
	// Attach wfcoro to Workflow with default config.
	wfcoro::global_init(wfcoro::CoroConfig{});

	WFHttpServer server(http_process);
	if (server.start(8888) == 0)
	{
		printf("Server started on port 8888. Press Enter to exit.\n");
		getchar();
		server.stop();
	}

	return 0;
}
