#pragma once

#include <oscar/UI/Panels/IPanel.h>
#include <oscar/Utils/CStringView.h>

#include <memory>
#include <string_view>

namespace osc
{
    class LogViewerPanel final : public IPanel {
    public:
        explicit LogViewerPanel(std::string_view panelName);
        LogViewerPanel(const LogViewerPanel&) = delete;
        LogViewerPanel(LogViewerPanel&&) noexcept;
        LogViewerPanel& operator=(const LogViewerPanel&) = delete;
        LogViewerPanel& operator=(LogViewerPanel&&) noexcept;
        ~LogViewerPanel() noexcept;

    private:
        CStringView implGetName() const final;
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
