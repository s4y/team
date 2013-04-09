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
	friend class coroutine_t;
protected:
	ucontext_t m_ctx;
public:
	void swap(context_t *ctx) { swapcontext(&ctx->m_ctx, &m_ctx); }
	void yield(context_t *ctx) { swapcontext(&m_ctx, &ctx->m_ctx); }
	void save() { getcontext(&m_ctx); }
};

class coroutine_t : public context_t {
	static coroutine_t *next;
	static void cb() { next->run(); }

	char m_stack[SIGSTKSZ];
	std::function<void()> *f;

	void run() {
		(*f)();
		delete this;
		printf("Coroutine be done\n");
	}

	coroutine_t(std::function<void()> *_f, context_t *ctx) : f(_f) {
		save();
		m_ctx.uc_link = &ctx->m_ctx;
		m_ctx.uc_stack.ss_sp = m_stack;
		m_ctx.uc_stack.ss_size = sizeof(m_stack);
		makecontext(&m_ctx, cb, 0);
		next = this;
		swap(ctx);
	}

public:
	static void spawn(std::function<void()> *f, context_t *ctx) {
		new coroutine_t(f, ctx);
	}
};
coroutine_t *coroutine_t::next = nullptr;
#pragma clang diagnostic pop;

class loop_t : public context_t {

	std::queue<context_t *> m_tasks;

public:
	void run() {
		for (;;) {
			if (m_tasks.empty()) break;
			m_tasks.front()->swap(this);
			delete m_tasks.front();
			m_tasks.pop();
		}
		printf("Nothing left to do\n");
	}

	void block() {
		auto t = new context_t();
		m_tasks.push(t);
		t->yield(this);
	}

	void spawn(std::function<void()> f) {
		coroutine_t::spawn(&f, this);
	}

	void sleep(unsigned int seconds) {
		block();
		::sleep(seconds);
	}
};

static loop_t loop;

void still_alive(const char name[]) {
	for (;;) {
		printf("Still alive: %s\n", name);
		loop.sleep(1);
	}
}


int main() {
	loop.spawn([&](){ still_alive("Foo"); });
	loop.spawn([&](){ still_alive("Bar"); });
	loop.spawn([&](){ loop.sleep(1); });

	loop.run();
}
