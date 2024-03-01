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
        if (ui::InputFloat("##", &fv, 0.0f, 0.0f, "%.9f"))
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
        if (ui::InputInt("##", &v))
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
        std::span<CStringView const> const methodStrings =
            GetAllIntegratorMethodStrings();
        auto method = static_cast<size_t>(im);

        if (ui::Combo("##", &method, methodStrings))
        {
            b.setValue(idx, static_cast<IntegratorMethod>(method));
            return true;
        }
        else
        {
            return false;
        }
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
    void implDrawContent() final
    {
        m_WasEdited = false;

        ui::Columns(2);
        for (int i = 0, len = m_LocalCopy.size(); i < len; ++i)
        {
            ui::PushID(i);

            ui::TextUnformatted(m_LocalCopy.getName(i));
            ui::SameLine();
            ui::DrawHelpMarker(m_LocalCopy.getName(i), m_LocalCopy.getDescription(i));
            ui::NextColumn();

            if (DrawEditor(m_LocalCopy, i))
            {
                m_WasEdited = true;
            }
            ui::NextColumn();

            ui::PopID();
        }
        ui::Columns();

        ui::Dummy({0.0f, 1.0f});

        if (ui::Button("save"))
        {
            *m_OutputTarget = m_LocalCopy;
            requestClose();
        }
        ui::SameLine();
        if (ui::Button("close"))
        {
            requestClose();
        }
    }

    bool m_WasEdited = false;
    ParamBlock* m_OutputTarget = nullptr;
    ParamBlock m_LocalCopy;
};

osc::ParamBlockEditorPopup::ParamBlockEditorPopup(std::string_view popupName, ParamBlock* paramBlock) :
    m_Impl{std::make_unique<Impl>(popupName, paramBlock)}
{
}

osc::ParamBlockEditorPopup::ParamBlockEditorPopup(ParamBlockEditorPopup&&) noexcept = default;
osc::ParamBlockEditorPopup& osc::ParamBlockEditorPopup::operator=(ParamBlockEditorPopup&&) noexcept = default;
osc::ParamBlockEditorPopup::~ParamBlockEditorPopup() noexcept = default;

bool osc::ParamBlockEditorPopup::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::ParamBlockEditorPopup::implOpen()
{
    m_Impl->open();
}

void osc::ParamBlockEditorPopup::implClose()
{
    m_Impl->close();
}

bool osc::ParamBlockEditorPopup::implBeginPopup()
{
    return m_Impl->beginPopup();
}

void osc::ParamBlockEditorPopup::implOnDraw()
{
    m_Impl->onDraw();
}

void osc::ParamBlockEditorPopup::implEndPopup()
{
    m_Impl->endPopup();
}
