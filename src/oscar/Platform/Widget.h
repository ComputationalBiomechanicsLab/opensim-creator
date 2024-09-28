#pragma once

#include <oscar/Utils/LifetimedPtr.h>

#include <memory>

namespace osc { class Event; }
namespace osc { class WidgetPrivate; }

#define OSC_WIDGET_DATA_GETTERS(ImplClass)                                                                    \
    const ImplClass& private_data() const { return reinterpret_cast<const ImplClass&>(base_private_data()); } \
    ImplClass& private_data() { return reinterpret_cast<ImplClass&>(base_private_data()); }

namespace osc
{
    class Widget {
    public:
        Widget();
        virtual ~Widget() noexcept;

        bool on_event(Event& e) { return impl_on_event(e); }
        LifetimedPtr<Widget> weak_ref();

        Widget* parent();
        const Widget* parent() const;
    protected:
        explicit Widget(std::unique_ptr<WidgetPrivate>&&);
        Widget(Widget&&) noexcept;
        Widget& operator=(Widget&&);

        const WidgetPrivate& base_private_data() const { return *data_; }
        WidgetPrivate& base_private_data() { return *data_; }

    private:
        Widget(const Widget&) = delete;
        Widget& operator=(const Widget&) = delete;

        OSC_WIDGET_DATA_GETTERS(WidgetPrivate);

        virtual bool impl_on_event(Event&) { return false; }

        std::unique_ptr<WidgetPrivate> data_;
    };
}
