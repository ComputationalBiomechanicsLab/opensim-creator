#pragma once

#include <oscar/UI/Panels/IPanel.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <memory>
#include <string_view>

namespace osc { class MainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class SimulatorUIAPI; }

namespace osc
{
    class OutputPlotsPanel final : public IPanel {
    public:
        OutputPlotsPanel(
            std::string_view panelName,
            ParentPtr<MainUIStateAPI> const&,
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
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
