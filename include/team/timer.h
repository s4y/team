#pragma once

#include "handle.h"

namespace team {

class timer : public context_t, public handle<uv_timer_t> {

	void cb(uv_timer_t *handle, int status) {
		loop.yield(this);
	}

public:
	timer() { init<uv_timer_init>(loop.uv, this); }

	void start(uint64_t msec) {
		uv_timer_start(
			m_handle, TEAM_UV_CALLBACK(uv_timer_t, timer::cb), msec, 0
		);
		this->yield(&loop);
	}
};

void usleep(int ms) { timer().start(ms); }
void sleep(int seconds) { usleep(seconds * 1000); }

}
