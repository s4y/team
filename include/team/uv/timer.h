#pragma once

#include "handle.h"

namespace team {
	namespace uv {

		class timer : public handle<uv_timer_t> {

			std::function<void()> f;

			void cb(uv_timer_t *handle, int status) { f(); }

		public:
			timer(uv_loop_t *l, std::function<void()> &&_f, uint64_t ms) : f(std::move(_f)) {
				init<uv_timer_init>(l, this);
				uv_timer_start(m_handle, TEAM_UV_CALLBACK(uv_timer_t, timer::cb), ms, 0);
			}
		};

	}
}
