#pragma once

#include "handle.h"

namespace team {
	namespace uv {
		
		namespace details {
			template <>
			void init<uv_timer_t>(uv_loop_t *loop, uv_timer_t *handle) {
				uv_timer_init(loop, handle);
			}
		}

		class timer : public handle<uv_timer_t> {

			std::function<void()> f;

			void cb(uv_timer_t *handle, int status) { f(); }

		public:
			timer(uv_loop_t *l, std::function<void()> &&_f, uint64_t ms) : handle(l), f(std::move(_f)) {
				uv_timer_start(m_handle, TEAM_UV_CALLBACK(uv_timer_t, timer::cb), ms, 0);
			}
		};

	}
}
