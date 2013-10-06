#pragma once

#include "context.h"
#include <stack>

class env_t : public context_t {

	std::stack<context_t> m_returns;

protected:

	operator ucontext_t *() override {
		if (!m_returns.empty()) return m_returns.top();
		return &m_ctx;
	}

public:

	void blockOnce(context_t *ctx) {
		m_returns.emplace();
		yield(ctx);
		m_returns.pop();
	}

	void spawn(std::function<void()> &&f, rendezvous_t *r = nullptr) {
		blockOnce(new coroutine_t(
			this, std::forward<std::function<void()>>(f), r
		));
	}
};

