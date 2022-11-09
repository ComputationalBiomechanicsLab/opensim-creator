#pragma once

#include "src/Widgets/VirtualPanel.hpp"

#include <string>
#include <string_view>

namespace osc
{
    // base class for implementing a panel that has a name
    class NamedPanel : public VirtualPanel {
    protected:
        explicit NamedPanel(std::string_view name);
        NamedPanel(std::string_view name, int imGuiWindowFlags);
        NamedPanel(NamedPanel const&) = default;
        NamedPanel(NamedPanel&&) noexcept = default;
        NamedPanel& operator=(NamedPanel const&) = default;
        NamedPanel& operator=(NamedPanel&&) noexcept = default;
    public:
        virtual ~NamedPanel() noexcept = default;

    protected:
        void requestClose();

    private:
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        void implDraw() final;
        virtual void implBeforeImGuiBegin() {}
        virtual void implAfterImGuiBegin() {}
        virtual void implDrawContent() = 0;

        std::string m_PanelName;
        int m_PanelFlags;
    };
}