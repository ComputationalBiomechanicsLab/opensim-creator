#pragma once

#include <oscar/Panels/Panel.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <memory>
#include <string_view>

namespace osc { class SimulatorUIAPI; }

namespace osc
{
    class SelectionDetailsPanel final : public Panel {
    public:
        SelectionDetailsPanel(
            std::string_view panelName,
            SimulatorUIAPI*
        );
        SelectionDetailsPanel(SelectionDetailsPanel const&) = delete;
        SelectionDetailsPanel(SelectionDetailsPanel&&) noexcept;
        SelectionDetailsPanel& operator=(SelectionDetailsPanel const&) = delete;
        SelectionDetailsPanel& operator=(SelectionDetailsPanel&&) noexcept;
        ~SelectionDetailsPanel() noexcept;

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
