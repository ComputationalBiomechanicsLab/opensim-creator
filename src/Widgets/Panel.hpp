#pragma once

#include "src/Utils/CStringView.hpp"

namespace osc
{
    // a class that exposes a virtual interface to a user-visible panel
    class Panel {
    protected:
        Panel() = default;
        Panel(Panel const&) = default;
        Panel(Panel&&) noexcept = default;
        Panel& operator=(Panel const&) = default;
        Panel& operator=(Panel&&) noexcept = default;
    public:
        virtual ~Panel() noexcept = default;

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

        void draw()
        {
            implDraw();
        }

    private:
        virtual CStringView implGetName() const = 0;
        virtual bool implIsOpen() const = 0;
        virtual void implOpen() = 0;
        virtual void implClose() = 0;
        virtual void implDraw() = 0;
    };
}