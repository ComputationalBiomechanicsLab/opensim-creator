#include "SaveChangesPopup.h"

#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Popups/SaveChangesPopupConfig.h>
#include <liboscar/UI/Popups/StandardPopup.h>

#include <utility>

class osc::SaveChangesPopup::Impl final : public StandardPopup {
public:

    explicit Impl(SaveChangesPopupConfig config) :
        StandardPopup{config.popup_name},
        config_{std::move(config)}
    {}

    void impl_draw_content() final
    {
        ui::draw_text_unformatted(config_.content);

        if (ui::draw_button("Yes")) {
            if (config_.on_user_clicked_save()) {
                close();
            }
        }

        ui::same_line();

        if (ui::draw_button("No")) {
            if (config_.on_user_clicked_dont_save()) {
                close();
            }
        }

        ui::same_line();

        if (ui::draw_button("Cancel")) {
            if (config_.on_user_cancelled()) {
                close();
            }
        }
    }
private:
    SaveChangesPopupConfig config_;
};


osc::SaveChangesPopup::SaveChangesPopup(const SaveChangesPopupConfig& config) :
    impl_{std::make_unique<Impl>(config)}
{}

osc::SaveChangesPopup::SaveChangesPopup(SaveChangesPopup&&) noexcept = default;
osc::SaveChangesPopup& osc::SaveChangesPopup::operator=(SaveChangesPopup&&) noexcept = default;
osc::SaveChangesPopup::~SaveChangesPopup() noexcept = default;

bool osc::SaveChangesPopup::impl_is_open() const
{
    return impl_->is_open();
}

void osc::SaveChangesPopup::impl_open()
{
    impl_->open();
}

void osc::SaveChangesPopup::impl_close()
{
    impl_->close();
}

bool osc::SaveChangesPopup::impl_begin_popup()
{
    return impl_->begin_popup();
}

void osc::SaveChangesPopup::impl_on_draw()
{
    impl_->on_draw();
}

void osc::SaveChangesPopup::impl_end_popup()
{
    impl_->end_popup();
}

void osc::SaveChangesPopup::on_draw()
{
    if (impl_->begin_popup()) {
        impl_->on_draw();
        impl_->end_popup();
    }
}
