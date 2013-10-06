#include "net.h"
#include <http_parser.h>
#include <string>

namespace HTTP {

class Parser {
	const static http_parser_settings settings;
	http_parser p;
	bool success;

	public:

	typedef async::channel<std::tuple<const char *, size_t>> channel_type;

	channel_type url;
	channel_type header_field;
	channel_type header_value;
	channel_type body;

	private:

	void send(channel_type &ch, const char *at, size_t length) {
		ch.send(std::make_tuple(at, length));
	}

	int handle_message_begin() { return 0; }
	int handle_url(const char *at, size_t length) {
		send(url, at, length);
		return 0;
	}
	int handle_header_field(const char *at, size_t length) {
		send(header_field, at, length);
		return 0;
	}
	int handle_header_value(const char *at, size_t length) {
		send(header_value, at, length);
		return 0;
	}
	int handle_headers_complete() {
		send(url, nullptr, 0);
		send(header_field, nullptr, 0);
		send(header_value, nullptr, 0);
		return 0;
	}
	int handle_body(const char *at, size_t length) {
		send(body, at, length);
		return 0;
	}
	int handle_message_complete() {
		body.close();
		success = true;
		return 0;
	}

	public:
	Parser(http_parser_type type) : success(false) {
		http_parser_init(&p, type);
		p.data = this;
	}

	bool execute(const char *data, size_t len) {
		return http_parser_execute(&p, &settings, data, len) == len;
	}
	bool finish() { return execute(nullptr, 0); }
	bool wasSuccessful() { return success; }

	bool parseFrom(std::unique_ptr<async::socket_tcp> &sock) {
		while (auto buf = sock->read()) {
			if (!execute(buf->base, buf->len)) return false;
			if (success) break;
		}
		if (!success) finish();
		return success;
	}
};

const http_parser_settings Parser::settings{
#define HTTP_CB(n)	  [] (http_parser *p) { return reinterpret_cast<Parser*>(p->data)->handle_##n(); }
#define HTTP_DATA_CB(n) [] (http_parser *p, const char *at, size_t length) { return reinterpret_cast<Parser*>(p->data)->handle_##n(at, length); }
	HTTP_CB(message_begin),
	HTTP_DATA_CB(url),
	nullptr,
	HTTP_DATA_CB(header_field),
	HTTP_DATA_CB(header_value),
	HTTP_CB(headers_complete),
	HTTP_DATA_CB(body),
	HTTP_CB(message_complete),
#undef HTTP_CB
#undef HTTP_DATA_CB
};

class Request {

	void collect(Parser::channel_type &ch, std::string &out, std::function<void()> f = nullptr) {
		using std::get;

		for (;;) {
			auto chunk = ch.recv();
			if (!get<0>(chunk)) break;
			if (f) f();
			out.append(get<0>(chunk), get<1>(chunk));
		}
	};

	public:

	std::string url, body;
	std::vector<std::pair<std::string, std::string>> headers;

	bool parseFrom(std::unique_ptr<async::socket_tcp> &sock) {
		using std::string;
		using std::vector;
		using std::pair;
		using std::move;

		Parser p(HTTP_REQUEST);
		string field, value;
		bool ret(false);

		auto collect_header = [&] {
			if (!value.empty()) {
				headers.emplace_back(move(field), move(value));
				field.clear();
				value.clear();
			}
		};

		await {
			A { collect(p.url, url); };
			A { collect(p.header_field, field, collect_header); };
			A { collect(p.header_value, value); };
			A { collect(p.body, body); };

			ret = p.parseFrom(sock);
		}
		collect_header();

		return ret;
	}

};

struct Server {
	async::listening_socket_tcp sock;

	Server(const char *ip, int port) : sock(ip, port) {}

	void serve(std::function<void(Request&, async::socket_tcp&)> cb) {
		sock.accept([&](decltype(sock)::client_type client) {
			Request req;
			if (req.parseFrom(client)) cb(req, *client);
		});
	}
	// void stop() { … }
};

}
