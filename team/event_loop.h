#pragma once

#include "environment.h"

struct loop_t : public env_t {

	uv_loop_t *uv;

	loop_t(uv_loop_t *_uv) : uv(_uv) {}

	void run() {
		uv_run(uv, UV_RUN_DEFAULT);
	}

};
