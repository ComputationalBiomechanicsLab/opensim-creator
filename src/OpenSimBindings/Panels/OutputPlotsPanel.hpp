#pragma once

#include "src/Panels/Panel.hpp"
#include "src/Utils/CStringView.hpp"

#include <memory>
#include <string_view>

namespace osc { class MainUIStateAPI; }
namespace osc { class SimulatorUIAPI; }

namespace osc
{
    class OutputPlotsPanel final : public Panel {
    public:
        OutputPlotsPanel(
            std::string_view panelName,
            std::weak_ptr<MainUIStateAPI>,
            SimulatorUIAPI*
        );
        OutputPlotsPanel(OutputPlotsPanel const&) = delete;
        OutputPlotsPanel(OutputPlotsPanel&&) noexcept;
        OutputPlotsPanel& operator=(OutputPlotsPanel const&) = delete;
        OutputPlotsPanel& operator=(OutputPlotsPanel&&) noexcept;
        ~OutputPlotsPanel() noexcept;

    private:
        CStringView implGetName() const final;
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        void implDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}