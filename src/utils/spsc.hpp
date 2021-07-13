#pragma once

#include "src/utils/shims.hpp"

#include <mutex>
#include <condition_variable>
#include <list>
#include <atomic>
#include <memory>
#include <utility>
#include <optional>

// extremely basic support for a single-producer single-consumer (sp-sc) queue
namespace osc::spsc {

    template<typename T>
    class Sender;

    template<typename T>
    class Receiver;

    // internal implementation class
    template<typename T>
    class Impl final {

        // queue mutex
        std::mutex mutex;

        // queue != empty condvar for worker
        std::condition_variable condvar;

        // the queue
        std::list<T> queue;

        // how many `Sender` classes use this Impl (should be 1/0)
        std::atomic<int> sender_count = 0;

        // how many `Receiver` classes use this impl (should be 1/0)
        std::atomic<int> receiver_count = 0;

        template<typename U>
        friend std::pair<Sender<U>, Receiver<U>> channel();
        friend class Sender<T>;
        friend class Receiver<T>;
    };

    // a class the client can send information through
    template<typename T>
    class Sender final {
        std::shared_ptr<Impl<T>> impl;

        template<typename U>
        friend std::pair<Sender<U>, Receiver<U>> channel();

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

        // asynchronously (non-blocking) send data
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

    // a class the client can receive data from
    template<typename T>
    class Receiver final {
        std::shared_ptr<Impl<T>> impl;

        template<typename U>
        friend std::pair<Sender<U>, Receiver<U>> channel();

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
            std::unique_lock l{impl->mutex};

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

    // create a new threadsafe spsc channel (sender + receiver)
    template<typename T>
    std::pair<Sender<T>, Receiver<T>> channel() {
        auto impl = std::make_shared<Impl<T>>();
        return {Sender<T>{impl}, Receiver<T>{impl}};
    }

    // SPSC worker: single-producer single-consumer worker abstraction
    //
    // encapsulates a worker background thread with threadsafe communication
    // channels
    template<typename Input, typename Output, typename Func>
    class Worker {

        // worker (background thread)
        osc::jthread worker;

        // sending end of the channel: sends inputs to background thread
        spsc::Sender<Input> tx;

        // receiving end of the channel: receives outputs from background thread
        spsc::Receiver<Output> rx;

        // MAIN function for an SPSC worker thread
        static int main(osc::stop_token,
                        spsc::Receiver<Input> rx,
                        spsc::Sender<Output> tx,
                        Func input2output) {

            // continously try to receive input messages and
            // respond to them one-by-one
            while (!tx.is_receiver_hung_up()) {
                std::optional<Input> msg = rx.recv();

                if (!msg) {
                    return 0;  // sender hung up
                }

                tx.send(input2output(*msg));
            }

            return 0;  // receiver hung up
        }

        Worker(osc::jthread&& _worker, spsc::Sender<Input>&& _tx, spsc::Receiver<Output>&& _rx) :
            worker{std::move(_worker)}, tx{std::move(_tx)}, rx{std::move(_rx)} {
        }

    public:

        // create a new worker
        static Worker create(Func f) {
            auto [req_tx, req_rx] = spsc::channel<Input>();
            auto [resp_tx, resp_rx] = spsc::channel<Output>();
            osc::jthread worker{Worker::main, std::move(req_rx), std::move(resp_tx), std::move(f)};
            return Worker{std::move(worker), std::move(req_tx), std::move(resp_rx)};
        }

        void send(Input req) {
            tx.send(std::move(req));
        }

        std::optional<Output> poll() {
            return rx.try_recv();
        }
    };
}
