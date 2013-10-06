#pragma once

#include "environment.h"

struct loop_t : public env_t {

	uv_loop_t *uv;
	bool running;

	loop_t(uv_loop_t *_uv) : uv(_uv), running(false) {}

	void run() {
		running = true;
		spawn([&] {
			yield_real(this);
			uv_run(uv, UV_RUN_DEFAULT);
		});
	}

	operator ucontext_t *() override {
		if (!running) run();
		return env_t::operator ucontext_t *();
	}
};
