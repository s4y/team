#pragma once

#ifndef _XOPEN_SOURCE
#error You must build with -D_XOPEN_SOURCE=1. Yes, this sucks. See https://github.com/Sidnicious/team/wiki/_XOPEN_SOURCE for details.
#endif

#include <ucontext.h>

#include <queue>
#include <functional>
#include <stdlib.h>
#include <uv.h>
#include <unistd.h>
#include <sys/mman.h>

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
	[[noreturn]] void load() { setcontext(&m_ctx); abort(); }
	template<typename... Args> void prepare(void(cb)(), Args... args) {
		makecontext(&m_ctx, cb, sizeof...(args), args...);
	}
	virtual ~context_t() {}
	context_t() = default;
	context_t(context_t &&) = default;

};
#pragma clang diagnostic pop;

namespace team {
	class stack {
		void *data;

	public:
		stack() : data(mmap(
			nullptr,
			SIGSTKSZ + getpagesize(),
			PROT_READ | PROT_WRITE,
#ifdef MAP_ANONYMOUS
			MAP_ANONYMOUS
#else
			MAP_ANON
#endif
			| MAP_SHARED
#ifdef MAP_GROWSDOWN
			| MAP_GROWSDOWN
#endif
			, -1, 0
		)) {
			// Guard the bottom of the stack
			mprotect(data, getpagesize(), PROT_NONE);
		}

		~stack() {
			fprintf(stderr, "munmap\n");
			munmap(data, SIGSTKSZ + getpagesize());
		}

		void bind(ucontext_t &ctx) {
			ctx.uc_stack.ss_sp = data;
			ctx.uc_stack.ss_size = SIGSTKSZ;
		}

		

	};
}

class coroutine_t : public context_t {
public:
	struct delegate {
		virtual void started(coroutine_t *) = 0;
		virtual void stopped(coroutine_t *) = 0;
		// virtual void threw(coroutine_t *) = 0; // Coming soon
	};
	static void cb(coroutine_t *target) { target->run(); }

	team::stack m_stack;
	context_t *m_parent;
	std::function<void()> f;
	delegate *d;

	void run() {
		if (d) d->started(this);
		f();
		delete this;
		// Isn't this bad? We just deleted our stack and ourself.
		if (d) d->stopped(this);
		yield(m_parent);
	}


public:
	coroutine_t(context_t *ctx, std::function<void()> &&_f, delegate *_d = nullptr) :
		m_parent(ctx), f(_f), d(_d)
	{
		save();
		m_stack.bind(m_ctx);
		prepare((void(*)())cb, this);
	}
};
