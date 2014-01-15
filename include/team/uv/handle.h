#pragma once

#include <stdlib.h>

namespace team {

template<typename T, typename CT = T>
struct handle {

	// Thanks to Kerrek SB: http://stackoverflow.com/a/9779391/84745
	// Improvements that simplify this are welcome.
	template<typename Handle, typename C, C> struct cb_t;
	template<
		typename Handle, typename C, typename... Args,
		void (C::*CB)(Handle *, Args...)
	> struct cb_t<Handle, void (C::*)(Handle *, Args...), CB> {
		static void f(Handle *handle, Args ...args) {
			return (static_cast<C*>(handle->data)->*CB)(handle, args...);
		}
	};

	// ...especially welcome are ones that remove this macro
#define TEAM_UV_CALLBACK(Handle, memb_fn)\
	cb_t<Handle, decltype(&memb_fn), &memb_fn>::f

	T *m_handle;

	void init(void *p) { m_handle->data = p; }

	template <int(*Init)(uv_loop_t*, T *handle)>
	void init(uv_loop_t *loop, void *p) {
		if (int ret = Init(loop, m_handle)) {
			fprintf(stderr, "libuv handle init failed with %d\n", ret);
			exit(1);
		}
		init(p);
	}

	handle() : m_handle(new T) {}

	~handle() {
		uv_close((uv_handle_t*)m_handle, [](uv_handle_t *h){ delete h; });
	}
};

}
