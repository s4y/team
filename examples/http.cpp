#include <team/http.h>
#include "util.h"

using namespace std;
using namespace async;

const char *ip = "127.0.0.1";
const int port = 8080;

int main() {

	log("Listening on http://", ip, ":", port);

	HTTP::Server(ip, port).serve([](HTTP::Request &req, socket_tcp &client) {
		// sleep(1);
		client.write(
            "HTTP/1.1 200 OK\r\n\r\n", "Hello there! You're at ",
            req.url.c_str(), "\n"
        );
	});

}
