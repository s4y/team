#include <team/async.h>
#include <iostream>

using namespace std;
using namespace async;

void amain() {

	int i;
	auto ev = mkevent(i);

	A [&]{
		sleep(1);
		ev->trigger(1);
		cout << "Fired!" << endl;
	};

	cout << "Waiting..." << endl;
	ev->wait();
	cout << "Got " << i << endl;

}
