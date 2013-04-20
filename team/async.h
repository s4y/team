#pragma once

#include "events.h"

namespace A {
	loop_t loop(uv_default_loop());
}
void asleep(int seconds) { timer_t(&A::loop).start(seconds * 1000); }

rendezvous_t * const __r = nullptr;

#define A(stmt) A::loop.spawn([&](){ stmt; }, __r)
#define await rendezvous_t _r(&A::loop); auto *__r = &_r;

void amain();

int main() {
	A(amain());
	A::loop.run();
}

