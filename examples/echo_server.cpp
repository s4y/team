#include <stdio.h>
#include <team/net.h>
#include <iostream>
#include <memory>

const char *ip = "127.0.0.1";
const int port = 8080;

void amain() {
	async::listening_socket_tcp sock(ip, port);
	std::cout << "Listening on " << ip << ":" << port << std::endl;

	// You can call accept() and read() with no arguments (blocking-style) or
	// with a callback (the callback gets called asynchronously as long as the
	// socket's open). Callback-style is just sugar, check out its
	// implementation in <team/net.h>.
	// So, this echo server accepts an unlimited number of connections but only
	// reads one buffer at a time per client.
	sock.accept([](decltype(sock)::client_type client) {
		printf("%p connected\n", client.get());
		while (auto buf = client->read()) {
			printf("%p data: %.*s", client.get(), (int)buf->len, buf->base);
			if (buf->len == 5 && !memcmp("quit\n", buf->base, 5)) { break; }
			client->write({std::move(buf)});
		}
		fprintf(stderr, "%p disconnected\n", client.get());
	});

}
