#pragma once

namespace team {

	template <typename T = void> class channel;

	template <>
	class channel<> {
		size_t count;
		size_t max_size;
		bool isClosed;

		fence_queue senders;
		fence_queue receivers;

		public:
		explicit channel(size_t _max_size) :
			count(0), max_size(_max_size), isClosed(false)
		{}
		channel() : channel(0) {}
		channel(channel &&) = default;

		bool closed() { return isClosed; }

		void send() {
			if (closed()) abort(); // TODO: throw
			while (!receivers && count == max_size) {
				senders.wait();
			}
			count++;
			receivers.maybe_pop();
		}

		void recv() {
			while (!senders && !count) {
				if (closed()) return;
				receivers.wait();
			}
			senders.maybe_pop();
			count--;
		}

		void close() {
			isClosed = true;
			while (receivers) receivers.maybe_pop();
		}
	};

	template <typename T>
	class channel : public channel<> {
		std::queue<T> values;

		public:
		explicit channel(size_t _max_size) : channel<>(_max_size) {}
		channel() : channel<>() {}
		channel(channel &&) = default;

		template <typename Val>
		void send(Val &&v) {
			if (!closed()) values.push(std::forward<Val>(v));
			channel<>::send();
		}

		T recv() {
			channel<>::recv();
			if (closed()) return T();
			auto ret = std::move(values.front());
			values.pop();
			return ret;
		}

		template <typename Val>
		channel &operator<<(Val &&v) { send(std::forward<Val>(v)); return *this; }

		template <typename Val>
		channel &operator>>(Val &v) { v = recv(); return *this; }

		T operator*() { return recv(); }
	};

}
