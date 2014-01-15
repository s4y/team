namespace team {

	struct basic_rendezvous {
		env &loop;

		void operator <<(std::function<void()> &&f) const {
			loop.spawn(std::forward<std::function<void()>>(f), nullptr);
		}
	};

	class rendezvous : public coroutine::delegate {

		env *m_loop;
		context *m_ctx;
		unsigned int m_count;
		bool m_waiting;

	public:

		rendezvous(env *loop) :
			m_loop(loop), m_count(0), m_waiting(false) {}

		void started(coroutine *) override { m_count++; }
		void stopped(coroutine *, std::function<void()> &&post) override {
			m_count--;
			if (m_waiting) {
				current_context->yield(m_ctx, std::move(post));
				abort();
			}
		}

		void operator <<(std::function<void()> &&f)
		{ m_loop->spawn(std::forward<std::function<void()>>(f), this); }

		~rendezvous() {
			m_waiting = true;
			m_ctx = current_context;
			while (m_count) current_context->yield(*m_loop);
		}
	};

}
