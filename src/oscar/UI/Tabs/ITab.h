#pragma once

#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

namespace osc { class Event; }
namespace osc { class ITabHost; }

namespace osc
{
    // a virtual interface to a single UI tab/workspace
    class ITab {
    protected:
        ITab() = default;
        ITab(const ITab&) = default;
        ITab(ITab&&) noexcept = default;
        ITab& operator=(const ITab&) = default;
        ITab& operator=(ITab&&) noexcept = default;
    public:
        virtual ~ITab() noexcept = default;

        UID id() const { return impl_get_id(); }
        CStringView name() const { return impl_get_name(); }
        bool is_unsaved() const { return impl_is_unsaved(); }
        bool try_save() { return impl_try_save(); }
        void on_mount() { impl_on_mount(); }
        void on_unmount() { impl_on_unmount(); }
        bool on_event(Event& e) { return impl_on_event(e); }
        void on_tick() { impl_on_tick(); }
        void on_draw_main_menu() { impl_on_draw_main_menu(); }
        void on_draw() { impl_on_draw(); }

    private:
        virtual UID impl_get_id() const = 0;
        virtual CStringView impl_get_name() const = 0;
        virtual bool impl_is_unsaved() const { return false; }
        virtual bool impl_try_save() { return true; }
        virtual void impl_on_mount() {}
        virtual void impl_on_unmount() {}
        virtual bool impl_on_event(Event&) { return false; }
        virtual void impl_on_tick() {}
        virtual void impl_on_draw_main_menu() {}
        virtual void impl_on_draw() = 0;
    };
}
