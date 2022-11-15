#pragma once

#include "src/Widgets/VirtualPanel.hpp"

#include <string>
#include <string_view>

namespace osc
{
    // a "standard" implementation for a VirtualPanel
    class StandardPanel : public VirtualPanel {
    protected:
        explicit StandardPanel(std::string_view name);
        StandardPanel(std::string_view name, int imGuiWindowFlags);
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
        int m_PanelFlags;
    };
}