#ifndef WFCORO_NETWORK_H
#define WFCORO_NETWORK_H

#include "wfcoro/event.h"
#include "workflow/WFTask.h"

namespace wfcoro
{

template<typename Resp>
struct NetworkResult
{
	int state;
	int error;
	Resp resp;
};

template<typename Req, typename Resp>
NetworkResult<Resp> wait_network_task(WFNetworkTask<Req, Resp> *task)
{
	NetworkResult<Resp> res;
	Event ev;

	task->set_callback([&res, ev](WFNetworkTask<Req, Resp> *task) {
		res.state = task->get_state();
		res.error = task->get_error();
		if (res.state == WFT_STATE_SUCCESS)
			res.resp = std::move(*task->get_resp());

		ev.signal();
	});

	task->start();
	ev.wait();

	return res;
}

} // namespace wfcoro

#endif
