#pragma once

#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <limits>
#include <memory>
#include <thread>
#include <type_traits>
#include <unordered_set>
#include <vector>

// Cpp20Shims: *rough* shims to C++20 stdlib features
namespace osc
{
    // C++20: std::stop_token
    class stop_token final {
    public:
        stop_token(std::shared_ptr<std::atomic<bool>> st) : m_SharedState{std::move(st)}
        {
        }
        stop_token(stop_token const&) = delete;
        stop_token(stop_token&& tmp) noexcept : m_SharedState{tmp.m_SharedState}
        {
        }
        stop_token& operator=(stop_token const&) = delete;
        stop_token& operator=(stop_token&&) noexcept = delete;
        ~stop_token() noexcept = default;

        bool stop_requested() const noexcept
        {
            return *m_SharedState;
        }

    private:
        std::shared_ptr<std::atomic<bool>> m_SharedState;
    };

    // C++20: std::stop_source
    class stop_source final {
    public:
        stop_source() : m_SharedState{std::make_shared<std::atomic<bool>>(false)}
        {
        }
        stop_source(stop_source const&) = delete;
        stop_source(stop_source&& tmp) noexcept : m_SharedState{std::move(tmp.m_SharedState)}
        {
        }
        stop_source& operator=(stop_source const&) = delete;
        stop_source& operator=(stop_source&& tmp) noexcept
        {
            m_SharedState = std::move(tmp.m_SharedState);
            return *this;
        }
        ~stop_source() = default;

        bool request_stop() noexcept
        {
            // as-per the C++20 spec, but always true for this impl.
            bool has_stop_state = m_SharedState != nullptr;
            bool already_stopped = m_SharedState->exchange(true);

            return has_stop_state && !already_stopped;
        }

        stop_token get_token() const noexcept
        {
            return stop_token{m_SharedState};
        }

    private:
        std::shared_ptr<std::atomic<bool>> m_SharedState;
    };

    // C++20: std::jthread
    class jthread final {
    public:
        // Creates new thread object which does not represent a thread
        jthread() :
            m_StopSource{},
            m_Thread{}
        {
        }

        // Creates new thread object and associates it with a thread of execution.
        // The new thread of execution immediately starts executing
        template<typename Function, typename... Args>
        jthread(Function&& f, Args&&... args) :
            m_StopSource{},
            m_Thread{std::forward<Function>(f), m_StopSource.get_token(), std::forward<Args>(args)...}
        {
        }

        // threads are non-copyable
        jthread(jthread const&) = delete;
        jthread& operator=(jthread const&) = delete;

        // threads are moveable: the moved-from value is a non-joinable thread that
        // does not represent a thread
        jthread(jthread&&) noexcept = default;

        jthread& operator=(jthread&& other) noexcept
        {
            if (joinable())
            {
                m_StopSource.request_stop();
                m_Thread.join();
            }
            std::swap(m_StopSource, other.m_StopSource);
            std::swap(m_Thread, other.m_Thread);
            return *this;
        }

        // jthreads (or "joining threads") cancel + join on destruction
        ~jthread() noexcept
        {
            if (joinable())
            {
                m_StopSource.request_stop();
                m_Thread.join();
            }
        }

        bool joinable() const noexcept
        {
            return m_Thread.joinable();
        }

        bool request_stop() noexcept
        {
            return m_StopSource.request_stop();
        }

        void join()
        {
            return m_Thread.join();
        }

    private:
        stop_source m_StopSource;
        std::thread m_Thread;
    };

    // C++20: <numbers>
    namespace numbers {
        template<typename T> inline constexpr T pi_v = static_cast<T>(3.14159265358979323846);
        inline constexpr double pi = pi_v<double>;
    }

    // C++20: ssize
    template<typename Container>
    constexpr auto ssize(Container const& c) ->
        std::common_type_t<std::ptrdiff_t, std::make_signed_t<decltype(c.size())>>
    {
        using R = std::common_type_t<std::ptrdiff_t, std::make_signed_t<decltype(c.size())>>;
        return static_cast<R>(c.size());
    }

    // C++20: ssize
    template<typename T, std::ptrdiff_t N>
    constexpr std::ptrdiff_t ssize(const T (&)[N]) noexcept
    {
        return N;
    }

