#include "fence.h"

namespace team {

	template <typename T = void> class event;

	template <>
	class event<> {
		bool triggered;
		context *ctx;

		public:
		event(bool _triggered = false) : triggered(_triggered), ctx(nullptr) {}

		void trigger() {
			triggered = true;
			if (ctx) loop.blockOnce(ctx);
		}

		void wait() {
			if (triggered) return;
			assert(!ctx);
			ctx = current_context;
			ctx->yield(loop);
		}
	};

	template <typename T>
	class event : public event<> {
		std::unique_ptr<T> value;

		public:

		typedef T type;

		event() : event<>() {}

		template <typename ...Args>
		event(Args &&...args) : event() {
			trigger(std::forward<Args>(args)...);
		}

		template <typename ...Args>
		void trigger(Args &&...args) {
			value.reset(new T(std::forward<Args>(args)...));
			event<>::trigger();
		}

		T &&get() {
			if (!value) { event<>::wait(); }
			return std::move(*value);
		}
	};

}
