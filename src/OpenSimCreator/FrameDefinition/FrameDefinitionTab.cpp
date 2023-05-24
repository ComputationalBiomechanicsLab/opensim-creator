#include "FrameDefinitionTab.hpp"

#include "OpenSimCreator/MiddlewareAPIs/EditorAPI.hpp"
#include "OpenSimCreator/Panels/ModelEditorViewerPanel.hpp"
#include "OpenSimCreator/Panels/PropertiesPanel.hpp"
#include "OpenSimCreator/UndoableModelStatePair.hpp"
#include "OpenSimCreator/Widgets/MainMenu.hpp"

#include <oscar/Panels/LogViewerPanel.hpp>
#include <oscar/Panels/Panel.hpp>
#include <oscar/Panels/PanelManager.hpp>
#include <oscar/Panels/StandardPanel.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>
#include <oscar/Widgets/Popup.hpp>
#include <oscar/Widgets/PopupManager.hpp>
#include <oscar/Widgets/StandardPopup.hpp>
#include <oscar/Widgets/WindowMenu.hpp>

#include <imgui.h>
#include <OpenSim/Common/ComponentPath.h>
#include <SDL_events.h>

#include <memory>
#include <string_view>
#include <utility>

namespace
{
    constexpr osc::CStringView c_TabStringID = "OpenSim/Experimental/FrameDefinition";
}

namespace osc
{
    class FrameDefinitionContextMenu final : public StandardPopup {
    public:
        FrameDefinitionContextMenu(
            std::string_view popupName_,
            std::shared_ptr<UndoableModelStatePair> model_,
            OpenSim::ComponentPath componentPath_) :

            StandardPopup{popupName_, {10.0f, 10.0f}, ImGuiWindowFlags_NoMove},
            m_Model{std::move(model_)},
            m_ComponentPath{std::move(componentPath_)}
        {
            setModal(false);
            OSC_ASSERT(m_Model != nullptr);
        }

    private:
        void implDrawContent() final
        {
            ImGui::Text("hello");
        }

        std::shared_ptr<UndoableModelStatePair> m_Model;
        OpenSim::ComponentPath m_ComponentPath;
    };

    class FrameDefinitionTabNavigatorPanel final : public StandardPanel {
    public:
        FrameDefinitionTabNavigatorPanel(std::string_view panelName_) :
            StandardPanel{panelName_}
        {
        }
    private:
        void implDrawContent() final
        {
            ImGui::Text("navigator todo");
        }
    };

    class FrameDefinitionTabMainMenu final {
    public:
        explicit FrameDefinitionTabMainMenu(
            std::shared_ptr<UndoableModelStatePair> model_,
            std::shared_ptr<PanelManager> panelManager_) :
            m_Model{std::move(model_)},
            m_WindowMenu{std::move(panelManager_)}
        {
        }

        void draw()
        {
            m_WindowMenu.draw();
            m_AboutMenu.draw();
        }

    private:
        std::shared_ptr<UndoableModelStatePair> m_Model;
        WindowMenu m_WindowMenu;
        MainMenuAboutTab m_AboutMenu;
    };
}

class osc::FrameDefinitionTab::Impl final : public EditorAPI {
public:

    Impl(std::weak_ptr<TabHost> parent_) :
        m_Parent{std::move(parent_)}
    {
        m_PanelManager->registerToggleablePanel(
            "Navigator",
            [this](std::string_view panelName)
            {
                return std::make_shared<FrameDefinitionTabNavigatorPanel>(
                    panelName
                );
            }
        );
        m_PanelManager->registerToggleablePanel(
            "Properties",
            [this](std::string_view panelName)
            {
                return std::make_shared<PropertiesPanel>(panelName, this, m_Model);
            }
        );
        m_PanelManager->registerToggleablePanel(
            "Log",
            [this](std::string_view panelName)
            {
                return std::make_shared<LogViewerPanel>(panelName);
            }
        );
        m_PanelManager->registerSpawnablePanel(
            "viewer",
            [this](std::string_view panelName)
            {
                return std::make_shared<ModelEditorViewerPanel>(
                    panelName,
                    m_Model,
                    [this](OpenSim::ComponentPath const& absPath)
                    {
                        pushPopup(std::make_unique<FrameDefinitionContextMenu>(
                            "##ContextMenu",
                            m_Model,
                            absPath
                        ));
                    }
                );
            },
            1
        );
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return c_TabStringID;
    }

    void onMount()
    {
        App::upd().makeMainEventLoopWaiting();
        m_PanelManager->onMount();
        m_PopupManager.onMount();
    }

    void onUnmount()
    {
        m_PanelManager->onUnmount();
        App::upd().makeMainEventLoopPolling();
    }

    bool onEvent(SDL_Event const&)
    {
        return false;
    }

    void onTick()
    {
        m_PanelManager->onTick();
    }

    void onDrawMainMenu()
    {
        m_MainMenu.draw();
    }

    void onDraw()
    {
        ImGui::DockSpaceOverViewport(
            ImGui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode
        );
        m_PanelManager->onDraw();
        m_PopupManager.draw();
    }

private:
    void implPushComponentContextMenuPopup(OpenSim::ComponentPath const&) final
    {
        // todo: custom context menu for this screen
    }

    void implPushPopup(std::unique_ptr<Popup> popup) final
    {
        popup->open();
        m_PopupManager.push_back(std::move(popup));
    }

    void implAddMusclePlot(OpenSim::Coordinate const&, OpenSim::Muscle const&)
    {
        // ignore: not applicable in this tab
    }

    std::shared_ptr<PanelManager> implGetPanelManager()
    {
        return m_PanelManager;
    }

    UID m_TabID;
    std::weak_ptr<TabHost> m_Parent;

    std::shared_ptr<UndoableModelStatePair> m_Model = std::make_shared<UndoableModelStatePair>();
    std::shared_ptr<PanelManager> m_PanelManager = std::make_shared<PanelManager>();
    PopupManager m_PopupManager;

    FrameDefinitionTabMainMenu m_MainMenu{m_Model, m_PanelManager};
};


// public API (PIMPL)

osc::CStringView osc::FrameDefinitionTab::id() noexcept
{
    return c_TabStringID;
}

osc::FrameDefinitionTab::FrameDefinitionTab(std::weak_ptr<TabHost> parent_) :
    m_Impl{std::make_unique<Impl>(std::move(parent_))}
{
}

osc::FrameDefinitionTab::FrameDefinitionTab(FrameDefinitionTab&&) noexcept = default;
osc::FrameDefinitionTab& osc::FrameDefinitionTab::operator=(FrameDefinitionTab&&) noexcept = default;
osc::FrameDefinitionTab::~FrameDefinitionTab() noexcept = default;

osc::UID osc::FrameDefinitionTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::FrameDefinitionTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::FrameDefinitionTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::FrameDefinitionTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::FrameDefinitionTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::FrameDefinitionTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::FrameDefinitionTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::FrameDefinitionTab::implOnDraw()
{
    m_Impl->onDraw();
}
