#pragma once

#include <oscar/Utils/CStringView.hpp>

namespace osc
{
    // a virtual interface to a single UI panel (in ImGui terms, a Window)
    class IPanel {
    protected:
        IPanel() = default;
        IPanel(IPanel const&) = default;
        IPanel(IPanel&&) noexcept = default;
        IPanel& operator=(IPanel const&) = default;
        IPanel& operator=(IPanel&&) noexcept = default;
    public:
        virtual ~IPanel() noexcept = default;

        CStringView getName() const
        {
            return implGetName();
        }

        bool isOpen() const
        {
            return implIsOpen();
        }

        void open()
        {
            implOpen();
        }

        void close()
        {
            implClose();
        }

        void onDraw()
        {
            implOnDraw();
        }

    private:
        virtual CStringView implGetName() const = 0;
        virtual bool implIsOpen() const = 0;
        virtual void implOpen() = 0;
        virtual void implClose() = 0;
        virtual void implOnDraw() = 0;
    };
}
