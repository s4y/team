#pragma once

#include "context.h"
#include <queue>
#include <stack>

namespace team {

	class env {

		std::queue<team::context *> tasks;
		std::stack<team::context *> returns;

	protected:

		context *scheduler_ctx;

	public:

		env() : scheduler_ctx(nullptr) {}

		virtual void schedule(context *ctx) {
			assert(ctx && ctx != scheduler_ctx);
			tasks.push(ctx);
		}

		void blockOnce(context *ctx) {
			assert(ctx);
			returns.push(current_context);
			current_context->yield(ctx);
			returns.pop();
		}

		coroutine *spawn(std::function<void()> &&f, coroutine::delegate *r = nullptr) {
			auto new_coroutine = new coroutine(
				std::forward<std::function<void()>>(f), r
			);
			blockOnce(new_coroutine);
			return new_coroutine;
		}

		[[noreturn]] virtual void main() {
			while (!tasks.empty()) {
				auto t = std::move(tasks.front());
				tasks.pop();
				current_context->yield(t);
			}
			exit(0);
		}

		operator context *() {
			if (!returns.empty()) {
				return returns.top();
			}
			if (!scheduler_ctx) {
				scheduler_ctx = new coroutine([&] { main(); });
			}
			return scheduler_ctx;
		}

	};

}
