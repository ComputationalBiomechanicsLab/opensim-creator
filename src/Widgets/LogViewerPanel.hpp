#pragma once

#include "src/Widgets/VirtualPanel.hpp"

#include <string_view>

namespace osc
{
    class LogViewerPanel final : public VirtualPanel {
    public:
        LogViewerPanel(std::string_view panelName);
        LogViewerPanel(LogViewerPanel const&) = delete;
        LogViewerPanel(LogViewerPanel&&) noexcept;
        LogViewerPanel& operator=(LogViewerPanel const&) = delete;
        LogViewerPanel& operator=(LogViewerPanel&&) noexcept;
        ~LogViewerPanel() noexcept;

    private:
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        void implDraw() final;

        class Impl;
        Impl* m_Impl;
    };
}
