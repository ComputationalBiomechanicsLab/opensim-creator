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
        CStringView name() const;
        bool is_unsaved() const { return impl_is_unsaved(); }
        bool try_save() { return impl_try_save(); }
        void on_mount() { impl_on_mount(); }
        void on_unmount() { impl_on_unmount(); }
        void on_tick() { impl_on_tick(); }
        void on_draw_main_menu() { impl_on_draw_main_menu(); }
        void on_draw() { impl_on_draw(); }

    protected:
        explicit Tab(std::unique_ptr<TabPrivate>&&);

        OSC_WIDGET_DATA_GETTERS(TabPrivate);
    private:
        virtual bool impl_is_unsaved() const { return false; }
        virtual bool impl_try_save() { return true; }
        virtual void impl_on_mount() {}
        virtual void impl_on_unmount() {}
        virtual void impl_on_tick() {}
        virtual void impl_on_draw_main_menu() {}
        virtual void impl_on_draw() = 0;
    };
}
