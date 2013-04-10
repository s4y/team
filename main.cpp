#include <stdio.h>
#include <unistd.h>
#include "async.h"

void still_alive(const char name[]) {
	for (;;) {
		printf("Still alive: %s\n", name);
		asleep(1);
	}
}

void printLater(int seconds, const char message[]) {
	asleep(seconds);
	printf("%s\n", message);
}

void amain() {

	A(still_alive("Foosauce"));

	printf("Before\n");
	{ await
		A(printLater(1, "First thing done"));
		printf("Kicked off one thing\n");
		A(printLater(2, "Second thing done"));
		printf("Kicked off another thing\n");
	}
	printf("After\n");

}
