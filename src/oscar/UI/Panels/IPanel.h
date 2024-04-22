#pragma once

#include <oscar/Utils/CStringView.h>

namespace osc
{
    // a virtual interface to a single UI panel (in ImGui terms, a Window)
    class IPanel {
    protected:
        IPanel() = default;
        IPanel(const IPanel&) = default;
        IPanel(IPanel&&) noexcept = default;
        IPanel& operator=(const IPanel&) = default;
        IPanel& operator=(IPanel&&) noexcept = default;
    public:
        virtual ~IPanel() noexcept = default;

        CStringView name() const { return impl_get_name(); }
        bool is_open() const { return impl_is_open(); }
        void open() { impl_open(); }
        void close() { impl_close(); }
        void on_draw() { impl_on_draw(); }

    private:
        virtual CStringView impl_get_name() const = 0;
        virtual bool impl_is_open() const = 0;
        virtual void impl_open() = 0;
        virtual void impl_close() = 0;
        virtual void impl_on_draw() = 0;
    };
}
