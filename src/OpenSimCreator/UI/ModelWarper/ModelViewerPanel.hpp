#pragma once

#include <oscar/UI/Panels/IPanel.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <memory>
#include <string_view>

namespace osc::mow { class UIState; }

namespace osc::mow
{
    class ModelViewerPanel final : public IPanel {
    public:
        ModelViewerPanel(
            std::string_view panelName_,
            std::shared_ptr<UIState> state_);
        ModelViewerPanel(ModelViewerPanel const&) = delete;
        ModelViewerPanel(ModelViewerPanel&&) noexcept;
        ModelViewerPanel& operator=(ModelViewerPanel const&) = delete;
        ModelViewerPanel& operator=(ModelViewerPanel&&) noexcept;
        ~ModelViewerPanel() noexcept;

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
