#pragma once

#include "event_loop.h"
#include <stdlib.h>

namespace async {

template<typename T, int(*Init)(uv_loop_t*, T *handle), typename CT = T>
class handle {
protected:
	static void _cb(CT *handle, int status) {
		reinterpret_cast<class handle*>(handle->data)->cb(handle, status);
	}
	virtual void cb(CT *handle, int status) = 0;

	T m_handle;

	handle(uv_loop_t *loop) {
		int ret;
		if ((ret = Init(loop, &m_handle))) {
			fprintf(stderr, "libuv handle init failed with %d\n", ret);
			exit(1);
		}
		m_handle.data = this;
	}
};

class timer : public context_t, public handle<uv_timer_t, uv_timer_init> {

	virtual void cb(uv_timer_t *handle, int status) override {
		m_loop->yield(this);
	}

	loop_t *m_loop;

public:
	timer(loop_t *loop) : handle(loop->uv), m_loop(loop) {}
	void start(uint64_t msec) {
		uv_timer_start(&m_handle, _cb, msec, 0);
		this->yield(m_loop);
	}
};

}
