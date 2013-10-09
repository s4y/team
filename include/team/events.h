#pragma once

#include "event_loop.h"
#include <stdlib.h>

namespace team {

template<typename T, int(*Init)(uv_loop_t*, T *handle), typename CT = T>
class handle {
protected:

	// Thanks to Kerrek SB: http://stackoverflow.com/a/9779391/84745
	// Improvements that simplify this are welcome.
	template<typename Handle, typename C, C> struct cb_t;
	template<
		typename Handle, typename C, typename... Args,
		void (C::*CB)(Handle *, Args...)
	> struct cb_t<Handle, void (C::*)(Handle *, Args...), CB> {
		static void f(Handle *handle, Args ...args) {
			return (reinterpret_cast<C*>(handle->data)->*CB)(handle, args...);
		}
	};

	// ...especially welcome are ones that remove this macro
#define TEAM_UV_CALLBACK(Handle, memb_fn)\
	cb_t<Handle, decltype(&memb_fn), &memb_fn>::f

	T *m_handle;

	void bind(void *p) { m_handle->data = p; }

	handle(uv_loop_t *loop) : m_handle(new T) {
		if (int ret = Init(loop, m_handle)) {
			fprintf(stderr, "libuv handle init failed with %d\n", ret);
			exit(1);
		}
	}
	~handle() {
		uv_close((uv_handle_t*)m_handle, [](uv_handle_t *h){ delete h; });
	}
};

class timer : public context_t, public handle<uv_timer_t, uv_timer_init> {

	void cb(uv_timer_t *handle, int status) {
		m_loop->yield(this);
	}

	loop_t *m_loop;

public:
	timer(loop_t *loop) : handle(loop->uv), m_loop(loop) { bind(this); }

	void start(uint64_t msec) {
		uv_timer_start(
			m_handle, TEAM_UV_CALLBACK(uv_timer_t, timer::cb), msec, 0
		);
		this->yield(m_loop);
	}
};


}
