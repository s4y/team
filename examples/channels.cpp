#include <stdio.h>
#include <team/async.h>

void worker(async::channel<int> &ch, const char *name) {
	while (true) {
		int i = ch.recv();
		printf("%s working on %d\n", name, i);
		async::sleep(1);
	}
}

void amain() {

	async::channel<int> ch(3);

	await {

		A [&]{ worker(ch, "Steve"); };
		A [&]{ worker(ch, "Fred"); };

		A {
			for (int i = 10; i--;) {
				printf("Sending %d\n", i);
				ch.send(i);
			}
		};

	}

}
