#pragma once

#include "event.h"

namespace team {

	void usleep(int ms) {
		event<> ev;
		uv::timer t(loop.uv, std::bind(&event<>::trigger, &ev), ms);
		ev.wait();
	}
	void sleep(int seconds) { usleep(seconds * 1000); }

}
