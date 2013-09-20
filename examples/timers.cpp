#include <stdio.h>
#include <team/async.h>

using async::sleep;

void still_alive() {
	for (;;) {
		printf("  Still alive\n");
		sleep(1);
	}
}

void amain() {

	A [&]{ still_alive(); };

	await {
		A [&]{ sleep(3); printf("Slow thing done\n"); };
		printf("Started a slow thing\n");
		A [&]{ sleep(1); printf("Quick thing done\n"); };
		printf("Started a quick thing\n");
	}

	printf("Everything done!\n");

}
