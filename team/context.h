#pragma once

#ifndef _XOPEN_SOURCE
#error You must build with -D_XOPEN_SOURCE=1. Yes, this sucks. See https://github.com/Sidnicious/team/wiki/_XOPEN_SOURCE for details.
#endif

#include <ucontext.h>

#include <queue>
#include <functional>
#include <uv.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
class context_t {
protected:
	ucontext_t m_ctx;

	// Use m_ctx directly instead of going through the virtual operator. Pretty
	// much just intended for use to spin up the event loop.
	void yield_real(context_t *ctx) { swapcontext(&m_ctx, *ctx); }
public:
	virtual operator ucontext_t *() { return &m_ctx; }
	void yield(context_t *ctx) { swapcontext(*this, *ctx); }
	void save() { getcontext(&m_ctx); }
	void load() { setcontext(&m_ctx); }
	template<typename... Args> void prepare(void(cb)(), Args... args) {
		makecontext(&m_ctx, cb, sizeof...(args), args...);
	}
	virtual ~context_t() {}

};
#pragma clang diagnostic pop;

class rendezvous_t;

class coroutine_t : public context_t {
	static void cb(coroutine_t *target) { target->run(); }

	char m_stack[SIGSTKSZ];
	context_t *m_parent;
	std::function<void()> f;
	rendezvous_t *m_rendezvous;

	void run();

public:
	coroutine_t(context_t *ctx, std::function<void()> &&_f, rendezvous_t *r = nullptr);
};

class rendezvous_t : private context_t {

	context_t *m_loop;
	unsigned int m_count;
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
	f();
	delete this;
	// Isn't this bad? We just deleted our stack and ourself.
	if (m_rendezvous) m_rendezvous->dec();
	yield(m_parent);
}

coroutine_t::coroutine_t(context_t *ctx, std::function<void()> &&_f, rendezvous_t *r) :
	m_parent(ctx), f(_f), m_rendezvous(r)
{
	save();
	m_ctx.uc_stack.ss_sp = m_stack;
	m_ctx.uc_stack.ss_size = sizeof(m_stack);
	prepare((void(*)())cb, this);
}
