#pragma once

#include <liboscar/shims/cpp20/stop_token.h>
#include <liboscar/shims/cpp20/thread.h>

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>

// extremely basic support for a single-producer single-consumer (sp-sc) queue
namespace osc
{
    template<typename T>
    class SpscSender;

    template<typename T>
    class SpscReceiver;

    namespace detail
    {
        // internal implementation class
        template<typename T>
        class SpscImplementation final {
        private:
            // queue mutex
            std::mutex mutex_;

            // queue != empty condition variable for worker
            std::condition_variable condition_variable_;

            // the queue
            std::list<T> message_queue_;

            // how many `SpscSender` classes use this SpscImplementation (should be 1/0)
            std::atomic<size_t> num_senders_ = 0;

            // how many `SpscReceiver` classes use this impl (should be 1/0)
            std::atomic<size_t> num_receivers_ = 0;

            template<typename U>
            friend std::pair<SpscSender<U>, SpscReceiver<U>> make_spsc_channel();
            friend class SpscSender<T>;
            friend class SpscReceiver<T>;
        };
    }

    // a class the client can send information through
    template<typename T>
    class SpscSender final {
        std::shared_ptr<detail::SpscImplementation<T>> impl_;

        template<typename U>
        friend std::pair<SpscSender<U>, SpscReceiver<U>> make_spsc_channel();

        explicit SpscSender(std::shared_ptr<detail::SpscImplementation<T>> impl) :
            impl_{std::move(impl)}
        {
            ++impl_->num_senders_;
        }
    public:
        SpscSender(const SpscSender&) = delete;
        SpscSender(SpscSender&&) noexcept = default;
        SpscSender& operator=(const SpscSender&) = delete;
        SpscSender& operator=(SpscSender&&) noexcept = default;

        ~SpscSender() noexcept
        {
            if (impl_) {
                --impl_->num_senders_;
                impl_->condition_variable_.notify_all();  // so receivers can know the hangup happened
            }
        }

        // asynchronously (non-blocking) send data
        void send(T v) {
            {
                std::lock_guard l{impl_->mutex_};
                impl_->message_queue_.push_front(std::move(v));
            }
            impl_->condition_variable_.notify_one();
        }

        [[nodiscard]] bool is_receiver_hung_up()
        {
            return impl_->num_receivers_ == 0;
        }
    };

    // a class the client can receive data from
    template<typename T>
    class SpscReceiver final {
        std::shared_ptr<detail::SpscImplementation<T>> impl_;

        template<typename U>
        friend std::pair<SpscSender<U>, SpscReceiver<U>> make_spsc_channel();

        explicit SpscReceiver(std::shared_ptr<detail::SpscImplementation<T>> impl) :
            impl_{std::move(impl)}
        {
            ++impl_->num_receivers_;
        }
    public:
        SpscReceiver(SpscReceiver const&) = delete;
        SpscReceiver(SpscReceiver&&) noexcept = default;
        SpscReceiver& operator=(const SpscReceiver&) = delete;
        SpscReceiver& operator=(SpscReceiver&&) noexcept = default;
        ~SpscReceiver() noexcept
        {
            if (impl_) {
                --impl_->num_receivers_;
            }
        }

        // non-blocking: empty if nothing is sent, or the sender has hung up
        std::optional<T> try_receive()
        {
            std::lock_guard l{impl_->mutex_};

            if (impl_->message_queue_.empty()) {
                return std::nullopt;
            }

            std::optional<T> rv{std::move(impl_->message_queue_.back())};
            impl_->message_queue_.pop_back();
            return rv;
        }

        // blocking: only empty if the sender hung up
        std::optional<T> receive()
        {
            std::unique_lock l{impl_->mutex_};

            // easy case: queue is not empty
            if (not impl_->message_queue_.empty())
            {
                std::optional<T> rv{std::move(impl_->message_queue_.back())};
                impl_->message_queue_.pop_back();
                return rv;
            }

            // harder case: sleep until the queue is not empty, *or* until
            // the sender hangs up
            impl_->condition_variable_.wait(l, [&]()
            {
                return not impl_->message_queue_.empty() or impl_->num_senders_ == 0;
            });

            // the condition variable woke up (non-spuriously), either:
            //
            // - there's something in the queue (return it)
            // - the sender hung up (return `std::nullopt`)

            if (not impl_->message_queue_.empty()) {
                std::optional<T> rv{std::move(impl_->message_queue_.back())};
                impl_->message_queue_.pop_back();
                return rv;
            }
            else {
                return std::nullopt;
            }
        }

        [[nodiscard]] bool is_sender_hung_up()
        {
            return impl_->num_senders_ == 0;
        }
    };

    // create a new thread-safe spsc channel (sender + receiver)
    template<typename T>
    std::pair<SpscSender<T>, SpscReceiver<T>> make_spsc_channel()
    {
        auto impl = std::make_shared<detail::SpscImplementation<T>>();
        return {SpscSender<T>{impl}, SpscReceiver<T>{impl}};
    }

    // SPSC worker: single-producer single-consumer worker abstraction
    //
    // encapsulates a worker background thread with thread-safe communication
    // channels
    template<typename Input, typename Output, typename Func>
    class SpscWorker {

        // worker (background thread)
        cpp20::jthread worker_thread_;

        // sending end of the channel: sends inputs to background thread
        SpscSender<Input> sender_;

        // receiving end of the channel: receives outputs from background thread
        SpscReceiver<Output> receiver_;

        // MAIN function for an SPSC worker thread
        static int main(
            cpp20::stop_token,
            SpscReceiver<Input> receiver,
            SpscSender<Output> sender,
            Func message_processor)
        {
            // continuously try to receive input messages and
            // respond to them one-by-one
            while (not sender.is_receiver_hung_up()) {
                std::optional<Input> message = receiver.receive();

                if (not message) {
                    return 0;  // sender hung up
                }

                sender.send(message_processor(*message));
            }

            return 0;  // receiver hung up
        }

        SpscWorker(
            cpp20::jthread&& worker,
            SpscSender<Input>&& sender,
            SpscReceiver<Output>&& receiver) :

            worker_thread_{std::move(worker)},
            sender_{std::move(sender)},
            receiver_{std::move(receiver)}
        {}

    public:

        // create a new worker
        static SpscWorker create(Func message_processor)
        {
            auto [request_sender, request_receiver] = make_spsc_channel<Input>();
            auto [response_sender, response_receiver] = make_spsc_channel<Output>();
            cpp20::jthread worker{SpscWorker::main, std::move(request_receiver), std::move(response_sender), std::move(message_processor)};
            return SpscWorker{std::move(worker), std::move(request_sender), std::move(response_receiver)};
        }

        void send(Input req)
        {
            sender_.send(std::move(req));
        }

        std::optional<Output> poll()
        {
            return receiver_.try_receive();
        }
    };
}
