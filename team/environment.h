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

	void spawn(std::function<void()> f, rendezvous_t *r) {
		m_returns.emplace();
		coroutine_t::spawn(this, &f, r);
		m_returns.pop();
	}
};

