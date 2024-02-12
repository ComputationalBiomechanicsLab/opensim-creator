#pragma once

#include <oscar/UI/Panels/IPanel.h>
#include <oscar/Utils/CStringView.h>

#include <memory>
#include <string_view>

namespace osc { class IMainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ISimulatorUIAPI; }

namespace osc
{
    class OutputPlotsPanel final : public IPanel {
    public:
        OutputPlotsPanel(
            std::string_view panelName,
            ParentPtr<IMainUIStateAPI> const&,
            ISimulatorUIAPI*
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
