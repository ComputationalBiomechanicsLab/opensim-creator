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
        virtual ~Popup() noexcept = default;

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