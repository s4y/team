#pragma once

#include "context.h"
#include <stack>

class env_t : public context_t {

	std::stack<context_t> m_returns;

protected:

	operator ucontext_t *() override {
		return m_returns.empty() ? &m_ctx : static_cast<ucontext_t *>(m_returns.top());
	}

public:

	void blockOnce(context_t *ctx) {
		m_returns.emplace();
		yield(ctx);
		m_returns.pop();
	}

	void spawn(std::function<void()> &&f, rendezvous_t *r) {
		blockOnce(new coroutine_t(
			this, std::forward<std::function<void()>>(f), r
		));
	}
};

