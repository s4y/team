#pragma once

#include "team.h"
#include <string>

namespace team {

	namespace uv { namespace details {
		template <>
		void init<uv_tcp_t>(uv_loop_t *loop, uv_tcp_t *handle) {
			uv_tcp_init(loop, handle);
		}
	} }


	std::shared_ptr<struct addrinfo> getaddrinfo(
		const char* node, const char* service, const struct addrinfo* hints
	) {
		event<std::shared_ptr<struct addrinfo>> ev;
		uv_getaddrinfo_t h;

		h.data = &ev;

		int err = uv_getaddrinfo(loop.uv, &h,
			[](uv_getaddrinfo_t *req, int status, struct addrinfo *res) {
				(void)status;
				static_cast<decltype(ev)*>(req->data)->trigger(
					res ? decltype(ev)::type(res, uv_freeaddrinfo) : nullptr
				);
			}, node, service, hints
		);

		if (err) abort(); // TODO throw

		return ev.get();
	}

	// Base buffer. Does not copy or free its contents. Good for const char *s.
	struct buffer : public uv_buf_t {
		buffer(uv_buf_t &&_buf) : uv_buf_t(std::move(_buf)) {}
		buffer(char *d) : uv_buf_t(uv_buf_init(d, strlen(d))) {}
		buffer(char *d, size_t len) : uv_buf_t(uv_buf_init(d, len)) {}
		virtual ~buffer() {}
	};

	struct tmp_buffer : public buffer {

		// Should buffers be able to represent capacity and bytes used?

		explicit tmp_buffer(size_t suggested_size) : buffer((char*)malloc(suggested_size), suggested_size) {}
		tmp_buffer(uv_buf_t &&_buf) : buffer(std::move(_buf)) {}
		virtual ~tmp_buffer() { free(base); }

		static uv_buf_t alloc (uv_handle_t *handle, size_t suggested_size) {
			return uv_buf_init((char*)malloc(suggested_size), suggested_size);
		}
	};

	struct bufstream {
		std::vector<std::shared_ptr<buffer>> bufs;
		size_t size;

		std::vector<std::string> hold_strings;

		bufstream() : size(0) {}

		bufstream &operator <<(const char *s) {
			bufs.emplace_back(new buffer((char *)s, strlen(s)));
			size += bufs.back()->len;
			return *this;
		}

		template <size_t S>
		bufstream &operator <<(const char s[S]) {
			bufs.emplace_back(new buffer((char *)s, size - 1));
			size += bufs.back()->len;
			return *this;
		}

		bufstream &operator <<(std::pair<const char *, size_t> p) {
			bufs.emplace_back(new buffer((char *)p.first, p.second));
			size += bufs.back()->len;
			return *this;
		}

		bufstream &operator <<(const std::string &s) {
			bufs.emplace_back(new buffer((char *)s.c_str(), s.size()));
			size += bufs.back()->len;
			return *this;
		}

		bufstream &operator <<(const std::string &&s_in) {
			hold_strings.push_back(std::move(s_in));
			return *this << hold_strings.back();
		}

		template <typename T, typename ...Args>
		void add(T &&arg, Args &&...args) {
			*this << arg;
			add(args...);
		}

		void add() {}
	};

	namespace uv_details {
		struct write : uv_write_t {
			const std::vector<std::shared_ptr<buffer>> bufs;
			fence fence;
			write(decltype(bufs) &&_bufs) : bufs(std::forward<decltype(bufs)>(_bufs)) {}
		};
	}

	template <typename Pipe=uv_pipe_t, typename Client=Pipe>
	class pipe : public handle<Pipe, uv_stream_t> {
		public:
		typedef std::unique_ptr<Client> client_type;

		private:
		channel<client_type> ch;

		void listen_cb(uv_stream_t *handle, int status) {
			(void)handle;
			(void)status;
			// TODO: like read, stop listen if not accepted again
			async {
				ch.send(std::unique_ptr<Client>(new Client(*this)));
			};
		}

