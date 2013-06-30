#pragma once

#include "events.h"

namespace A {
	loop_t loop(uv_default_loop());

	struct await_t {
		bool done;
		rendezvous_t r;

		await_t() : done(false), r(&async::loop) {}
		operator rendezvous_t *() { return &r; }
	};
}
void asleep(int seconds) { timer_t(&A::loop).start(seconds * 1000); }

rendezvous_t * const __r = nullptr;

#define A(stmt) A::loop.spawn([&](){ stmt; }, __r)
// #define await rendezvous_t _r(&async::loop); auto *__r = &_r;
#define await for (async::await_t __r; !__r.done; __r.done = true)

void amain();

int main() {
	A(amain());
	A::loop.run();
}

