#include "SaveChangesPopup.h"

#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/popups/PopupPrivate.h>
#include <liboscar/ui/popups/SaveChangesPopupConfig.h>

#include <utility>

class osc::SaveChangesPopup::Impl final : public PopupPrivate {
public:

    explicit Impl(
        SaveChangesPopup& owner,
        Widget* parent,
        SaveChangesPopupConfig config) :

        PopupPrivate{owner, parent, config.popup_name},
        config_{std::move(config)}
    {}

    void draw_content()
    {
        ui::draw_text(config_.content);

        if (ui::draw_button("Yes")) {
            if (config_.on_user_clicked_save()) {
                owner().close();
            }
        }

        ui::same_line();

        if (ui::draw_button("No")) {
            if (config_.on_user_clicked_dont_save()) {
                owner().close();
            }
        }

        ui::same_line();

        if (ui::draw_button("Cancel")) {
            if (config_.on_user_cancelled()) {
                owner().close();
            }
        }
    }
private:
    SaveChangesPopupConfig config_;
};


osc::SaveChangesPopup::SaveChangesPopup(
    Widget* parent,
    const SaveChangesPopupConfig& config) :
    Popup{std::make_unique<Impl>(*this, parent, config)}
{}
void osc::SaveChangesPopup::impl_draw_content() { private_data().draw_content(); }