		public:
		pipe() : handle<Pipe, uv_stream_t>(loop.uv) {}

		explicit pipe(pipe &from) : pipe() {
			int err = uv_accept(reinterpret_cast<uv_stream_t *>(from.m_handle), (uv_stream_t*)this->m_handle);
			if (err) {
				fprintf(stderr, "Failed to accept a connection (%d)\n", err);
			}
		}


		void listen(int backlog) {
			if (int err = uv_listen(
				(uv_stream_t*)this->m_handle, backlog,
				handle<Pipe, Client>::template TEAM_UV_CALLBACK(uv_stream_t, pipe::listen_cb)
			)) {
				// TODO: Expose errors. Throw an exception?
				fprintf(stderr, "Failed to listen (%d) \n", err);
			}
		}

		client_type accept() { return ch.recv(); }

		void accept(std::function<void(client_type)> cb) {
			while (auto client = accept()) {
				async { cb(std::move(client)); };
			}
		}
	};

	class socket_tcp : public pipe<uv_tcp_t, socket_tcp> {
		public:
		uv_err_code err;

		socket_tcp(const char *ip, int port, int backlog = 128) {
			if (int err = uv_tcp_bind(this->m_handle, uv_ip4_addr(ip, port))) {
				// TODO: Expose errors. Throw an exception?
				fprintf(stderr, "Failed to bind to %s:%d (%d) \n", ip, port, err);
			} else {
				listen(backlog);
			}
		}

		explicit socket_tcp(pipe &from) : pipe(from) {}

		private:
		channel<std::unique_ptr<buffer>> ch;
		bool reading, want_read;

		void read_cb(uv_stream_t *stream, ssize_t nread, uv_buf_t buf) {
			if (nread == 0) {
				if (buf.base) free(buf.base);
				return;
			}
			want_read = false;
			if (nread > 0) {
				err = UV_OK;
				buf.len = nread;
				async { ch.send(std::unique_ptr<buffer>(new tmp_buffer(std::move(buf)))); };
				if (!want_read) {
					reading = false;
					uv_read_stop(stream);
				}
			} else {
				auto last_error = uv_last_error(loop.uv).code;
				err = last_error == UV_EOF ? UV_OK : err;
				async { ch.send(nullptr); };
			}
		}

		public:
		socket_tcp() : pipe(), reading(false), want_read(false) {}

		bool connect(const sockaddr_in &addr) {
			uv_connect_t connect;
			event<bool> ev;
			connect.data = &ev;
			uv_tcp_connect(&connect, this->m_handle, addr, [] (uv_connect_t *c, int status) {
				static_cast<event<bool>*>(c->data)->trigger(status == 0);
			});
			return ev.get();
		}

		auto read() -> decltype(ch.recv()) {
			if (!reading) {
				reading = true;
				uv_read_start(
					(uv_stream_t*)this->m_handle, tmp_buffer::alloc,
					TEAM_UV_CALLBACK(uv_stream_t, socket_tcp::read_cb)
				);
			}
			want_read = true;
			return ch.recv();
		}

		void read(std::function<void(decltype(ch.recv()))> cb) {
			while (auto data = read()) {
				async { cb(std::move(data)); };
			}
		}

		void write(std::vector<std::shared_ptr<buffer>> &&bufs) {
			auto *req = new uv_details::write(std::forward<std::vector<std::shared_ptr<buffer>>>(bufs));
			std::vector<uv_buf_t> bare_buffers;
			bare_buffers.reserve(bufs.size());
			for (const auto &buf : bufs) { bare_buffers.push_back(*buf); }
			uv_write((uv_write_t*)req, (uv_stream_t*)this->m_handle, &bare_buffers.front(), bare_buffers.size(), [](uv_write_t *req, int status){
				(void)status;
				delete reinterpret_cast<uv_details::write*>(req);
			});
			req->fence.wait();
		}

		template <typename ...Args>
		void write(Args &&...args) {
			write({std::make_shared<buffer>(const_cast<char *>(args))...});
		}
	};

}
