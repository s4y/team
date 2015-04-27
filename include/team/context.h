#pragma once

#ifndef _XOPEN_SOURCE
#error You must build with -D_XOPEN_SOURCE=1. Yes, this sucks. See https://github.com/Sidnicious/team/wiki/_XOPEN_SOURCE for details.
#endif

#include <ucontext.h>

#include <queue>
#include <functional>
#include <stdlib.h>
#include <uv/uv.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>

namespace team {

	struct context {
		
		std::function<void()> post_yield;
		bool active;

		virtual operator ucontext_t *() = 0;
		virtual void yield(context *, std::function<void()> && = nullptr) = 0;
		virtual ~context() = default;
		context() : active(false) {};
	};

	extern context *current_context;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	class basic_context : public context {
	protected:
		ucontext_t m_ctx;

	public:
		operator ucontext_t *() override { return &m_ctx; }
		void yield(context *ctx, std::function<void()> &&_post_yield = nullptr) override {
			assert(team::current_context == (context*)this);
			assert(!ctx->active);
			current_context = ctx;
			active = false;
			ctx->active = true;
			ctx->post_yield = std::move(_post_yield);
			swapcontext(*this, *ctx);
			if (post_yield) {
				post_yield();
				post_yield = nullptr;
			}
		}
		void save() { getcontext(&m_ctx); }
		[[noreturn]] void load() { setcontext(&m_ctx); abort(); }
		template<typename... Args> void prepare(void(cb)(), Args... args) {
			makecontext(&m_ctx, cb, sizeof...(args), args...);
		}

		basic_context() : context() {}
	};
#pragma clang diagnostic pop;

	basic_context root_context{};
	context *current_context(&root_context);
	context *get_default_context();

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

		~stack() { munmap(data, SIGSTKSZ + getpagesize()); }

		void bind(ucontext_t &ctx) {
			ctx.uc_stack.ss_sp = data;
			ctx.uc_stack.ss_size = SIGSTKSZ;
		}

		

	};

	class coroutine : public basic_context {

	public:
		struct delegate {
			virtual void started(coroutine *) = 0;
			virtual void stopped(coroutine *, std::function<void()> &&) = 0;
			// virtual void threw(coroutine *) = 0; // Coming soon
		};
		static void cb(coroutine *target) { target->run(); }

	private:

		team::stack m_stack;
		std::function<void()> f;
		delegate *d;

		void run() {
			if (d) d->started(this);
			f();
			if (d) d->stopped(this, [this]{ delete this; });
			current_context->yield(get_default_context(), [this]{ delete this; });
		}

	public:

		coroutine(std::function<void()> &&_f, delegate *_d = nullptr) :
			basic_context(), f(_f), d(_d)
		{
			save();
			m_stack.bind(m_ctx);
			prepare((void(*)())cb, this);
		}
	};

}
