#include <team/team.h>
#include "util.h"

using team::sleep;

void still_alive() {
	for (;;) {
		log("  Still alive");
		sleep(1);
	}
}

int main() {

	// This is fire-and-forget since we're not in an await {}.
	// In this case, it'll keep running forever.
	async { still_alive(); };

	await {
		async { sleep(3); log("Slow thing done"); };
		log("Started a slow thing");
		async { sleep(1); log("Quick thing done"); };
		log("Started a quick thing");
	}

	log("Everything done!");

}
