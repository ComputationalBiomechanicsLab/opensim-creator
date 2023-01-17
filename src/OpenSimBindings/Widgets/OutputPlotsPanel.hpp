#pragma once

#include <memory>
#include <string_view>

namespace osc { class MainUIStateAPI; }
namespace osc { class SimulatorUIAPI; }

namespace osc
{
    class OutputPlotsPanel final {
    public:
        OutputPlotsPanel(
            std::string_view panelName,
            MainUIStateAPI* mainUIStateAPI,
            SimulatorUIAPI* simulatorUIAPI
        );
        OutputPlotsPanel(OutputPlotsPanel const&) = delete;
        OutputPlotsPanel(OutputPlotsPanel&&) noexcept;
        OutputPlotsPanel& operator=(OutputPlotsPanel const&) = delete;
        OutputPlotsPanel& operator=(OutputPlotsPanel&&) noexcept;
        ~OutputPlotsPanel() noexcept;

        void draw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}