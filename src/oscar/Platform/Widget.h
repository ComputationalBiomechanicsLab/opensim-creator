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

        // Returns `impl_on_event(e)`.
        //
        // This method directly notifies this `Widget` with no propagation, filtering,
        // or batching. If you want those kinds of features, you should use:
        //
        // - `App::post_event(Widget&, std::unique_ptr<Event>)` or
        // - `App::notify(Widget&, Event&)`
        bool on_event(Event& e) { return impl_on_event(e); }

        // Returns a lifetime-checked, but non-lockable, pointer to this `Widget`.
        //
        // Runtime lifetime is handy for checking logic/lifetime errors at runtime,
        // but does fundamentally fix any lifetime issues in your application. If
        // you're triggering runtime lifetime assertion exceptions, you need to fix
        // your code.
        LifetimedPtr<Widget> weak_ref();

        // If it has a parent, returns a pointer to the parent of this `Widget`; otherwise,
        // returns `nullptr`.
        Widget* parent();

        // If it has a parent, returns a pointer to the parent of this `Widget`; otherwise,
        // returns `nullptr`.
        const Widget* parent() const;
    protected:
        explicit Widget(std::unique_ptr<WidgetPrivate>&&);

        const WidgetPrivate& base_private_data() const { return *data_; }
        WidgetPrivate& base_private_data() { return *data_; }

    private:
        Widget(const Widget&) = delete;
        Widget& operator=(const Widget&) = delete;
        Widget(Widget&&) noexcept = delete;
        Widget& operator=(Widget&&) noexcept = delete;

        OSC_WIDGET_DATA_GETTERS(WidgetPrivate);

        virtual bool impl_on_event(Event&) { return false; }

        std::unique_ptr<WidgetPrivate> data_;
    };
}
