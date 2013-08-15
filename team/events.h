#pragma once

#include "event_loop.h"
#include <stdlib.h>

namespace async {

template<typename T, int(*Init)(uv_loop_t*, T *handle), typename CT = T>
class handle {
protected:

	// template<typename C, typename CB> struct cb_t;
	// template<typename C, void C::*CB(CT *,typename Args &&...)>
	// struct cb_t {
	// 	static void f(CT *handle, Args &&...args) {
	// 		reinterpret_cast<C*>(handle->data)->*CB(std::forward<Args>(args)...);
	// 	}
	// };
	// template<typename CB, typename C, typename... Args>
	// static void _cb(CT *handle, Args&& ...args);

	// //template <typename C, typename... Args, void (C::*CB)(CT *, Args...)>
	//template <typename C, typename CB, typename... Args>
	static void _cb(CT *handle, int status) {
		reinterpret_cast<class handle*>(handle->data)->cb(handle, status);
	}

	virtual void cb(CT *handle, int status) = 0;

	T *m_handle;

	handle(uv_loop_t *loop) : m_handle(new T) {
		if (int ret = Init(loop, m_handle)) {
			fprintf(stderr, "libuv handle init failed with %d\n", ret);
			exit(1);
		}
		m_handle->data = this;
	}
	~handle() { uv_close((uv_handle_t*)m_handle, [](uv_handle_t *h){ delete h; }); }
};

class timer : public context_t, public handle<uv_timer_t, uv_timer_init> {

	virtual void cb(uv_timer_t *handle, int status) override {
		m_loop->yield(this);
	}

	loop_t *m_loop;

public:
	timer(loop_t *loop) : handle(loop->uv), m_loop(loop) {}
	void start(uint64_t msec) {
		uv_timer_start(m_handle, _cb, msec, 0);
		this->yield(m_loop);
	}
};

}
