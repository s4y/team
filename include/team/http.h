#include "net.h"
#include <http_parser.h>
#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <memory>

namespace HTTP {

class Parser {
	http_parser p;
	bool success;

	public:

	typedef team::channel<std::tuple<const char *, size_t>> channel_type;

	channel_type url;
	channel_type header_field;
	channel_type header_value;
	channel_type body;

	private:

	int handle_message_begin() { return 0; }
	int handle_url(const char *at, size_t length) {
		url << std::make_tuple(at, length);
		return 0;
	}
	int handle_header_field(const char *at, size_t length) {
		header_field << std::make_tuple(at, length);
		return 0;
	}
	int handle_header_value(const char *at, size_t length) {
		header_value << std::make_tuple(at, length);
		return 0;
	}
	int handle_headers_complete() {
		url.close();
		header_field.close();
		header_value.close();
		return 0;
	}
	int handle_body(const char *at, size_t length) {
		body << std::make_tuple(at, length);
		return 0;
	}
	int handle_message_complete() {
		body.close();
		success = true;
		return 0;
	}

	template <int (Parser::*CB)()>
	static int void_cb(http_parser *p) {
		return (static_cast<Parser*>(p->data)->*CB)();
	}

	template <int (Parser::*CB)(const char*, size_t)>
	static int data_cb(http_parser *p, const char *at, size_t length) {
		return (static_cast<Parser*>(p->data)->*CB)(at, length);
	}

	const http_parser_settings settings{
		void_cb<&Parser::handle_message_begin>,
		data_cb<&Parser::handle_url>,
		nullptr,
		data_cb<&Parser::handle_header_field>,
		data_cb<&Parser::handle_header_value>,
		void_cb<&Parser::handle_headers_complete>,
		data_cb<&Parser::handle_body>,
		void_cb<&Parser::handle_message_complete>
	};

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
	const char *method() {
		return http_method_str((http_method)p.method);
	}

	bool parseFrom(std::unique_ptr<team::socket_tcp> &sock) {
		while (auto buf = sock->read()) {
			if (!execute(buf->base, buf->len)) return false;
			if (success) break;
		}
		if (!success) finish();
		return success;
	}
};

class Headers {
	public:
	typedef std::shared_ptr<const std::string> header_type;

	private:
	std::multimap<std::string, header_type> headers;

	public:
	Headers() = default;

	Headers(std::initializer_list<decltype(headers)::value_type> _init) : headers(_init) {}


	decltype(headers)::const_iterator find(const std::string &key) {
		return headers.find(key);
	}

	void add(const std::string &key, const header_type &value) {
		headers.emplace(key, value);
	}

	template <typename T>
	void add(const std::string &key, T &&value) {
		headers.emplace(key, std::make_shared<const std::string>(std::forward<T>(value)));
	}

	friend team::bufstream &operator<<(team::bufstream &s, const Headers &h) {
		for(const auto &field : h.headers) {
			s << field.first << ": " << *field.second << "\r\n";
		}
		return s;
	}

	friend std::ostream &operator<<(std::ostream &s, const Headers &h) {
		for(const auto &field : h.headers) {
			s << field.first << ": " << *field.second << "\r\n";
		}
		return s;
	}
};

class ClientRequest {

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
	const char *method;
	Headers headers;

	bool parseFrom(std::unique_ptr<team::socket_tcp> &sock) {
		using std::string;
		using std::vector;
		using std::pair;
		using std::move;

		Parser p(HTTP_REQUEST);
		string field, value;
		bool ret(false);

		auto collect_header = [&] {
			if (!value.empty()) {
				headers.add(move(field), move(value));
				field.clear();
				value.clear();
			}
		};

		await {
			async { collect(p.url, url); };
			async { collect(p.header_field, field, collect_header); };
			async { collect(p.header_value, value); };
			async { collect(p.body, body); };

			ret = p.parseFrom(sock);
		}
		collect_header();
		method = p.method();

		return ret;
	}

};

struct Response {
	std::unique_ptr<team::socket_tcp> sock;
	unsigned int status;
	Headers headers;

	Response(std::unique_ptr<team::socket_tcp> &&_sock) :
		sock(std::move(_sock)), status(200) {}

	void writeHeaders() {
		team::bufstream bufs;
		std::string statusLine = (std::stringstream() <<
			"HTTP/1.1 " << status << " Todo" << "\r\n").str();
		bufs << statusLine << headers << "\r\n";
		sock->write(std::move(bufs.bufs));
	}

	template <typename ...Args>
	void send(Args &&...args) {
		team::bufstream body;
		body.add(std::forward<Args>(args)...);
		headers.add("Content-Length", std::to_string(body.size));
		writeHeaders();
		sock->write(std::move(body.bufs));
	}
};

template <typename Req = ClientRequest, typename Res = Response>
struct Server {
	team::listening_socket_tcp sock;

	Server(const char *ip, int port) : sock(ip, port) {}

	void serve(std::function<void(Req &, Res &)> cb) {
		sock.accept([&](typename decltype(sock)::client_type client) {
			Req req;
			if (req.parseFrom(client)) {
				Res res(std::move(client));
				cb(req, res);
			}
		});
	}
	// void stop() { … }
};

}
