#pragma once

#include "oscar/Panels/Panel.hpp"
#include "oscar/Utils/CStringView.hpp"

#include <memory>
#include <string_view>

namespace osc
{
    class LogViewerPanel final : public Panel {
    public:
        explicit LogViewerPanel(std::string_view panelName);
        LogViewerPanel(LogViewerPanel const&) = delete;
        LogViewerPanel(LogViewerPanel&&) noexcept;
        LogViewerPanel& operator=(LogViewerPanel const&) = delete;
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
