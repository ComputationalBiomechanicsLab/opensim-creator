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

        bool isOpen()
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

        bool beginPopup()
        {
            return implBeginPopup();
        }

        void drawPopupContent()
        {
            implDrawPopupContent();
        }

        void endPopup()
        {
            implEndPopup();
        }

    private:
        virtual bool implIsOpen() const = 0;
        virtual void implOpen() = 0;
        virtual void implClose() = 0;
        virtual bool implBeginPopup() = 0;
        virtual void implDrawPopupContent() = 0;
        virtual void implEndPopup() = 0;
    };
}