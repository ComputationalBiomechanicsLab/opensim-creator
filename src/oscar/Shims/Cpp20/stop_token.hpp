#pragma once

#include <atomic>
#include <memory>
#include <utility>

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
}
