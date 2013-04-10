#include <stdio.h>
#include <unistd.h>
#define _XOPEN_SOURCE
#include <ucontext.h>

#include <queue>
#include <stack>
#include <functional>

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
};

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
	makecontext(&m_ctx, cb, 0);
	next = this;
	ctx->yield(this);
}
#pragma clang diagnostic pop;

class loop_t : public context_t {

	std::queue<context_t> m_tasks;
	std::stack<context_t> m_returns;

protected:

	operator ucontext_t *() override {
		return m_returns.empty() ? &m_ctx : static_cast<ucontext_t *>(m_returns.top());
	}

public:
	void run() {
		for (;;) {
			if (m_tasks.empty()) break;
			this->yield(&m_tasks.front());
			m_tasks.pop();
		}
		printf("Nothing left to do\n");
	}

	void block() {
		m_tasks.emplace();
		m_tasks.back().yield(this);
	}

	void spawn(std::function<void()> f, rendezvous_t *r) {
		m_returns.emplace();
		coroutine_t::spawn(this, &f, r);
		m_returns.pop();
	}

	void sleep(unsigned int seconds) {
		block();
		::sleep(seconds);
	}
};

loop_t __loop;
rendezvous_t * const __r = nullptr;

#define A(stmt) __loop.spawn([&](){ stmt; }, __r)

void still_alive(const char name[]) {
	for (;;) {
		printf("Still alive: %s\n", name);
		__loop.sleep(1);
	}
}


int main() {

	A(
		rendezvous_t _r(&__loop); auto *__r = &_r;
		A(still_alive("Foo"));
		A(still_alive("Bar"));
		A(__loop.sleep(1));
		printf("jump to loop\n");
		__loop.block();
		printf("jumped to loop\n");
	);

	// A(
	// 	A(printf("Block 1\n"); __loop.block());
	// 	A(printf("Block 2\n"); __loop.block());
	// 	printf("outer\n");
	// );
	// printf("top\n");
	// A(__loop.block());

	__loop.run();
}
