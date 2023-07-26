#pragma once

#include "oscar/Utils/Cpp20Shims.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>

// extremely basic support for a single-producer single-consumer (sp-sc) queue
namespace osc::spsc
{
    template<typename T>
    class Sender;

    template<typename T>
    class Receiver;

    // internal implementation class
    template<typename T>
    class Impl final {

        // queue mutex
        std::mutex m_Mutex;

        // queue != empty condvar for worker
        std::condition_variable m_Condvar;

        // the queue
        std::list<T> m_MessageQueue;

        // how many `Sender` classes use this Impl (should be 1/0)
        std::atomic<int32_t> m_NumSenders = 0;

        // how many `Receiver` classes use this impl (should be 1/0)
        std::atomic<int32_t> m_NumReceivers = 0;

        template<typename U>
        friend std::pair<Sender<U>, Receiver<U>> channel();
        friend class Sender<T>;
        friend class Receiver<T>;
    };

    // a class the client can send information through
    template<typename T>
    class Sender final {
        std::shared_ptr<Impl<T>> m_Impl;

        template<typename U>
        friend std::pair<Sender<U>, Receiver<U>> channel();

        Sender(std::shared_ptr<Impl<T>> impl) : m_Impl{std::move(impl)}
        {
            ++m_Impl->m_NumSenders;
        }
    public:
        Sender(Sender const&) = delete;
        Sender(Sender&&) noexcept = default;
        Sender& operator=(Sender const&) = delete;
        Sender& operator=(Sender&&) noexcept = default;

        ~Sender() noexcept
        {
            if (m_Impl)
            {
                --m_Impl->m_NumSenders;
                m_Impl->m_Condvar.notify_all();  // so receivers can know the hangup happened
            }
        }

        // asynchronously (non-blocking) send data
        void send(T v) {
            {
                std::lock_guard l{m_Impl->m_Mutex};
                m_Impl->m_MessageQueue.push_front(std::move(v));
            }
            m_Impl->m_Condvar.notify_one();
        }

        [[nodiscard]] bool isReceiverHungUp() noexcept
        {
            return m_Impl->m_NumReceivers <= 0;
        }
    };

    // a class the client can receive data from
    template<typename T>
    class Receiver final {
        std::shared_ptr<Impl<T>> m_Impl;

        template<typename U>
        friend std::pair<Sender<U>, Receiver<U>> channel();

        Receiver(std::shared_ptr<Impl<T>> impl) : m_Impl{std::move(impl)}
        {
            ++m_Impl->m_NumReceivers;
        }
    public:
        Receiver(Receiver const&) = delete;
        Receiver(Receiver&&) noexcept = default;
        Receiver& operator=(Receiver const&) = delete;
        Receiver& operator=(Receiver&&) noexcept = default;
        ~Receiver() noexcept
        {
            if (m_Impl)
            {
                --m_Impl->m_NumReceivers;
            }
        }

        // non-blocking: empty if nothing is sent, or the sender has hung up
        std::optional<T> tryReceive()
        {
            std::lock_guard l{m_Impl->m_Mutex};

            if (m_Impl->m_MessageQueue.empty())
            {
                return std::nullopt;
            }

            std::optional<T> rv{std::move(m_Impl->m_MessageQueue.back())};
            m_Impl->m_MessageQueue.pop_back();
            return rv;
        }

        // blocking: only empty if the sender hung up
        std::optional<T> recv()
        {
            std::unique_lock l{m_Impl->m_Mutex};

            // easy case: queue is not empty
            if (!m_Impl->m_MessageQueue.empty())
            {
                std::optional<T> rv{std::move(m_Impl->m_MessageQueue.back())};
                m_Impl->m_MessageQueue.pop_back();
                return rv;
            }

            // harder case: sleep until the queue is not empty, *or* until
            // the sender hangs up
            m_Impl->m_Condvar.wait(l, [&]()
            {
                return !m_Impl->m_MessageQueue.empty() || m_Impl->m_NumSenders <= 0;
            });

            // the condvar woke up (non-spuriously), either:
            //
            // - there's something in the queue (return it)
            // - the sender hung up (return nullopt)

            if (!m_Impl->m_MessageQueue.empty())
            {
                std::optional<T> rv{std::move(m_Impl->m_MessageQueue.back())};
                m_Impl->m_MessageQueue.pop_back();
                return rv;
            }
            else
            {
                return std::nullopt;
            }
        }

        [[nodiscard]] bool isSenderHungUp() noexcept
        {
            return m_Impl->m_NumSenders <= 0;
        }
    };

    // create a new threadsafe spsc channel (sender + receiver)
    template<typename T>
    std::pair<Sender<T>, Receiver<T>> channel()
    {
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
        jthread m_WorkerThread;

        // sending end of the channel: sends inputs to background thread
        spsc::Sender<Input> m_Tx;

        // receiving end of the channel: receives outputs from background thread
        spsc::Receiver<Output> m_Rx;

        // MAIN function for an SPSC worker thread
        static int main(
            stop_token,
            spsc::Receiver<Input> rx,
            spsc::Sender<Output> tx,
            Func input2output)
        {
            // continously try to receive input messages and
            // respond to them one-by-one
            while (!tx.isReceiverHungUp())
            {
                std::optional<Input> msg = rx.recv();

                if (!msg)
                {
                    return 0;  // sender hung up
                }

                tx.send(input2output(*msg));
            }

            return 0;  // receiver hung up
        }

        Worker(jthread&& worker, spsc::Sender<Input>&& tx, spsc::Receiver<Output>&& rx) :
            m_WorkerThread{std::move(worker)},
            m_Tx{std::move(tx)},
            m_Rx{std::move(rx)}
        {
        }

    public:

        // create a new worker
        static Worker create(Func f)
        {
            auto [reqTransmit, reqReceive] = spsc::channel<Input>();
            auto [respTransmit, respReceive] = spsc::channel<Output>();
            jthread worker{Worker::main, std::move(reqReceive), std::move(respTransmit), std::move(f)};
            return Worker{std::move(worker), std::move(reqTransmit), std::move(respReceive)};
        }

        void send(Input req)
        {
            m_Tx.send(std::move(req));
        }

        std::optional<Output> poll()
        {
            return m_Rx.tryReceive();
        }
    };
}
