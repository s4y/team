#pragma once

#include <stdlib.h>
#include <uv/uv.h>

namespace team {

namespace uv { namespace details {
	template<typename T, typename ...Args>
	void init(uv_loop_t *, T *, Args ...);
} }

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

	template <typename ...Args>
	handle(uv_loop_t *loop, Args &&...args) : m_handle(new T) {
		uv::details::init(loop, m_handle, std::forward<Args>(args)...);
		m_handle->data = this;
	}

	~handle() {
		uv_close((uv_handle_t*)m_handle, [](uv_handle_t *h){ delete h; });
	}
};

}
