#pragma once

#include <oscar/Platform/Widget.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <memory>

namespace osc { class TabPrivate; }

namespace osc
{
    class Tab : public Widget {
    public:
        UID id() const;
        bool is_unsaved() const { return impl_is_unsaved(); }
        bool try_save() { return impl_try_save(); }
        void on_draw_main_menu() { impl_on_draw_main_menu(); }

    protected:
        explicit Tab(std::unique_ptr<TabPrivate>&&);

        OSC_WIDGET_DATA_GETTERS(TabPrivate);
    private:
        virtual bool impl_is_unsaved() const { return false; }
        virtual bool impl_try_save() { return true; }
        virtual void impl_on_draw_main_menu() {}
    };
}
