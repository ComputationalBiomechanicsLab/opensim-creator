#include "ReassignSocketPopup.hpp"

#include "src/Actions/ActionFunctions.hpp"
#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Widgets/StandardPopup.hpp"

#include <glm/vec2.hpp>
#include <imgui.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

class osc::ReassignSocketPopup::Impl final : public osc::StandardPopup {
public:
    Impl(std::string_view popupName,
         std::shared_ptr<UndoableModelStatePair> model,
         std::string_view componentAbsPath,
         std::string_view socketName) :

        StandardPopup{std::move(popupName)},
        m_Model{std::move(model)},
        m_ComponentPath{std::string{componentAbsPath}},
        m_SocketName{std::move(socketName)}
    {
    }

private:
    void implDraw() override
    {
        osc::InputString("search", m_Search, 128);

        ImGui::TextUnformatted("components:");

        OpenSim::Component const* chosenComponent = nullptr;

        ImGui::BeginChild("##componentlist", ImVec2(512, 256), true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);

        int id = 0;
        for (OpenSim::Component const& c : m_Model->getModel().getComponentList())
        {
            std::string const& name = c.getName();

            if (name.find(m_Search) != std::string::npos)
            {
                ImGui::PushID(id++);
                if (ImGui::Selectable(name.c_str()))
                {
                    chosenComponent = &c;
                }
                ImGui::PopID();
            }
        }
        ImGui::EndChild();

        if (!m_Error.empty())
        {
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
            ImGui::TextWrapped("%s", m_Error.c_str());
        }

        if (ImGui::Button("Cancel"))
        {
            requestClose();
        }

        if (chosenComponent && osc::ActionReassignSelectedComponentSocket(*m_Model, m_ComponentPath, m_SocketName, *chosenComponent, m_Error))
        {
            requestClose();
        }
    }

    void implOnClose() override
    {
        m_Search.clear();
        m_Error.clear();
    }

    std::shared_ptr<UndoableModelStatePair> m_Model;
    OpenSim::ComponentPath m_ComponentPath;
    std::string m_SocketName;
    std::string m_Search;
    std::string m_Error;
};


// public API (PIMPL)

osc::ReassignSocketPopup::ReassignSocketPopup(
    std::string_view popupName,
    std::shared_ptr<UndoableModelStatePair> model,
    std::string_view componentAbsPath,
    std::string_view socketName) :

    m_Impl{new Impl{std::move(popupName), std::move(model), std::move(componentAbsPath), std::move(socketName)}}
{
}

osc::ReassignSocketPopup::ReassignSocketPopup(ReassignSocketPopup&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::ReassignSocketPopup& osc::ReassignSocketPopup::operator=(ReassignSocketPopup&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::ReassignSocketPopup::~ReassignSocketPopup() noexcept
{
    delete m_Impl;
}

bool osc::ReassignSocketPopup::isOpen() const
{
    return m_Impl->isOpen();
}

void osc::ReassignSocketPopup::open()
{
    m_Impl->open();
}

void osc::ReassignSocketPopup::close()
{
    m_Impl->close();
}

void osc::ReassignSocketPopup::draw()
{
    m_Impl->draw();
}
