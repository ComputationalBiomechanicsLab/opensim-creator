#include "ParamBlockEditorPopup.h"

#include <libopensimcreator/Documents/Simulation/IntegratorMethod.h>
#include <libopensimcreator/Documents/ParamBlock.h>

#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/popups/popup.h>
#include <liboscar/ui/popups/popup_private.h>
#include <liboscar/utilities/std_variant_helpers.h>

#include <string_view>
#include <variant>

using namespace osc;

namespace
{
    bool DrawEditor(ParamBlock& b, int idx, double v)
    {
        // note: the input prevision has to be quite high here, because the
        //       ParamBlockEditorPopup has to edit simulation parameters, and
        //       one of those parameters is "Simulation Step Size (seconds)",
        //       which OpenSim defaults to a very very small number (10 ns)
        //
        //       see: #553

        auto fv = static_cast<float>(v);
        if (ui::draw_float_input("##", &fv, 0.0f, 0.0f, "%.9f")) {
            b.setValue(idx, static_cast<double>(fv));
            return true;
        }
        else {
            return false;
        }
    }

    bool DrawEditor(ParamBlock& b, int idx, int v)
    {
        if (ui::draw_int_input("##", &v)) {
            b.setValue(idx, v);
            return true;
        }
        else {
            return false;
        }
    }

    bool DrawEditor(ParamBlock& b, int idx, IntegratorMethod im)
    {
        bool rv = false;
        if (ui::begin_combobox("##", im.label())) {
            for (IntegratorMethod m : IntegratorMethod::all()) {
                if (ui::draw_selectable(m.label(), m == im)) {
                    b.setValue(idx, m);
                    rv = true;
                }
            }
            ui::end_combobox();
        }
        return rv;
    }

    bool DrawEditor(ParamBlock& b, int idx)
    {
        return std::visit(Overload{
            [&b, idx](const auto& val) { return DrawEditor(b, idx, val); },
        }, b.getValue(idx));
    }
}

class osc::ParamBlockEditorPopup::Impl final : public PopupPrivate {
public:

    explicit Impl(
        ParamBlockEditorPopup& owner,
        Widget* parent,
        std::string_view popupName,
        ParamBlock* paramBlock) :

        PopupPrivate{owner, parent, popupName, {512.0f, 0.0f}, ui::PanelFlag::AlwaysAutoResize},
        m_OutputTarget{paramBlock},
        m_LocalCopy{*m_OutputTarget}
    {}

    void draw_content()
    {
        m_WasEdited = false;

        ui::set_num_columns(2);
        for (int i = 0, len = m_LocalCopy.size(); i < len; ++i) {
            ui::push_id(i);

            ui::draw_text(m_LocalCopy.getName(i));
            ui::same_line();
            ui::draw_help_marker(m_LocalCopy.getName(i), m_LocalCopy.getDescription(i));
            ui::next_column();

            if (DrawEditor(m_LocalCopy, i)) {
                m_WasEdited = true;
            }
            ui::next_column();

            ui::pop_id();
        }
        ui::set_num_columns();

        ui::draw_vertical_spacer(1.0f/15.0f);

        if (ui::draw_button("save")) {
            *m_OutputTarget = m_LocalCopy;
            request_close();
        }
        ui::same_line();
        if (ui::draw_button("close")) {
            request_close();
        }
    }

private:
    bool m_WasEdited = false;
    ParamBlock* m_OutputTarget = nullptr;
    ParamBlock m_LocalCopy;
};

osc::ParamBlockEditorPopup::ParamBlockEditorPopup(
    Widget* parent,
    std::string_view popupName,
    ParamBlock* paramBlock) :

    Popup{std::make_unique<Impl>(*this, parent, popupName, paramBlock)}
{}
void osc::ParamBlockEditorPopup::impl_draw_content() { private_data().draw_content(); }
