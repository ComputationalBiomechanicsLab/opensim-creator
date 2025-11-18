#pragma once

#include <atomic>
#include <memory>
#include <utility>

namespace osc::cpp20
{
    // C++20: std::stop_token
    class stop_token final {
    public:
        explicit stop_token(std::shared_ptr<std::atomic<bool>> shared_state) :
            shared_state_{std::move(shared_state)}
        {}
        stop_token(stop_token const&) = delete;
        stop_token(stop_token&&) noexcept = default;
        stop_token& operator=(stop_token const&) = delete;
        stop_token& operator=(stop_token&&) noexcept = delete;
        ~stop_token() noexcept = default;

        bool stop_requested() const noexcept
        {
            return *shared_state_;
        }

    private:
        std::shared_ptr<std::atomic<bool>> shared_state_;
    };

    // C++20: std::stop_source
    class stop_source final {
    public:
        stop_source() = default;
        stop_source(stop_source const&) = delete;
        stop_source(stop_source&&) noexcept = default;
        stop_source& operator=(stop_source const&) = delete;
        stop_source& operator=(stop_source&& tmp) noexcept = default;
        ~stop_source() = default;

        bool request_stop() noexcept
        {
            // as-per the C++20 spec, but always true for this impl.
            const bool has_stop_state = shared_state_ != nullptr;
            const bool already_stopped = shared_state_->exchange(true);

            return has_stop_state and not already_stopped;
        }

        stop_token get_token() const noexcept
        {
            return stop_token{shared_state_};
        }

    private:
        std::shared_ptr<std::atomic<bool>> shared_state_ = std::make_shared<std::atomic<bool>>(false);
    };
}
