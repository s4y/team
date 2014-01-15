#pragma once

#include "environment.h"
#include "uv/timer.h"

namespace team {

	struct loop_t : public env {

		uv_loop_t *uv;

	public:

		struct task {
			uv::timer t;

			task(uv_loop_t *uv, context *ctx) : t(uv, [ctx, this]{
				current_context->yield(ctx);
				delete this;
			}, 0) {}
		};

		loop_t(uv_loop_t *_uv) : uv(_uv) { }

		virtual void schedule(context *ctx) {
			assert(ctx && ctx != scheduler_ctx);
			new task(uv, ctx);
		}

		[[noreturn]] virtual void main() override {
			uv_run(uv, UV_RUN_DEFAULT);
			exit(0);
		}

	};

	loop_t loop(uv_default_loop());
	context *get_default_context() { return loop; }

	// Yield to the event loop forever, allowing your own context to be lost in
	// the sands of time. You can peaceOut() from main() to keep running until
	// the event loop is clear (all timers fired their last, sockets closed,
	// etc.) at which point it'll exit(0). Calling peaceOut() from an async
	// task is a leak.
	[[noreturn]] void peaceOut() { current_context->yield(loop); abort(); }

}
