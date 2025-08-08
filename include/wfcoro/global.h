#ifndef WFCORO_GLOBAL_H
#define WFCORO_GLOBAL_H

#include "marl/scheduler.h"
#include "marl/waitgroup.h"
#include "workflow/WFGlobal.h"

namespace wfcoro
{

struct CoroConfig
{
	int worker_threads = 20;
	size_t stack_size = 1024 * 1024;
};

class EventHandler : public CommEventHandler
{
private:
	static EventHandler *event_handler;

public:
	static void init(const CoroConfig& cfg)
	{
		if (!event_handler)
		{
			marl::Scheduler::Config marl_cfg;
			marl_cfg.setWorkerThreadCount(cfg.worker_threads);
			marl_cfg.setFiberStackSize(cfg.stack_size);

			event_handler = new EventHandler(marl_cfg);
			WFGlobal::get_scheduler()->customize_event_handler(event_handler);
		}
	}

	static EventHandler *get_event_handler()
	{
		return event_handler;
	}

	template<typename Func>
	static void schedule(Func&& func)
	{
		auto *sched = &event_handler->sched;
		sched->enqueue(marl::Task(std::forward<Func>(func)));
	}

private:
	EventHandler(marl::Scheduler::Config cfg) : sched(cfg)
	{
	}

	void schedule(void (*routine)(void *), void *context) override
	{
		wg.add();

		sched.enqueue(marl::Task([this, routine, context] {
			routine(context);
			wg.done();
		}));
	}

	void wait() override
	{
		wg.wait();
		delete this;
	}

private:
	marl::WaitGroup wg;
	marl::Scheduler sched;
};

inline void global_init(const CoroConfig& cfg)
{
	EventHandler::init(cfg);
}

template<typename Function>
void schedule(Function&& func)
{
	EventHandler::schedule(std::forward<Function>(func));
}

} // namespace wfcoro

#endif
