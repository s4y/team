#pragma once

#include "events.h"

namespace async {
	loop_t loop(uv_default_loop());

	struct await_t {
		bool done;
		rendezvous_t r;

		await_t() : done(false), r(&async::loop) {}
		operator rendezvous_t *() { return &r; }
	};

	struct spawn_t {
		rendezvous_t *r;
		spawn_t(rendezvous_t *_r) : r(_r) {}
		void operator <<(std::function<void()> &&f) {
			loop.spawn(std::forward<std::function<void()>>(f), r); }
	};

	// High-level async constructs

	class fence {
		context_t ctx;
		public:
		~fence() { loop.blockOnce(&ctx); }
		void wait() { ctx.yield(&loop); }
	};

	template <typename T>
	class channel {

		std::queue<T> values;
		size_t max_size;

		std::queue<fence> senders;
		std::queue<fence> receivers;

		public:
		channel() : max_size(0) {}
		channel(size_t _max_size) : max_size(_max_size) {}

		template <typename Val>
		void send(Val &&v) {
			while (max_size && values.size() == max_size) {
				senders.emplace();
				senders.back().wait();
			}

			values.push(std::forward<Val>(v));
			if (!receivers.empty()) { receivers.pop(); }
		}

		T recv() {
			while (!values.size()) {
				receivers.emplace();
				receivers.back().wait();
			}
			auto ret = values.front();
			values.pop();
			if (!senders.empty()) { senders.pop(); }
			return ret;
		}
	};

}
void asleep(int seconds) { timer_t(&async::loop).start(seconds * 1000); }

rendezvous_t * const __r = nullptr;

// #define A(stmt) async::loop.spawn([&](){ stmt; }, __r)
#define A async::spawn_t(__r) << [&]()
// #define await rendezvous_t _r(&async::loop); auto *__r = &_r;
#define await for (async::await_t __r; !__r.done; __r.done = true)

void amain();

int main() {
	A{ amain(); };
	async::loop.run();
}

