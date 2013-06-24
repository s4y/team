#pragma once

#define _XOPEN_SOURCE
#include <ucontext.h>

#include <queue>
#include <functional>
#include <uv.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
class context_t {
protected:
	ucontext_t m_ctx;
public:
	virtual operator ucontext_t *() { return &m_ctx; }
	void yield(context_t *ctx) { swapcontext(*this, *ctx); }
	void save() { getcontext(&m_ctx); }
	void load() { setcontext(&m_ctx); }
	template<typename... Args> void prepare(void(cb)(), Args... args) {
		makecontext(&m_ctx, cb, sizeof...(args), args...);
	}

};
#pragma clang diagnostic pop;

class rendezvous_t;

class coroutine_t : public context_t {
	static coroutine_t *next;
	static void cb() { next->run(); }

	char m_stack[SIGSTKSZ];
	context_t *m_parent;
	std::function<void()> *f;
	rendezvous_t *m_rendezvous;

	void run();

	coroutine_t(context_t *ctx, std::function<void()> *_f, rendezvous_t *r);

public:
	static void spawn(context_t *ctx, std::function<void()> *f, rendezvous_t *r) {
		new coroutine_t(ctx, f, r);
	}
};
coroutine_t *coroutine_t::next = nullptr;

class rendezvous_t : private context_t {

	unsigned int m_count;
	context_t *m_loop;
	bool m_waiting;
public:
	rendezvous_t(context_t *loop) :
		m_loop(loop), m_count(0), m_waiting(false) {}

	void inc() { m_count++; }
	void dec() { m_count--; if (m_waiting) load(); }

	~rendezvous_t() {
		m_waiting = true;
		while (m_count) this->yield(m_loop);
	}
};

void coroutine_t::run() {
	if (m_rendezvous) m_rendezvous->inc();
	(*f)();
	delete this;
	// Isn't this bad? We just deleted our stack and ourself.
	if (m_rendezvous) m_rendezvous->dec();
	yield(m_parent);
}

coroutine_t::coroutine_t(context_t *ctx, std::function<void()> *_f, rendezvous_t *r) :
	m_parent(ctx), f(_f), m_rendezvous(r)
{
	save();
	m_ctx.uc_stack.ss_sp = m_stack;
	m_ctx.uc_stack.ss_size = sizeof(m_stack);
	prepare(cb);
	next = this;
	ctx->yield(this);
}