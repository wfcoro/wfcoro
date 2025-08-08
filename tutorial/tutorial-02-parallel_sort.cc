#include <algorithm>
#include <chrono>
#include <cstdio>
#include <random>

#include "wfcoro/global.h"
#include "wfcoro/wait_group.h"

long current_ms()
{
	auto now = std::chrono::system_clock::now().time_since_epoch();
	return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

template<typename T>
void merge_sort_impl(T *begin, T *end, int depth)
{
	if (depth > 5 || end - begin < 10000)
	{
		std::sort(begin, end);
		return;
	}

	T *mid = begin + (end - begin) / 2;
	wfcoro::WaitGroup wg(2);

	wfcoro::schedule([=] {
		merge_sort_impl(begin, mid, depth + 1);
		wg.done();
	});

	wfcoro::schedule([=] {
		merge_sort_impl(mid, end, depth + 1);
		wg.done();
	});

	wg.wait();

	std::inplace_merge(begin, mid, end);
}

template<typename T>
void merge_sort(T *begin, T *end)
{
	merge_sort_impl(begin, end, 0);
}

void sort_integers(size_t count, bool parallel)
{
	unsigned *arr = new unsigned[count];

	std::mt19937 mt;
	std::generate(arr, arr + count, mt);

	long start, cost;

	start = current_ms();

	if (parallel)
		merge_sort(arr, arr + count);
	else
		std::sort(arr, arr + count);

	cost = current_ms() - start;

	bool sorted = std::is_sorted(arr, arr + count);
	printf("Sort %zu integers, sorted: %s, cost %ld ms\n", count,
		   sorted ? "true" : "false", cost);

	delete[] arr;
}

int main(int argc, char *argv[])
{
	if (argc != 2 && argc != 3)
	{
		fprintf(stderr, "USAGE: %s <count> [p]\n", argv[0]);
		return 1;
	}

	wfcoro::CoroConfig cfg;
	cfg.worker_threads = 4;
	wfcoro::global_init(cfg);

	size_t count;
	bool parallel = false;

	if (argc >= 2)
		count = atol(argv[1]);
	else
		count = 10000000;

	if (argc >= 3 && argv[2][0] == 'p')
		parallel = true;

	wfcoro::WaitGroup wg(1);
	wfcoro::schedule([=] {
		sort_integers(count, parallel);
		wg.done();
	});

	wg.wait();
	return 0;
}
