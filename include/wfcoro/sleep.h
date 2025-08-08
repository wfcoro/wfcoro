#ifndef WFCORO_SLEEP_H
#define WFCORO_SLEEP_H

#include <chrono>

#include "marl/event.h"

namespace wfcoro
{

template<typename Rep, typename Period>
void sleep(const std::chrono::duration<Rep, Period>& duration)
{
	marl::Event ev;
	ev.wait_for(duration);
}

void sleep(double sec)
{
	sleep(std::chrono::duration<double>(sec));
}

} // namespace wfcoro

#endif
