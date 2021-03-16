#pragma once

namespace osmv {
    [[noreturn]] void on_assertion_failure(char const* failing_code, char const* file, unsigned int line) noexcept;
}

#define OSMV_ASSERT_ALWAYS(expr)                                                                                       \
    (static_cast<bool>(expr) ? (void)0 : osmv::on_assertion_failure(#expr, __FILE__, __LINE__))

#ifdef OSMV_FORCE_ASSERTS_ENABLED
#define OSMV_ASSERT(expr) OSMV_ASSERT_ALWAYS(expr)
#elif !defined(NDEBUG)
#define OSMV_ASSERT(expr) OSMV_ASSERT_ALWAYS(expr)
#else
#define OSMV_ASSERT(expr)
#endif
