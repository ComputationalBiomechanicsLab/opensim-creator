#pragma once

namespace osc
{
    class IPopup {
    protected:
        IPopup() = default;
        IPopup(IPopup const&) = default;
        IPopup(IPopup&&) noexcept = default;
        IPopup& operator=(IPopup const&) = default;
        IPopup& operator=(IPopup&&) noexcept = default;

    public:
        virtual ~IPopup() noexcept = default;

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

        void onDraw()
        {
            implOnDraw();
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
        virtual void implOnDraw() = 0;
        virtual void implEndPopup() = 0;
    };
}
