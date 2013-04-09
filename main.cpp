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
	void swap(context_t *ctx) { swapcontext(&ctx->m_ctx, &m_ctx); }
	void yield(context_t *ctx) { swapcontext(&m_ctx, &ctx->m_ctx); }
	void save() { getcontext(&m_ctx); }
};

class stack_context_t : public context_t {
	static std::function<void()> *next;
	static void cb() { auto n = next; next = nullptr; (*n)(); }

	char m_stack[SIGSTKSZ];

public:
	void spawn(std::function<void()> *f, context_t *ctx) {
		save();
		// m_ctx.uc_link = NULL; // TODO
		m_ctx.uc_stack.ss_sp = m_stack;
		m_ctx.uc_stack.ss_size = sizeof(m_stack);
		makecontext(&m_ctx, cb, 0);
		next = f;
		swap(ctx);
	}
};
std::function<void()> *stack_context_t::next = nullptr;
#pragma clang diagnostic pop;

class loop_t : public context_t {
	// class task_t : public context_t {
	// 	loop_t *m_loop;
	// public:
	// 	task_t(loop_t *loop) : m_loop(loop) {}
	// };
	class return_t : public context_t {
		bool returned;
	public:
		return_t() : returned(false) { }
		bool go() {
			printf("GOGOGO\n");
			save();
			if (returned) {
				return false;
			} else {
				returned = true;
				return true;
			}
		}
	};

	std::queue<context_t *> m_tasks;
	std::stack<context_t *> m_returns;

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

	// void add(task_t *task) {
	// 	m_tasks.push(task);
	// }

	void block() {
		auto t = new context_t();
		m_tasks.push(t);
		if (m_returns.empty()) {
			t->yield(this);
			printf("Unblock from main loop return\n");
		} else {
			auto r = m_returns.top();
			m_returns.pop();
			printf("Swapping to alt return\n");
			t->yield(r);
			delete r;
			// TODO: do I ever get here?
			printf("Unblock from alt return\n");
		}
	}

	void fork(std::function<void()> f) {
		// TODO: memory manage me?
		auto t = new stack_context_t();
		t->spawn(&f, this);
	}

	// bool here() {
	// 	auto t = new return_t();
	// 	m_returns.push(t);
	// 	return t->go();
	// }

	void sleep(unsigned int seconds) {
		block();
		::sleep(seconds);
	}
};

static loop_t loop;

void still_alive(const char name[]) {
	for (;;) {
		printf("Looped: %s\n", name);
		loop.sleep(1);
		printf("After tsleep\n");
	}
}


int main() {
	loop.fork([&](){ still_alive("Foo"); });
	loop.fork([&](){ still_alive("Bar"); });

	printf("About to run the loop\n");
	loop.run();
}
