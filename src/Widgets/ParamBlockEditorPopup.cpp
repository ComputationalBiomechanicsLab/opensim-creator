#include "ParamBlockEditorPopup.hpp"

#include "osc_config.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/ParamBlock.hpp"
#include "src/OpenSimBindings/ParamValue.hpp"
#include "src/OpenSimBindings/IntegratorMethod.hpp"
#include "src/Widgets/StandardPopup.hpp"

#include <imgui.h>
#include <nonstd/span.hpp>

#include <string>
#include <utility>
#include <variant>

template<class... Ts>
struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

static bool DrawEditor(osc::ParamBlock& b, int idx, double v)
{
    float fv = static_cast<float>(v);
    if (ImGui::InputFloat("##", &fv, 0.0f, 0.0f, OSC_DEFAULT_FLOAT_INPUT_FORMAT))
    {
        b.setValue(idx, static_cast<double>(fv));
        return true;
    }
    else
    {
        return false;
    }
}

static bool DrawEditor(osc::ParamBlock& b, int idx, int v)
{
    if (ImGui::InputInt("##", &v))
    {
        b.setValue(idx, v);
        return true;
    }
    else
    {
        return false;
    }
}

static bool DrawEditor(osc::ParamBlock& b, int idx, osc::IntegratorMethod im)
{
    nonstd::span<char const* const> methodStrings = osc::GetAllIntegratorMethodStrings();
    int method = static_cast<int>(im);

    if (ImGui::Combo("##", &method, methodStrings.data(), static_cast<int>(methodStrings.size())))
    {
        b.setValue(idx, static_cast<osc::IntegratorMethod>(method));
        return true;
    }
    else
    {
        return false;
    }
}

static bool DrawEditor(osc::ParamBlock& b, int idx)
{
    osc::ParamValue v = b.getValue(idx);
    bool rv = false;
    auto handler = Overloaded{
        [&b, &rv, idx](double dv) { rv = DrawEditor(b, idx, dv); },
        [&b, &rv, idx](int iv) { rv = DrawEditor(b, idx, iv); },
        [&b, &rv, idx](osc::IntegratorMethod imv) { rv = DrawEditor(b, idx, imv); },
    };
    std::visit(handler, v);
    return rv;
}

class osc::ParamBlockEditorPopup::Impl final : protected StandardPopup {
public:
    Impl(std::string_view popupName) :
        StandardPopup{std::move(popupName), 512.0f, 0.0f, ImGuiWindowFlags_AlwaysAutoResize}
    {
    }

    bool isOpen() const
    {
        return static_cast<StandardPopup const&>(*this).isOpen();
    }

    void open()
    {
        static_cast<StandardPopup&>(*this).open();
    }

    void close()
    {
        static_cast<StandardPopup&>(*this).close();
    }

    bool draw(ParamBlock& pb)
    {
        m_ParamBlock = &pb;
        if (m_ParamBlock)
        {
            static_cast<StandardPopup&>(*this).draw();
            return m_WasEdited;
        }
        else
        {
            return false;
        }
    }

private:
    void implDraw() override
    {
        if (m_ParamBlock == nullptr)
        {
            return;
        }

        m_WasEdited = false;

        ImGui::Columns(2);
        for (int i = 0, len = m_ParamBlock->size(); i < len; ++i)
        {
            ImGui::PushID(i);

            ImGui::TextUnformatted(m_ParamBlock->getName(i).c_str());
            ImGui::SameLine();
            DrawHelpMarker(m_ParamBlock->getName(i).c_str(), m_ParamBlock->getDescription(i).c_str());
            ImGui::NextColumn();

            if (DrawEditor(*m_ParamBlock, i))
            {
                m_WasEdited = true;
            }
            ImGui::NextColumn();

            ImGui::PopID();
        }
        ImGui::Columns();

        ImGui::Dummy({0.0f, 1.0f});

        if (ImGui::Button("save"))
        {
            requestClose();
        }
    }

    bool m_WasEdited = false;
    ParamBlock* m_ParamBlock = nullptr;
};

osc::ParamBlockEditorPopup::ParamBlockEditorPopup(std::string_view popupName) :
    m_Impl{new Impl{std::move(popupName)}}
{
}

osc::ParamBlockEditorPopup::ParamBlockEditorPopup(ParamBlockEditorPopup&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::ParamBlockEditorPopup& osc::ParamBlockEditorPopup::operator=(ParamBlockEditorPopup&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::ParamBlockEditorPopup::~ParamBlockEditorPopup() noexcept
{
    delete m_Impl;
}

bool osc::ParamBlockEditorPopup::isOpen() const
{
    return m_Impl->isOpen();
}

void osc::ParamBlockEditorPopup::open()
{
    m_Impl->open();
}

void osc::ParamBlockEditorPopup::close()
{
    m_Impl->close();
}

bool osc::ParamBlockEditorPopup::draw(ParamBlock& pb)
{
    return m_Impl->draw(pb);
}
