#include <stdio.h>
#include <unistd.h>
#define _XOPEN_SOURCE
#include <ucontext.h>

#include <queue>
#include <functional>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
class context_t {
	ucontext_t m_ctx;
public:
	void swap(ucontext_t *ctx) { swapcontext(ctx, &m_ctx); }
};

class loop_t : public context_t {
	class task_t : public context_t {
		ucontext_t m_ctx;
		loop_t *m_loop;
	public:
		task_t(loop_t *loop) : m_loop(loop) {}
		void yield() { m_loop->swap(&m_ctx); }
	};

	ucontext_t m_ctx;
	std::queue<task_t *> m_tasks;

public:
	void run() {
		for (;;) {
			if (m_tasks.empty()) break;
			m_tasks.front()->swap(&m_ctx);
			delete m_tasks.front();
			m_tasks.pop();
		}
		printf("Nothing left to do\n");
	}

	void add(task_t *task) {
		m_tasks.push(task);
	}

	void yield() {
		task_t *t = new task_t(this);
		m_tasks.push(t);
		t->yield();
	}
};
#pragma clang diagnostic pop;

static loop_t loop;

void still_alive(const char name[]) {
	for (;;) {
		printf("Looped\n");
		printf("Looped: %s\n", name);
		loop.yield();
		printf("After tsleep\n");
	}
}


int main() {
	still_alive("Foo");
	loop.run();
}
