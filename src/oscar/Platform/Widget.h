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

        bool on_event(Event& e) { return impl_on_event(e); }
    protected:
        explicit Widget(std::unique_ptr<WidgetPrivate>&&);

        const WidgetPrivate& private_data() const { return *data_; }
        WidgetPrivate& private_data() { return *data_; }
        const WidgetPrivate& base_private_data() const { return *data_; }
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

#define OSC_WIDGET_DATA_GETTERS(ImplClass)                                                                    \
    const ImplClass& private_data() const { return reinterpret_cast<const ImplClass&>(base_private_data()); } \
    ImplClass& private_data() { return reinterpret_cast<ImplClass&>(base_private_data()); }
