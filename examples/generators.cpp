#include <stdio.h>
#include <team/async.h>

void amain() {

	auto fib = generator [](std::function<void(unsigned int)> yield) {
		unsigned int a, b;
		yield((a = 0));
		yield((b = 1));
		for(;;) {
			std::tie(a, b) = std::make_tuple(b, a + b);
			yield(b);
		}
	};

	for (size_t i = 0; i < 13; i++) {
		printf("%d\n", fib());
	}
}
