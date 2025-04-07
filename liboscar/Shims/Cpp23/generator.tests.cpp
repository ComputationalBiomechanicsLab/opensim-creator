#include "generator.h"

#include <gtest/gtest.h>

using namespace osc;

namespace
{
	[[maybe_unused]] cpp23::generator<int> coroutine_that_returns_7()
	{
		co_yield 7;
	}
}
