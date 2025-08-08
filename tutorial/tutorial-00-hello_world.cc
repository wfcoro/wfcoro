#include <cstdio>

#include "wfcoro/global.h"
#include "wfcoro/sleep.h"
#include "wfcoro/wait_group.h"

void say_hello(int id)
{
	// The sleep won't block the scheduler, because we are in coroutine now.
	wfcoro::sleep(1.0 * id);
	printf("Task %d say hello!\n", id);
}

int main()
{
	// Attach wfcoro to Workflow. Use one thread to show nonblock sleep.
	wfcoro::CoroConfig cfg;
	cfg.worker_threads = 1;
	wfcoro::global_init(cfg);

	wfcoro::WaitGroup wg(3);
	for (int i = 0; i < 3; i++)
	{
		wfcoro::schedule([=] {
			say_hello(i);
			wg.done();
		});
	}

	wg.wait();

	return 0;
}
