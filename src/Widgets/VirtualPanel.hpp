#pragma once

namespace osc
{
    // a class that exposes a virtual interface to a user-visible panel
    class VirtualPanel {
    protected:
        VirtualPanel() = default;
        VirtualPanel(VirtualPanel const&) = default;
        VirtualPanel(VirtualPanel&&) noexcept = default;
        VirtualPanel& operator=(VirtualPanel const&) = default;
        VirtualPanel& operator=(VirtualPanel&&) noexcept = default;
    public:
        virtual ~VirtualPanel() noexcept = default;

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
        virtual bool implIsOpen() const = 0;
        virtual void implOpen() = 0;
        virtual void implClose() = 0;
        virtual void implDraw() = 0;
    };
}