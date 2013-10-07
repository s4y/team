#include <team/async.h>
#include "util.h"

using namespace async;

int main() {

	int i;
	auto ev = mkevent(i);

	A {
		sleep(1);
		ev->trigger(1);
		log("Fired!");
	};

	log("Waiting...");
	ev->wait();
	log("Got ", i);

}
