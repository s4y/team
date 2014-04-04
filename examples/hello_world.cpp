#include <team/team.h>
#include "util.h"

using namespace std;
using namespace team;

int work(int in) {
	sleep(1); // In real life we'd make a blocking network request here.
	return in;
}

int main() {

	vector<int> results(10);

	// Let's pretend to fetch ten users/emails/kittens, at the same time.
	await {
		for (size_t i = 0; i < results.size(); i++) {
			// C++11 lambdas can explicitly capture by value from their enclosing scope.
			// The value of `i` will be correct for each task.
			acall [&,i] { results[i] = work(i); };
		}
	}

	for (const int &res : results) {
		log(res);
	}
}
