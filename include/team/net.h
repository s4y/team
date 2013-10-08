#pragma once

#include "async.h"

namespace async {

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

	namespace uv_details {
		struct write : uv_write_t {
			const std::vector<std::shared_ptr<buffer>> bufs;
			fence fence;
			write(decltype(bufs) &&_bufs) : bufs(std::forward<decltype(bufs)>(_bufs)) {}
		};
	}

	class socket_tcp : public handle<uv_tcp_t, uv_tcp_init, uv_stream_t> {
		public:
		uv_err_code err;

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
				A { ch.send(std::unique_ptr<buffer>(new tmp_buffer(std::move(buf)))); };
				if (!want_read) {
					reading = false;
					uv_read_stop(stream);
				}
			} else {
				auto last_error = uv_last_error(loop.uv).code;
				err = last_error == UV_EOF ? UV_OK : err;
				A { ch.send(nullptr); };
			}
		}

		public:
		socket_tcp(uv_stream_t *server) : handle(loop.uv), reading(false), want_read(false) {
			int err = uv_accept(server, (uv_stream_t*)m_handle);
			if (err) {
				fprintf(stderr, "Failed to accept a connection (%d)\n", err);
			}
		}

		auto read() -> decltype(ch.recv()) {
			if (!reading) {
				reading = true;
				uv_read_start(
					(uv_stream_t*)m_handle, tmp_buffer::alloc,
					TEAM_UV_CALLBACK(uv_stream_t, socket_tcp::read_cb)
				);
			}
			want_read = true;
			return ch.recv();
		}

		void read(std::function<void(decltype(ch.recv()))> cb) {
			while (auto data = read()) {
				A { cb(std::move(data)); };
			}
		}

		void write(std::vector<std::shared_ptr<buffer>> &&bufs) {
			auto *req = new uv_details::write(std::forward<std::vector<std::shared_ptr<buffer>>>(bufs));
			std::vector<uv_buf_t> bare_buffers;
			bare_buffers.reserve(bufs.size());
			for (const auto &buf : bufs) { bare_buffers.push_back(*buf); }
			uv_write((uv_write_t*)req, (uv_stream_t*)m_handle, &bare_buffers.front(), bare_buffers.size(), [](uv_write_t *req, int status){
				delete reinterpret_cast<uv_details::write*>(req);
			});
			req->fence.wait();
		}

		template <typename ...Args>
		void write(Args &&...args) {
			write({std::make_shared<buffer>(const_cast<char *>(args))...});
		}
	};

	class listening_socket_tcp : public handle<uv_tcp_t, uv_tcp_init, uv_stream_t> {
		public:
		typedef std::unique_ptr<socket_tcp> client_type;

		private:
		channel<client_type> ch;

		void cb(uv_stream_t *handle, int status) {
			// TODO: like read, stop listen if not accepted again
			A {
				ch.send(std::unique_ptr<socket_tcp>(new socket_tcp(handle)));
			};
		}

		public:
		listening_socket_tcp(const char *ip, int port, int backlog = 128) : handle(loop.uv) {
			uv_tcp_bind(m_handle, uv_ip4_addr(ip, port));
			if (int err = uv_listen(
				(uv_stream_t*)m_handle, backlog,
				TEAM_UV_CALLBACK(uv_stream_t, listening_socket_tcp::cb)
			)) {
				// TODO: Expose errors. Throw an exception?
				fprintf(stderr, "Failed to listen on %s:%d (%d) \n", ip, port, err);
			}
		}

		client_type accept() { return ch.recv(); }

		void accept(std::function<void(client_type)> cb) {
			while (auto client = accept()) {
				A { cb(std::move(client)); };
			}
		}
	};
}
