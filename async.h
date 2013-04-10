#define _XOPEN_SOURCE
#include <ucontext.h>

#include <queue>
#include <stack>
#include <functional>
#include <uv.h>

#pragma once

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

class loop_t : public context_t {

	std::stack<context_t> m_returns;

protected:

	operator ucontext_t *() override {
		return m_returns.empty() ? &m_ctx : static_cast<ucontext_t *>(m_returns.top());
	}

public:

	uv_loop_t *uv;

	loop_t(uv_loop_t *_uv) : uv(_uv) {}

	void run() {
		uv_run(uv, UV_RUN_DEFAULT);
	}

	void spawn(std::function<void()> f, rendezvous_t *r) {
		m_returns.emplace();
		coroutine_t::spawn(this, &f, r);
		m_returns.pop();
	}
};

template<typename T, int(*Init)(uv_loop_t*, T *handle)>
class handle_t {
protected:
	static void _cb(T *handle, int status) {
		reinterpret_cast<handle_t*>(handle->data)->cb(handle, status);
	}
	virtual void cb(T *handle, int status) = 0;

	T m_handle;

	handle_t(uv_loop_t *loop) {
		int ret;
		if ((ret = Init(loop, &m_handle))) {
			// TODO: better error handling?
			fprintf(stderr, "libuv handle init failed with %d\n", ret);
			exit(1);
		}
		m_handle.data = this;
	}
};

class timer_t : public context_t, public handle_t<uv_timer_t, uv_timer_init> {

	virtual void cb(uv_timer_t *handle, int status) override {
		m_loop->yield(this);
	}

	loop_t *m_loop;

public:
	timer_t(loop_t *loop) : handle_t(loop->uv), m_loop(loop) {}
	void start(uint64_t msec) {
		uv_timer_start(&m_handle, _cb, msec, 0);
		this->yield(m_loop);
	}
};


namespace A {
	loop_t loop(uv_default_loop());
}
void asleep(int seconds) { timer_t(&A::loop).start(seconds * 1000); }

rendezvous_t * const __r = nullptr;

#define A(stmt) A::loop.spawn([&](){ stmt; }, __r)
#define await rendezvous_t _r(&A::loop); auto *__r = &_r;

void amain();

int main() {
	A(amain());
	A::loop.run();
}

