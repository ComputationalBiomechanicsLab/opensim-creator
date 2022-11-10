#pragma once

namespace osc
{
    class VirtualPopup {
    protected:
        VirtualPopup() = default;
        VirtualPopup(VirtualPopup const&) = default;
        VirtualPopup(VirtualPopup&&) noexcept = default;
        VirtualPopup& operator=(VirtualPopup const&) = default;
        VirtualPopup& operator=(VirtualPopup&&) noexcept = default;

    public:
        virtual ~VirtualPopup() noexcept = default;

        virtual bool isOpen() const = 0;
        virtual void open() = 0;
        virtual void close() = 0;
        virtual bool beginPopup() = 0;
        virtual void drawPopupContent() = 0;
        virtual void endPopup() = 0;
    };
}