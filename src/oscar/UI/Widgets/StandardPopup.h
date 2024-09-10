#pragma once

#include <oscar/Maths/Vec2.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Widgets/IPopup.h>
#include <oscar/Utils/CStringView.h>

#include <optional>
#include <string>
#include <string_view>

namespace osc { struct Rect; }

namespace osc
{
    // base class for implementing a standard UI popup (that blocks the whole screen
    // apart from the popup content)
    class StandardPopup : public IPopup {
    protected:
        StandardPopup(const StandardPopup&) = default;
        StandardPopup(StandardPopup&&) noexcept = default;
        StandardPopup& operator=(const StandardPopup&) = default;
        StandardPopup& operator=(StandardPopup&&) noexcept = default;
    public:
        virtual ~StandardPopup() noexcept = default;

        explicit StandardPopup(
            std::string_view popup_name
        );

        StandardPopup(
            std::string_view popup_name,
            Vec2 dimensions,
            ui::WindowFlags
        );

    protected:
        CStringView name() const { return popup_name_; }
        bool is_popup_opened_this_frame() const;
        void request_close();
        bool is_modal() const;
        void set_modal(bool);
        void set_rect(const Rect&);
        void set_dimensions(Vec2);
        void set_position(std::optional<Vec2>);

    private:
        // this standard implementation supplies these
        bool impl_is_open() const final;
        void impl_open() final;
        void impl_close() final;
        bool impl_begin_popup() final;
        void impl_on_draw() final;
        void impl_end_popup() final;

        // derivers can/must provide these
        virtual void impl_before_imgui_begin_popup() {}
        virtual void impl_after_imgui_begin_popup() {}
        virtual void impl_draw_content() = 0;
        virtual void impl_on_close() {}

        std::string popup_name_;
        Vec2i dimensions_;
        std::optional<Vec2i> maybe_position_;
        ui::WindowFlags popup_flags_;
        bool should_open_;
        bool should_close_;
        bool just_opened_;
        bool is_open_;
        bool is_modal_;
    };
}
