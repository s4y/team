#include <team/async.h>
#include "util.h"

using namespace std;
using namespace async;

int work(int in) {
	sleep(1); // In real life we'd make a blocking network request here.
	return in;
}

int main() {

	vector<int> results(10);

	// Let's pretend to fetch ten users/emails/kittens, at the same time.
	await {
		for (size_t i = 0; i < results.size(); i++) {
			// AC runs any callable asynchronously (in this case a lambda with custom capture)
			AC [&,i] { results[i] = work(i); };
		}
	}

	for (const int &res : results) {
		log(res);
	}
}
