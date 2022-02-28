#pragma once

#include <memory>
#include <stdexcept>

// events: a lot of this API was inspired by APIs like RxCpp: this is use a simplifed version of
//         those libraries that is "good enough" for OSC's current use-cases
namespace osc
{
    // noop onNext handler: can be used as a stand-in when the caller doesn't provide one
    template<class T>
    class OnNextEmpty {
    public:
        void operator()(T const&) const {}
    };

    // default onError handler: can be used as a stand-in when the caller doesn't provide one
    class OnErrorEmpty {
    public:
        void operator()(std::exception const& ex) const
        {
            throw ex;
        }
    };

    // noop onCompleted handler: can be used as a stand-in when the caller doesn't provide one
    class OnCompletedEmpty {
    public:
        void operator()() const {}
    };

    // a specific observer instantiation that holds three concretely-known callable objects
    // (OnNext, OnError, and OnCompleted) within the object
    template<class T, class OnNext = void, class OnError = void, class OnCompleted = void>
    class Observer {
    public:

        Observer(OnNext onNext = OnNext{}, OnError onError = OnError{}, OnCompleted onCompleted = OnCompleted{}) :
            m_OnNext{std::move(onNext)},
            m_OnError{std::move(onError)},
            m_OnCompleted{std::move(onCompleted)}
        {
        }

        void onNext(T const& v)
        {
            m_OnNext(v);
        }

        void onNext(T&& v)
        {
            m_OnNext(std::move(v));
        }

        void onError(std::exception const& e)
        {
            m_OnError(e);
        }

        void onCompleted()
        {
            m_OnCompleted();
        }

    private:
        OnNext m_OnNext;
        OnError m_OnError;
        OnCompleted m_OnCompleted;
    };

    // a virtual API for an observer: used as a type-erased API to a concrete observer
    // implementation (held on the heap as a shared pointer)
    template<class T>
    class VirtualObserver : public std::enable_shared_from_this<VirtualObserver<T>> {
    public:
        virtual ~VirtualObserver() noexcept;
        virtual void onNext(T const&) {}
        virtual void onNext(T&&) {}
        virtual void onError(std::exception const&) {}
        virtual void onCompleted() {}
    };

    // a specific instantiation of a virtual observer: effectively uses template
    // metaprogamming to override the VirtualObserver API: other code can then
    // heap-allocate this as a `std::shared_ptr<VirtualObserver>` for type-erasure
    template<class T, class TObserver>
    class SpecificObserver final : public VirtualObserver<T> {
    public:
        explicit SpecificObserver(TObserver o) :
            m_Observer{std::move(o)}
        {
        }

        void onNext(T const& v) override
        {
            m_Observer.onNext(v);
        }

        void onNext(T&& v) override
        {
            m_Observer.onNext(std::move(v));
        }

        void onError(std::exception const& ex) override
        {
            m_Observer.onError(ex);
        }

        void onCompleted() override
        {
            m_Observer.onCompleted();
        }

    private:
        TObserver m_Observer;
    };

    // partial template specialization (for type-erasure)
    //
    // this Observer<T> type is one that holds a type-erased pointer to a
    // `VirtualObserver`. It's what APIs etc. typically handle
    template<class T>
    class Observer<T, void, void, void> {
    public:
        Observer() = default;

        template<typename TConcreteObserver>
        explicit Observer(TConcreteObserver o) :
            m_Observer(std::make_shared<SpecificObserver<T, TConcreteObserver>>(std::move(o)))
        {
        }

        void onNext(T const& v)
        {
            if (m_Observer)
            {
                m_Observer->onNext(v);
            }
        }

        void onNext(T&& v)
        {
            if (m_Observer)
            {
                m_Observer->onNext(std::move(v));
            }
        }

        void onError(std::exception const& ex)
        {
            if (m_Observer)
            {
                m_Observer->onError(ex);
            }
        }

        void onCompleted()
        {
            if (m_Observer)
            {
                m_Observer->onCompleted();
            }
        }

    private:
        std::shared_ptr<VirtualObserver<T>> m_Observer;
    };

    // make a concrete observer with a concrete onNext function implementation
    template<class T, class OnNext>
    Observer<T, OnNext, OnErrorEmpty, OnCompletedEmpty> MakeObserver(OnNext&& onNextFn)
    {
        return Observer<T, OnNext, OnErrorEmpty, OnCompletedEmpty>{std::forward(onNextFn)};
    }

    template<class T, class OnNext>
    Observer<T> MakeObserverDynamic(OnNext&& onNextFn)
    {
        return Observer<T>{MakeObserver(std::forward<OnNext>(onNextFn))};
    }
}
