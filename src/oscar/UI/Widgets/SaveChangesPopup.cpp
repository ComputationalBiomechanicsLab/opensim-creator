#include "SaveChangesPopup.h"

#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Widgets/SaveChangesPopupConfig.h>
#include <oscar/UI/Widgets/StandardPopup.h>

#include <utility>

class osc::SaveChangesPopup::Impl final : public StandardPopup {
public:

    explicit Impl(SaveChangesPopupConfig config) :
        StandardPopup{config.popup_name},
        config_{std::move(config)}
    {}

    void impl_draw_content() final
    {
        ui::TextUnformatted(config_.content);

        if (ui::Button("Yes")) {
            if (config_.on_user_clicked_save()) {
                close();
            }
        }

        ui::SameLine();

        if (ui::Button("No")) {
            if (config_.on_user_clicked_dont_save()) {
                close();
            }
        }

        ui::SameLine();

        if (ui::Button("Cancel")) {
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
