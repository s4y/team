#pragma once

#include "events.h"

namespace async {
	loop_t loop(uv_default_loop());

	struct await_t {
		bool done;
		rendezvous_t r;

		await_t() : done(false), r(&async::loop) {}
		operator rendezvous_t *() { return &r; }
	};

	struct spawn_t {
		rendezvous_t *r;
		spawn_t(rendezvous_t *_r) : r(_r) {}
		void operator <<(std::function<void()> &&f) {
			loop.spawn(std::forward<std::function<void()>>(f), r); }
	};

	// High-level async constructs

	class fence {
		context_t ctx;
		public:
		~fence() { loop.blockOnce(&ctx); }
		void wait() { ctx.yield(&loop); }
	};

	class fence_queue {
		std::queue<std::unique_ptr<fence>> q;

		public:
		void push() { q.emplace(new fence); q.back()->wait(); }
		void maybe_pop() {
			if (q.empty()) return;
			// The fence will be destroyed when *after* it's been removed from
			// the queue. Else, we'll yield to the fence inside queues.pop()
			// and it may destroy us and reenter the fence's destructor
			auto fence = std::move(q.front());
			q.pop();
		}
	};

	template <typename ...S>
	class event {
		std::tuple<S&...> slots;
		context_t ctx;
		bool triggered, waiting;

		public:
		event(S &...s) : slots(s...), triggered(false), waiting(false) {}

		template <typename ...T>
		void trigger(T &&...t) {
			slots = std::forward_as_tuple(t...);
			triggered = true;
			if (waiting) loop.blockOnce(&ctx);
		}

		void wait() {
			if (triggered) return;
			waiting = true;
			ctx.yield(&loop);
			waiting = false;
		}
	};

	template <typename ...T>
	auto mkevent(T &...t) -> std::shared_ptr<event<T...>> {
		return std::make_shared<event<T...>>(t...);
	}

	template <typename T>
	class channel {

		std::queue<T> values;
		size_t max_size;

		fence_queue senders;
		fence_queue receivers;

		public:
		channel() : max_size(0) {}
		explicit channel(size_t _max_size) : max_size(_max_size) {}

		template <typename Val>
		void send(Val &&v) {
			while (max_size && values.size() == max_size) {
				senders.push();
			}

			values.push(std::forward<Val>(v));
			receivers.maybe_pop();
		}

		T recv() {
			while (!values.size()) {
				receivers.push();
			}
			auto ret = std::move(values.front());
			values.pop();
			senders.maybe_pop();
			return ret;
		}
	};

	template <typename T>
	class generator {
		channel<std::unique_ptr<T>> channel;

		coroutine_t *gen;
		bool running;
		context_t ctx;

		protected:

		template<typename V>
		void yield(V&& v){
			channel.send(std::unique_ptr<T>(new T(std::forward<V>(v))));
		}
		void yield(std::unique_ptr<T> &&p){ channel.send(std::move(p)); }

		virtual void generate() = 0;

		bool over;

		private:
		void start() { generate(); over = true; channel.send(nullptr); }

		public:

		typedef T value_type;

		generator() : channel(1), running(false), over(false), gen(
			new coroutine_t(&loop, std::bind(&generator::start, this), nullptr)
		) { }

		auto operator()() -> decltype(channel.recv()) {
			if (!running) {
				running = true;
				loop.blockOnce(gen);
			}
			return channel.recv();
		}

		class iterator {
			generator *g;
			std::unique_ptr<value_type> last;

			public:
			iterator(generator &_g) : g(&_g), last((*g)()) {}
			iterator() : g(nullptr) {}

			value_type *operator++() {
				last = (*g)();
				return last.get();
			}
			value_type &operator*() { return *last.get(); }

			bool operator!=(iterator &other) {
				return !(
					g == other.g ||
					(!g && other.g->over && !other.last) ||
					(!other.g && g->over && !last)
				);
			}
		};

		iterator begin() { return iterator(*this); }
		iterator end() { return iterator(); }

	};

	namespace util {

		// Based on http://stackoverflow.com/a/7943765/84745 by KennyTM
		template <typename T>
		struct arg_type : public arg_type<decltype(&T::operator())> {};

		template <typename C, typename R, typename... Args>
		struct arg_type<R(C::*)(Args...) const> {
			typedef typename std::tuple_element<0, std::tuple<Args...>>::type type;
		};

		template <typename T> using cb_type = typename arg_type<typename arg_type<T>::type>::type;
	}

	template<typename T>
	class lambda_generator : public generator<T> {
		public:

		class yield_t {
			lambda_generator &g;
			public:
			typedef T argument_type;
			template<typename V>
			void operator()(V&& v) { g.yield(std::forward<V>(v)); }
			yield_t(lambda_generator &_g) : g(_g) {}
		};

		private:
		std::function<void(yield_t)> f;

		public:
		explicit lambda_generator(decltype(f)&& _f) : f(std::forward<decltype(f)>(_f)) {}
		virtual void generate() override { f(yield_t(*this)); }
	};

	template <typename T> using yield = typename lambda_generator<T>::yield_t;

	template<typename F>
	lambda_generator<
		typename util::arg_type<F>::type::argument_type
	> make_generator(F&& f) {
		return lambda_generator<
			// This is ugly. I want to specialize util::cb_type to use
			// argument_type if it exists but had trouble doing that.
			typename util::arg_type<F>::type::argument_type
		>(std::forward<F>(f));
	}

	void sleep(int seconds) { timer(&loop).start(seconds * 1000); }
}

rendezvous_t * const __r = nullptr;

// Ugly. Open to ideas.

// A (async): Implies a lambda with default capture by reference
#define A async::spawn_t(__r) << [&]()
// AC (async call): Implies nothing. Takes any callable.
#define AC async::spawn_t(__r) <<

// #define await rendezvous_t _r(&async::loop); auto *__r = &_r;
#define await for (async::await_t __r; !__r.done; __r.done = true)

void amain();

int main() {
	A { amain(); };
	async::loop.run();
}