    // C++20: erase_if (for unordered_set)
    //
    // see: https://en.cppreference.com/w/cpp/container/unordered_set/erase_if
    template<
        typename Key,
        typename Hash,
        typename KeyEqual,
        typename Alloc,
        typename UnaryPredicate
    >
    typename std::unordered_set<Key, Hash, KeyEqual, Alloc>::size_type erase_if(
        std::unordered_set<Key, Hash, KeyEqual, Alloc>& c,
        UnaryPredicate pred)
    {
        auto oldSize = c.size();
        for (auto it = c.begin(), end = c.end(); it != end;)
        {
            if (pred(*it))
            {
                it = c.erase(it);
            }
            else
            {
                ++it;
            }
        }
        return oldSize - c.size();
    }

    // C++20: std::erase_if (std::vector)
    //
    // see: https://en.cppreference.com/w/cpp/container/vector/erase2
    template<
        typename T,
        typename Alloc,
        typename UnaryPredicate
    >
    constexpr typename std::vector<T, Alloc>::size_type erase_if(
        std::vector<T, Alloc>& c,
        UnaryPredicate pred)
    {
        auto const it = std::remove_if(c.begin(), c.end(), pred);
        auto const r = std::distance(it, c.end());
        c.erase(it, c.end());
        return r;
    }

    // C++20: popcount
    //
    // see: https://en.cppreference.com/w/cpp/numeric/popcount
    template<typename T>
    constexpr int popcount(T x) noexcept
    {
        static_assert(std::is_unsigned_v<T> && sizeof(T) <= sizeof(unsigned long long));

        unsigned long long uv = x;
        int i = 0;
        while (uv)
        {
            uv &= (uv - 1);
            ++i;
        }
        return i;
    }

    // C++20: countr_zero: counts the number of consecutive 0 bits, starting from the least significant bit
    //
    // see: https://en.cppreference.com/w/cpp/numeric/countr_zero
    template<typename T>
    constexpr int countr_zero(T x) noexcept
    {
        static_assert(std::is_unsigned_v<T> && sizeof(T) <= sizeof(unsigned long long));

        if (x == 0)
        {
            return std::numeric_limits<T>::digits;
        }

        unsigned long long uv = x;
        int rv = 0;
        while (!(uv & 0x1))
        {
            uv >>= 1;
            ++rv;
        }
        return rv;
    }

    template<typename T>
    constexpr int bit_width(T x) noexcept
    {
        static_assert(std::is_unsigned_v<T>);
        static_assert(!std::is_same_v<T, bool>);

        int rv = 0;
        while (x)
        {
            x >>= 1;
            ++rv;
        }
        return rv;
    }

    // C++20: to_array
    //
    // see: https://en.cppreference.com/w/cpp/container/array/to_array
    namespace detail
    {
        template <class T, std::size_t N, std::size_t... I>
        constexpr std::array<std::remove_cv_t<T>, N> to_array_impl(T (&&a)[N], std::index_sequence<I...>)
        {
            return {{std::move(a[I])...}};
        }
    }
    template <class T, std::size_t N>
    constexpr std::array<std::remove_cv_t<T>, N> to_array(T (&&a)[N])
    {
        return detail::to_array_impl(std::move(a), std::make_index_sequence<N>{});
    }

    namespace detail
    {
        template <class T, std::size_t N, std::size_t... I>
        constexpr std::array<std::remove_cv_t<T>, N> to_array_impl(T (&a)[N], std::index_sequence<I...>)
        {
            return {{a[I]...}};
        }
    }
    template <class T, std::size_t N>
    constexpr std::array<std::remove_cv_t<T>, N> to_array(T (&a)[N])
    {
        return detail::to_array_impl(std::move(a), std::make_index_sequence<N>{});
    }

    template<class To, class From>
    To bit_cast(From const& src) noexcept
    {
        static_assert(
            sizeof(To) == sizeof(From) &&
            std::is_trivially_copyable_v<From> &&
            std::is_trivially_copyable_v<To> &&
            std::is_trivially_constructible_v<To>
        );

        To dst;
        std::memcpy(&dst, &src, sizeof(To));
        return dst;
    }
}
