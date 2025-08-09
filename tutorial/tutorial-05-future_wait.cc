#include <chrono>
#include <cstdio>

#include "wfcoro/future.h"
#include "wfcoro/global.h"
#include "wfcoro/sleep.h"
#include "wfcoro/wait_group.h"

void future_wait()
{
	wfcoro::Promise<int> promise;
	wfcoro::Future<int> future = promise.get_future();

	// Promise is not copiable.
	// If >= C++14, use `[promise = std::move(promise)]` instead of reference.
	wfcoro::schedule([&promise] {
		std::chrono::seconds sec(1);
		wfcoro::sleep(sec);
		promise.set_value(42);
	});

	std::chrono::milliseconds ms(100);
	while (!future.ready())
	{
		printf(".");
		fflush(stdout);

		future.wait_for(ms);
	}

	printf("\nFuture is ready, value: %d\n", future.get());
}

int main()
{
	wfcoro::CoroConfig cfg;
	cfg.worker_threads = 1;
	wfcoro::global_init(cfg);

	wfcoro::WaitGroup wg(1);
	wfcoro::schedule([wg] {
		future_wait();
		wg.done();
	});

	wg.wait();
	return 0;
}
