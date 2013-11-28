#include <team/team.h>
#include "util.h"

using namespace team;

int main() {

	event<int> ev;

	async {
		log("Waiting...");
		log("Got ", ev.get());
	};

	ev.trigger(1);

}
