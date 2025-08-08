#ifndef WFCORO_EVENT_H
#define WFCORO_EVENT_H

#include "marl/event.h"

namespace wfcoro
{

class Event
{
public:
	Event() : ev(marl::Event::Mode::Manual)
	{
	}

	void signal() const
	{
		ev.signal();
	}

	bool is_signalled() const
	{
		return ev.isSignalled();
	}

	void clear() const
	{
		ev.clear();
	}

	void wait() const
	{
		ev.wait();
	}

	template<typename Rep, typename Period>
	bool wait_for(const std::chrono::duration<Rep, Period>& duration) const
	{
		return ev.wait_for(duration);
	}

	template<typename Clock, typename Duration>
	bool
	wait_until(const std::chrono::time_point<Clock, Duration>& timepoint) const
	{
		return ev.wait_until(timepoint);
	}

private:
	marl::Event ev;
};

} // namespace wfcoro

#endif
