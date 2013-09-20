#include <stdio.h>
#include <team/async.h>

void worker(async::channel<int> &ch, const char *name) {
	for (;;) {
		int i = ch.recv();
		printf("%s working on %d\n", name, i);
		async::sleep(1);
	}
}

void amain() {

	// Team's channels work a bit like channels in Go and Clojure: you can
	// send() values to them or recv() from them. recv() blocks if the
	// channel is empty. send() blocks if the channel has a maximum size
	// (here, 3) and is full.
	async::channel<int> ch(3);

	await {

		// Since Team just uses cooperative multitasking more workers won't be
		// faster. This just demonstrates having more than one listener on a
		// channel at a time.
		A [&]{ worker(ch, "Steve"); };
		A [&]{ worker(ch, "Fred"); };

		for (int i = 10; i--;) {
			printf("Sending %d\n", i);
			ch.send(i);
		}

	}

}
