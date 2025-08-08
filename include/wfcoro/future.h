#ifndef WFCORO_FUTURE_H
#define WFCORO_FUTURE_H

#include <future>

#include "wfcoro/event.h"

namespace wfcoro
{

template<typename T>
class Promise;

template<typename T>
class Future
{
public:
	Future() = default;
	~Future() = default;

	Future(Future&&) = default;
	Future(const Future&) = delete;

	Future& operator=(Future&&) = default;
	Future& operator=(const Future&) = delete;

	bool valid() const
	{
		return fut.valid();
	}

	bool ready() const
	{
		return ev.is_signalled();
	}

	T get()
	{
		ev.wait();
		return fut.get();
	}

	void wait() const
	{
		ev.wait();
	}

	template<typename Rep, typename Period>
	bool wait_for(std::chrono::duration<Rep, Period>& duration) const
	{
		return ev.wait_for(duration);
	}

	template<typename Clock, typename Duration>
	bool wait_until(std::chrono::time_point<Clock, Duration>& timepoint) const
	{
		return ev.wait_until(timepoint);
	}

private:
	Future(std::future<T>&& fut, Event ev) : fut(std::move(fut)), ev(ev)
	{
	}

private:
	std::future<T> fut;
	Event ev;

	friend Promise<T>;
};

template<typename T>
class Promise
{
public:
	Promise() = default;
	~Promise() = default;

	Promise(Promise&&) = default;
	Promise(const Promise&) = delete;

	Promise& operator=(Promise&&) = default;
	Promise& operator=(const Promise&) = delete;

	Future<T> get_future()
	{
		return Future<T>(pro.get_future(), ev);
	}

	void set_value(const T& value)
	{
		pro.set_value(value);
		ev.signal();
	}

	void set_value(T&& value)
	{
		pro.set_value(std::move(value));
		ev.signal();
	}

	void set_exception(std::exception_ptr e)
	{
		pro.set_exception(std::move(e));
		ev.signal();
	}

private:
	std::promise<T> pro;
	Event ev;
};

template<>
class Promise<void>
{
public:
	Promise() = default;
	~Promise() = default;

	Promise(Promise&&) = default;
	Promise(const Promise&) = delete;

	Promise& operator=(Promise&&) = default;
	Promise& operator=(const Promise&) = delete;

	Future<void> get_future()
	{
		return Future<void>(pro.get_future(), ev);
	}

	void set_value()
	{
		pro.set_value();
		ev.signal();
	}

	void set_exception(std::exception_ptr e)
	{
		pro.set_exception(std::move(e));
		ev.signal();
	}

private:
	std::promise<void> pro;
	Event ev;
};

} // namespace wfcoro

#endif
