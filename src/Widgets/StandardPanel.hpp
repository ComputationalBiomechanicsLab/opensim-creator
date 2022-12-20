#pragma once

#include "src/Widgets/Panel.hpp"

#include <imgui.h>

#include <string>
#include <string_view>

namespace osc
{
    // a "standard" implementation for a Panel
    class StandardPanel : public Panel {
    protected:
        explicit StandardPanel(std::string_view name);
        StandardPanel(std::string_view name, ImGuiWindowFlags);
        StandardPanel(StandardPanel const&) = default;
        StandardPanel(StandardPanel&&) noexcept = default;
        StandardPanel& operator=(StandardPanel const&) = default;
        StandardPanel& operator=(StandardPanel&&) noexcept = default;
    public:
        virtual ~StandardPanel() noexcept = default;

    protected:
        void requestClose();

    private:
        // this standard implementation supplies these
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        void implDraw() final;

        // inheritors can/must provide these
        virtual void implBeforeImGuiBegin()
        {
        }
        virtual void implAfterImGuiBegin()
        {
        }
        virtual void implDrawContent() = 0;

        std::string m_PanelName;
        ImGuiWindowFlags m_PanelFlags;
    };
}