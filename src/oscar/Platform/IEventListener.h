#pragma once

namespace osc { class Event; }

namespace osc
{
    class IEventListener {
    protected:
        IEventListener() = default;
        IEventListener(const IEventListener&) = default;
        IEventListener(IEventListener&&) noexcept = default;
        IEventListener& operator=(const IEventListener&) = default;
        IEventListener& operator=(IEventListener&&) noexcept = default;
    public:
        virtual ~IEventListener() noexcept = default;

        bool on_event(Event& e) { return impl_on_event(e); }

    private:
        virtual bool impl_on_event(Event&) { return false; }
    };
}
