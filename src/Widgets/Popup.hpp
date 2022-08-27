#pragma once

namespace osc
{
    class Popup {
    protected:
        Popup() = default;
        Popup(Popup const&) = default;
        Popup(Popup&&) noexcept = default;
        Popup& operator=(Popup const&) = default;
        Popup& operator=(Popup&&) noexcept = default;
    public:
        ~Popup() noexcept = default;

        virtual bool isOpen() const = 0;
        virtual void open() = 0;
        virtual void close() = 0;
        virtual void draw() = 0;
    };
}