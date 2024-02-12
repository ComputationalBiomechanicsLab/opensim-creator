#pragma once

#include <oscar/UI/Panels/IPanel.h>
#include <oscar/Utils/CStringView.h>

#include <imgui.h>

#include <string>
#include <string_view>

namespace osc
{
    // a "standard" implementation for an IPanel
    class StandardPanelImpl : public IPanel {
    protected:
        explicit StandardPanelImpl(std::string_view panelName);
        StandardPanelImpl(std::string_view panelName, ImGuiWindowFlags);
        StandardPanelImpl(StandardPanelImpl const&) = default;
        StandardPanelImpl(StandardPanelImpl&&) noexcept = default;
        StandardPanelImpl& operator=(StandardPanelImpl const&) = default;
        StandardPanelImpl& operator=(StandardPanelImpl&&) noexcept = default;
    public:
        ~StandardPanelImpl() noexcept override = default;

    protected:
        void requestClose();

    private:
        // this standard implementation supplies these
        CStringView implGetName() const final;
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        void implOnDraw() final;

        // inheritors can/must provide these
        virtual void implBeforeImGuiBegin() {}
        virtual void implAfterImGuiBegin() {}
        virtual void implDrawContent() = 0;

        std::string m_PanelName;
        ImGuiWindowFlags m_PanelFlags;
    };
}
