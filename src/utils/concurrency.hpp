#pragma once

#include <mutex>
#include <condition_variable>
#include <optional>
#include <memory>
#include <utility>
#include <list>
#include <atomic>

namespace osc {
    // a mutex guard over a reference to `T`
    template<typename T, typename TGuard = std::lock_guard<std::mutex>>
    class Mutex_guard final {
        TGuard guard;
        T& ref;

    public:
        explicit Mutex_guard(std::mutex& mutex, T& _ref) : guard{mutex}, ref{_ref} {
        }

        T& operator*() noexcept {
            return ref;
        }

        T const& operator*() const noexcept {
            return ref;
        }

        T* operator->() noexcept {
            return &ref;
        }

        T const* operator->() const noexcept {
            return &ref;
        }

        TGuard& raw_guard() noexcept {
            return guard;
        }
    };

    // represents a `T` value that can only be accessed via a mutex guard
    template<typename T>
    class Mutex_guarded final {
        std::mutex mutex;
        T value;

    public:
        explicit Mutex_guarded(T _value) : value{std::move(_value)} {
        }

        // in-place constructor for T
        template<typename... Args>
        explicit Mutex_guarded(Args... args) : value{std::forward<Args...>(args)...} {
        }

        Mutex_guard<T> lock() {
            return Mutex_guard<T>{mutex, value};
        }

        Mutex_guard<T, std::unique_lock<std::mutex>> unique_lock() {
            return Mutex_guard<T, std::unique_lock<std::mutex>>{mutex, value};
        }
    };

    // extremely basic support for a single-producer single-consumer queue
    namespace spsc {
        template<typename T>
        class Sender;

        template<typename T>
        class Receiver;

        // internal implementation class
        template<typename T>
        class Impl final {
            std::mutex mutex;
            std::condition_variable condvar;
            std::list<T> queue;
            std::atomic<int> sender_count = 0;
            std::atomic<int> receiver_count = 0;

            friend std::pair<Sender<T>, Receiver<T>> channel();
            friend class Sender<T>;
            friend class Receiver<T>;
        };

        template<typename T>
        class Sender final {
            std::shared_ptr<Impl<T>> impl;

            Sender(std::shared_ptr<Impl<T>> _impl) : impl{std::move(_impl)} {
                ++impl->sender_count;
            }
        public:
            Sender(Sender const&) = delete;
            Sender(Sender&&) = default;
            Sender& operator=(Sender const&) = delete;
            Sender& operator=(Sender&&) = default;

            ~Sender() noexcept {
                if (impl) {
                    --impl->sender_count;
                    impl->condvar.notify_all();  // so receivers can know the hangup happened
                }
            }

            // non-blocking
            void send(T v) {
                {
                    std::lock_guard l{impl->mutex};
                    impl->queue.push_front(std::move(v));
                }
                impl->condvar.notify_one();
            }

            [[nodiscard]] bool is_receiver_hung_up() noexcept {
                return impl->receiver_count <= 0;
            }
        };

        template<typename T>
        class Receiver final {
            std::shared_ptr<Impl<T>> impl;

            Receiver(std::shared_ptr<Impl<T>> _impl) : impl{std::move(_impl)} {
                ++impl->receiver_count;
            }
        public:
            Receiver(Receiver const&) = delete;
            Receiver(Receiver&&) = default;
            Receiver& operator=(Receiver const&) = delete;
            Receiver& operator=(Receiver&&) = default;
            ~Receiver() noexcept {
                if (impl) {
                    --impl->receiver_count;
                }
            }

            // non-blocking: empty if nothing is sent, or the sender has hung up
            std::optional<T> try_recv() {
                std::lock_guard l{impl->mutex};

                if (impl->queue.empty()) {
                    return std::nullopt;
                }

                std::optional<T> rv{std::move(impl->queue.back())};
                impl->queue.pop_back();
                return rv;
            }

            // blocking: only empty if the sender hung up
            std::optional<T> recv() {
                std::lock_guard l{impl->mutex};

                // easy case: queue is not empty
                if (!impl->queue.empty()) {
                    std::optional<T> rv{std::move(impl->queue.back())};
                    impl->queue.pop_back();
                    return rv;
                }

                // harder case: sleep until the queue is not empty, *or* until
                // the sender hangs up
                impl->condvar.wait(l, [&]() {
                    return !impl->queue.empty() || impl->sender_count <= 0;
                });

                // the condvar woke up (non-spuriously), either:
                //
                // - there's something in the queue (return it)
                // - the sender hung up (return nullopt)

                if (!impl->queue.empty()) {
                    std::optional<T> rv{std::move(impl->queue.back())};
                    impl->queue.pop_back();
                    return rv;
                } else {
                    return std::nullopt;
                }
            }

            [[nodiscard]] bool is_sender_hung_up() noexcept {
                return impl->sender_count <= 0;
            }
        };

        // create a new channel
        template<typename T>
        std::pair<Sender<T>, Receiver<T>> channel() {
            auto impl = std::make_shared<Impl>();
            return {Sender<T>{impl}, Receiver<T>{impl}};
        }
    }
}
