#pragma once

#include "environment.h"
#include <stdlib.h>

namespace team {

struct loop_t : public env_t {

	uv_loop_t *uv;
	bool running;

	loop_t(uv_loop_t *_uv) : uv(_uv), running(false) {}

	void run() {
		running = true;
		spawn([&] {
			yield_real(this);
			uv_run(uv, UV_RUN_DEFAULT);
			exit(0);
		});
	}

	operator ucontext_t *() override {
		if (!running) run();
		return env_t::operator ucontext_t *();
	}
};

loop_t loop(uv_default_loop());

// Yield to the event loop forever, allowing your own context to be lost in
// the sands of time. You can peaceOut() from main() to keep running until
// the event loop is clear (all timers fired their last, sockets closed,
// etc.) at which point you'll exit(0). Calling peaceOut() from an async
// task is a leak.
[[noreturn]] void peaceOut() { loop.load(); }

}
