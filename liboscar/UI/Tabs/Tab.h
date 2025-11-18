#pragma once

#include <liboscar/Platform/Widget.h>
#include <liboscar/UI/Tabs/TabSaveResult.h>
#include <liboscar/Utils/UID.h>

#include <future>
#include <memory>

namespace osc { class TabPrivate; }

namespace osc
{
    class Tab : public Widget {
    public:
        UID id() const;
        bool is_unsaved() const { return impl_is_unsaved(); }
        std::future<TabSaveResult> try_save() { return impl_try_save(); }
        void on_draw_main_menu() { impl_on_draw_main_menu(); }

    protected:
        explicit Tab(std::unique_ptr<TabPrivate>&&);

        OSC_WIDGET_DATA_GETTERS(TabPrivate);
    private:
        // Implementors should return `true` if the contents of the `Tab`
        // are "unsaved". The `Tab`-managing implementation may then use
        // this to figure out whether it needs to schedule a call `try_save`
        // or not.
        virtual bool impl_is_unsaved() const { return false; }

        // Implementors should return a `std::future<bool>` that yields
        // its result once the save operation, which may be asynchronous,
        // yields a result. The result yielded via the future should be
        // `true` if the save operation was sucessful; otherwise, it
        // should be `false`.
        //
        // By default, returns a `std::future<bool>` that immediately yields
        // `TabSaveResult::Done`.
        virtual std::future<TabSaveResult> impl_try_save();

        virtual void impl_on_draw_main_menu() {}
    };
}
