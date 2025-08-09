#ifndef WFCORO_WAIT_GROUP_H
#define WFCORO_WAIT_GROUP_H

#include "marl/waitgroup.h"

namespace wfcoro
{

class WaitGroup
{
public:
	WaitGroup(unsigned init_count = 0) : wg(init_count)
	{
	}

	void add(unsigned count = 1) const
	{
		wg.add(count);
	}

	bool done() const
	{
		return wg.done();
	}

	void wait() const
	{
		wg.wait();
	}

private:
	marl::WaitGroup wg;
};

} // namespace wfcoro

#endif
