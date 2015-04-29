#include <team/net.h>
#include "util.h"

using namespace std;

const char *ip = "127.0.0.1";
const int port = 8080;

int main() {
	team::socket_tcp sock{ip, port};
	log("Listening on ", ip, ":", port);

	// You can call accept() and read() with no arguments (blocking-style) or
	// with a callback (the callback gets called asynchronously as long as the
	// socket's open). Callback-style is just sugar, check out its
	// implementation in <team/net.h>.
	// So, this echo server accepts an unlimited number of connections but only
	// reads one buffer at a time per client.
	sock.accept([](decltype(sock)::client_type client) {
		log(client.get(), " connected");
		while (auto buf = client->read()) {
			cout << client.get() << " data: ";
			cout.write(buf->base, buf->len);
			cout << endl;
			if (buf->len == 5 && !memcmp("quit\n", buf->base, 5)) { break; }
			client->write({move(buf)});
		}
		log(client.get(), " disconnected");
	});

}
