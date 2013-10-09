#include <team/team.h>
#include "util.h"

using namespace std;
using namespace team;

class range : public generator<int> {
	int max;

	void generate() {
		for (int i = 0; i < max; ++i) {
			yield(i);
		}
	}
	public:
	explicit range(int _max) : max(_max) {}
};

template <typename G>
class take_t : public generator<typename G::value_type> {
	G &g;
	int max;

	void generate() {
		for (auto i : range(max)) { (void)i;
			auto v = g();
			if (!v) break;
			this->yield(std::move(v));
		}
	}
	public:
	explicit take_t(int _max, G &_g) : g(_g), max(_max) {}
};

template <typename G>
take_t<G> take(int max, G &g) { return take_t<G>(max, g); }

int main() {

	log("Check out this sweet range:");
	for (auto i : range(10)) {
		log(i);
	}

	log("\nHere are some Fibonacci numbers:");

	auto fib = make_generator([](yield<uint64_t> yield) {
		uint64_t a, b;
		yield((a = 0));
		yield((b = 1));
		for(;;) {
			std::tie(a, b) = std::make_tuple(b, a + b);
			yield(b);
		}
	});

	for (auto i : take(10, fib)) {
		log(i);
	}
}
