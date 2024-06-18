#include "ParamBlockEditorPopup.h"

#include <OpenSimCreator/Documents/Simulation/IntegratorMethod.h>
#include <OpenSimCreator/Utils/ParamBlock.h>
#include <OpenSimCreator/Utils/ParamValue.h>

#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Widgets/StandardPopup.h>
#include <oscar/Utils/StdVariantHelpers.h>

#include <span>
#include <string>
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
        if (ui::draw_float_input("##", &fv, 0.0f, 0.0f, "%.9f"))
        {
            b.setValue(idx, static_cast<double>(fv));
            return true;
        }
        else
        {
            return false;
        }
    }

    bool DrawEditor(ParamBlock& b, int idx, int v)
    {
        if (ui::draw_int_input("##", &v))
        {
            b.setValue(idx, v);
            return true;
        }
        else
        {
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
        ParamValue v = b.getValue(idx);
        bool rv = false;
        auto handler = Overload
        {
            [&b, &rv, idx](double dv) { rv = DrawEditor(b, idx, dv); },
            [&b, &rv, idx](int iv) { rv = DrawEditor(b, idx, iv); },
            [&b, &rv, idx](IntegratorMethod imv) { rv = DrawEditor(b, idx, imv); },
        };
        std::visit(handler, v);
        return rv;
    }
}

class osc::ParamBlockEditorPopup::Impl final : public StandardPopup {
public:

    Impl(std::string_view popupName, ParamBlock* paramBlock) :
        StandardPopup{popupName, {512.0f, 0.0f}, ImGuiWindowFlags_AlwaysAutoResize},
        m_OutputTarget{paramBlock},
        m_LocalCopy{*m_OutputTarget}
    {
    }

private:
    void impl_draw_content() final
    {
        m_WasEdited = false;

        ui::set_num_columns(2);
        for (int i = 0, len = m_LocalCopy.size(); i < len; ++i)
        {
            ui::push_id(i);

            ui::draw_text_unformatted(m_LocalCopy.getName(i));
            ui::same_line();
            ui::draw_help_marker(m_LocalCopy.getName(i), m_LocalCopy.getDescription(i));
            ui::next_column();

            if (DrawEditor(m_LocalCopy, i))
            {
                m_WasEdited = true;
            }
            ui::next_column();

            ui::pop_id();
        }
        ui::set_num_columns();

        ui::draw_dummy({0.0f, 1.0f});

        if (ui::draw_button("save"))
        {
            *m_OutputTarget = m_LocalCopy;
            request_close();
        }
        ui::same_line();
        if (ui::draw_button("close"))
        {
            request_close();
        }
    }

    bool m_WasEdited = false;
    ParamBlock* m_OutputTarget = nullptr;
    ParamBlock m_LocalCopy;
};

osc::ParamBlockEditorPopup::ParamBlockEditorPopup(std::string_view popupName, ParamBlock* paramBlock) :
    m_Impl{std::make_unique<Impl>(popupName, paramBlock)}
{}
osc::ParamBlockEditorPopup::ParamBlockEditorPopup(ParamBlockEditorPopup&&) noexcept = default;
osc::ParamBlockEditorPopup& osc::ParamBlockEditorPopup::operator=(ParamBlockEditorPopup&&) noexcept = default;
osc::ParamBlockEditorPopup::~ParamBlockEditorPopup() noexcept = default;

bool osc::ParamBlockEditorPopup::impl_is_open() const
{
    return m_Impl->is_open();
}

void osc::ParamBlockEditorPopup::impl_open()
{
    m_Impl->open();
}

void osc::ParamBlockEditorPopup::impl_close()
{
    m_Impl->close();
}

bool osc::ParamBlockEditorPopup::impl_begin_popup()
{
    return m_Impl->begin_popup();
}

void osc::ParamBlockEditorPopup::impl_on_draw()
{
    m_Impl->on_draw();
}

void osc::ParamBlockEditorPopup::impl_end_popup()
{
    m_Impl->end_popup();
}
