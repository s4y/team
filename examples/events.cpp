#include <team/team.h>
#include "util.h"

using namespace team;

int main() {

	int i;
	auto ev = mkevent(i);

	async {
		sleep(1);
		ev->trigger(1);
		log("Fired!");
	};

	log("Waiting...");
	ev->wait();
	log("Got ", i);

}
