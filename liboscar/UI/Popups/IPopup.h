#pragma once

namespace osc
{
    class IPopup {
    protected:
        IPopup() = default;
        IPopup(const IPopup&) = default;
        IPopup(IPopup&&) noexcept = default;
        IPopup& operator=(const IPopup&) = default;
        IPopup& operator=(IPopup&&) noexcept = default;

    public:
        virtual ~IPopup() noexcept = default;

        bool is_open() { return impl_is_open(); }
        void open() { impl_open(); }
        void close() { impl_close(); }

        bool begin_popup() { return impl_begin_popup(); }
        void on_draw() { impl_on_draw(); }
        void end_popup() { impl_end_popup(); }

    private:
        virtual bool impl_is_open() const = 0;
        virtual void impl_open() = 0;
        virtual void impl_close() = 0;

        virtual bool impl_begin_popup() = 0;
        virtual void impl_on_draw() = 0;
        virtual void impl_end_popup() = 0;
    };
}
