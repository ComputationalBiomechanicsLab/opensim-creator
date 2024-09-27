#pragma once

#include <oscar/Utils/LifetimedPtr.h>

#include <memory>

namespace osc { class Event; }
namespace osc { class WidgetPrivate; }

namespace osc
{
    class Widget {
    public:
        virtual ~Widget() noexcept;

        bool on_event(Event& e) { impl_on_event(e); }
    protected:
        explicit Widget(std::unique_ptr<WidgetPrivate>&&);

        WidgetPrivate& private_data() { return *data_; }
        WidgetPrivate& base_private_data() { return *data_; }
    private:
        Widget(const Widget&) = delete;
        Widget(Widget&&) noexcept;
        Widget& operator=(const Widget&) = delete;
        Widget& operator=(Widget&&);

        LifetimedPtr<Widget> weak_ref();

        virtual bool impl_on_event(Event&) { return false; }

        std::unique_ptr<WidgetPrivate> data_;
    };
}
