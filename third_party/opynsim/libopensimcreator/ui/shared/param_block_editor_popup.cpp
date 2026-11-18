#include "param_block_editor_popup.h"

#include <libopensimcreator/documents/param_block.h>

#include <libopynsim/solvers/integrator_method.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/popups/popup.h>
#include <liboscar/ui/popups/popup_private.h>
#include <liboscar/utilities/enum_helpers.h>
#include <liboscar/utilities/std_variant_helpers.h>

#include <string_view>
#include <variant>

using namespace osc;

namespace
{
    bool draw_editor(ParamBlock& b, int idx, double v)
    {
        // note: the input prevision has to be quite high here, because the
        //       ParamBlockEditorPopup has to edit simulation parameters, and
        //       one of those parameters is "Simulation Step Size (seconds)",
        //       which OpenSim defaults to a very very small number (10 ns)
        //
        //       see: #553

        auto fv = static_cast<float>(v);
        if (ui::draw_float_input("##", &fv, 0.0f, 0.0f, "%.9f")) {
            b.set_value(idx, static_cast<double>(fv));
            return true;
        } else {
            return false;
        }
    }

    bool draw_editor(ParamBlock& b, int idx, int v)
    {
        if (ui::draw_int_input("##", &v)) {
            b.set_value(idx, v);
            return true;
        } else {
            return false;
        }
    }

    bool draw_editor(ParamBlock& b, int idx, opyn::IntegratorMethod im)
    {
        bool rv = false;
        if (ui::begin_combobox("##", opyn::label_for(im))) {
            for (opyn::IntegratorMethod m : osc::make_option_iterable<opyn::IntegratorMethod>()) {
                if (ui::draw_selectable(opyn::label_for(m), m == im)) {
                    b.set_value(idx, m);
                    rv = true;
                }
            }
            ui::end_combobox();
        }
        return rv;
    }

    bool draw_editor(ParamBlock& b, int idx)
    {
        return std::visit(
            Overload{
                [&b, idx](const auto& val) { return draw_editor(b, idx, val); },
            },
            b.get_value(idx));
    }
}

class osc::ParamBlockEditorPopup::Impl final : public PopupPrivate {
public:
    explicit Impl(
        ParamBlockEditorPopup& owner,
        Widget* parent,
        std::string_view popup_name,
        ParamBlock* param_block) :

        PopupPrivate{owner, parent, popup_name, {512.0f, 0.0f}, ui::PanelFlag::AlwaysAutoResize},
        output_target_{param_block},
        local_copy_{*output_target_}
    {}

    void draw_content()
    {
        was_edited_ = false;

        ui::set_num_columns(2);
        for (int i = 0, len = local_copy_.size(); i < len; ++i) {
            ui::push_id(i);

            ui::draw_text(local_copy_.get_name(i));
            ui::same_line();
            ui::draw_help_marker(local_copy_.get_name(i), local_copy_.get_description(i));
            ui::next_column();

            if (draw_editor(local_copy_, i)) {
                was_edited_ = true;
            }
            ui::next_column();

            ui::pop_id();
        }
        ui::set_num_columns();

        ui::draw_vertical_spacer(1.0f / 15.0f);

        if (ui::draw_button("save")) {
            *output_target_ = local_copy_;
            request_close();
        }
        ui::same_line();
        if (ui::draw_button("close")) {
            request_close();
        }
    }

private:
    bool was_edited_ = false;
    ParamBlock* output_target_ = nullptr;
    ParamBlock local_copy_;
};

osc::ParamBlockEditorPopup::ParamBlockEditorPopup(
    Widget* parent, std::string_view popup_name, ParamBlock* param_block) :

    Popup{std::make_unique<Impl>(*this, parent, popup_name, param_block)}
{}
void osc::ParamBlockEditorPopup::impl_draw_content()
{
    private_data().draw_content();
}
