#pragma once

namespace team {

	class fence {
		context *ctx;
		public:
		fence(fence &&) = default;
		fence() = default;
		~fence() { loop.blockOnce(ctx); }
		void wait() { (ctx = current_context)->yield(loop); }
	};

	class fence_queue {
		std::queue<std::unique_ptr<fence>> q;

		public:
		void wait() {
			q.emplace(new fence); q.back()->wait();
		}
		void maybe_pop() {
			if (q.empty()) return;
			// The fence will be destroyed *after* it's been removed from the
			// queue. Else, we'll yield to the fence inside queues.pop() and it
			// may destroy us and reenter the fence's destructor
			auto fence = std::move(q.front());
			q.pop();
		}

		operator bool() { return !q.empty(); }
	};

}
