#include "fence.h"

namespace team {

	template <typename T = void> class event;

	template <>
	class event<> {
		bool triggered;
		fence_queue waiters;

		public:
		event(bool _triggered = false) : triggered(_triggered) {}

		void trigger() {
			triggered = true;
			while (waiters) { waiters.maybe_pop(); }
		}

		void wait() {
			if (triggered) return;
			waiters.wait();
		}
	};

	template <typename T>
	class event : public event<> {
		std::unique_ptr<T> value;

		public:
		event() {}

		template <typename ...Args>
		event(Args &&...args) {
			trigger(std::forward<Args>(args)...);
		}

		template <typename ...Args>
		void trigger(Args &&...args) {
			value.reset(new T(std::forward<Args>(args)...));
			event<>::trigger();
		}

		T &get() {
			if (!value) { event<>::wait(); }
			return *value;
		}
	};

}
