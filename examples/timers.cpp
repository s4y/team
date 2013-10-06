#include <stdio.h>
#include <team/async.h>

using async::sleep;

void still_alive() {
	for (;;) {
		printf("  Still alive\n");
		sleep(1);
	}
}

int main() {

	// This is fire-and-forget since we're not in an await {}.
	// In this case, it'll keep running forever.
	A { still_alive(); };

	await {
		A { sleep(3); printf("Slow thing done\n"); };
		printf("Started a slow thing\n");
		A { sleep(1); printf("Quick thing done\n"); };
		printf("Started a quick thing\n");
	}

	printf("Everything done!\n");

}
