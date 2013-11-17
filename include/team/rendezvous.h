struct basic_rendezvous_t {
	env_t &loop;

	void operator <<(std::function<void()> &&f) const {
		loop.spawn(std::forward<std::function<void()>>(f), nullptr);
	}
};

class rendezvous_t : private context_t, public coroutine_t::delegate {

	env_t *m_loop;
	unsigned int m_count;
	bool m_waiting;
public:
	rendezvous_t(env_t *loop) :
		m_loop(loop), m_count(0), m_waiting(false) {}

	void started(coroutine_t *) override { m_count++; }
	void stopped(coroutine_t *) override { m_count--; if (m_waiting) load(); }

	void operator <<(std::function<void()> &&f)
	{ m_loop->spawn(std::forward<std::function<void()>>(f), this); }

	~rendezvous_t() {
		m_waiting = true;
		while (m_count) this->yield(m_loop);
	}
};

