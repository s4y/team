#include <team/net.h>
#include "util.h"

using namespace std;
using namespace team;

vector<string> host(const char *name) {
	vector<string> out;
	struct addrinfo hint{};
	hint.ai_socktype = SOCK_STREAM;
	if (auto res = getaddrinfo(name, "http", &hint)) {
		for (addrinfo *a(res.get()); a; a = a->ai_next) {
			switch (a->ai_family) {
				case AF_INET: {
					char buf[INET_ADDRSTRLEN];
					inet_ntop(a->ai_family, &reinterpret_cast<
						struct sockaddr_in*
					>(a->ai_addr)->sin_addr, buf, sizeof(buf));
					out.emplace_back(buf);
					break;
				} case AF_INET6: {
					char buf[INET6_ADDRSTRLEN];
					inet_ntop(a->ai_family, &reinterpret_cast<
						struct sockaddr_in6*
					>(a->ai_addr)->sin6_addr, buf, sizeof(buf));
					out.emplace_back(buf);
					break;
				} default: continue;
			}
		}
	}
	return out;
}

int main(int argc, char *argv[]) {

	if (argc != 2) {
		cerr << "usage: " << argv[0] << " hostname" << endl;
		return 2;
	}

	auto addrs = host(argv[1]);
	if (!addrs.size()) return 1;

	for (const string &s : addrs) {
		log(s);
	}
}

