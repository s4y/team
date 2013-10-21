#include <team/http.h>
#include "util.h"
#include <unordered_map>

namespace Bahn {

	template <typename ...Inherits> struct ClientRequest;

	template <>
	struct ClientRequest<> : public HTTP::ClientRequest {};

	template <typename Next, typename ...Inherits>
	struct ClientRequest<Next, Inherits...> : public ClientRequest<Inherits...>, public Next {};


	template <typename ...Addons>
	class App {

		public:

		typedef ClientRequest<Addons...> req_type;
		typedef HTTP::Response res_type;
		typedef std::function<void (req_type &, res_type &)> cb_type;

		private:

		HTTP::Server<req_type, res_type> server;

		// TODO: make routes more awesome
		std::unordered_map<std::string, std::unordered_map<std::string, cb_type>> routes;

		void onConnection(req_type &req, res_type &res) {
			auto path = routes.find(req.url);
			if (path != routes.end()) {
				auto cb = path->second.find(req.method);
				if (cb != path->second.end()) {
					cb->second(req, res);
					return;
				}
			}
			res.status = 404;
			res.send("Not found, sorry\n");
		}

		public:

		App(const char *ip, const int port) : server(ip, port) {
			log("Listening on http://", ip, ":", port);
		}

		void serve() {
			server.serve(std::bind(&App::onConnection, this, std::placeholders::_1, std::placeholders::_2));
		}

		App &route(std::string verb, const std::string &path, const cb_type &cb) {
			routes[path][verb] = cb;
			return *this;
		}

		App &get(const std::string &path, const cb_type &cb) { route("GET", path, cb); return *this; }
		App &post(const std::string &path, const cb_type &cb) { route("POST", path, cb); return *this; }

	};

}

namespace HTTP {

	class Request {
		static const Headers default_headers;

		public:
		Headers headers;

		Request() : headers(default_headers) {}

		template <typename... Args>
		Request &add_header(Args &&...args) {
			headers.add(std::forward<Args>(args)...); return *this;
		}

		bool send(http_method method, const std::string &url) {
			struct addrinfo hint{};
			hint.ai_socktype = SOCK_STREAM;
			auto addr = team::getaddrinfo("example.com", "80", &hint);
			if (!addr) {
				// Bad host. Just say no.
				return false;
			}

			// TODO: try harder. Try lots of harders.
			team::socket_tcp sock;

			for (auto *a = addr.get(); a; a = a->ai_next) {
				switch (a->ai_family) {
					case AF_INET:
						if (sock.connect(*reinterpret_cast<
							struct sockaddr_in*
						>(a->ai_addr))) goto got_socket;
					default:
						break;
				}
			}
			return false;
got_socket:

			sock.write("GET / HTTP/1.1\r\nHost: example.com\r\n\r\n");

			while (auto d = sock.read()) {
				std::cout.write(d->base, d->len);
			}

			return true;
		}
	};

	const Headers Request::default_headers{
		{"foo", std::make_shared<const std::string>("bar")}
	};

}

const char *ip = "127.0.0.1";
const int port = 8080;

using namespace std;
using namespace team;

int main() {

	typedef Bahn::App<> App;

	App(ip, port)
		.get("/", [](App::req_type &req, App::res_type &res) {
			res.send("Welcome home.\n");
		})
		.get("/slow", [](App::req_type &req, App::res_type &res) {
			sleep(4);
			res.send("And... done.\n");
		})
		.get("/less_slow", [](App::req_type &req, App::res_type &res) {
			await {
				async { sleep(2); };
				async { sleep(2); };
			}
			res.send("And... done.\n");
		})
		.get("/fast", [](App::req_type &req, App::res_type &res) {
			async {
				sleep(1);
				cout << "Someone called me a second ago" << endl;
			};
			res.send("Fired and forgot.\n");
		})
		.serve();
}

