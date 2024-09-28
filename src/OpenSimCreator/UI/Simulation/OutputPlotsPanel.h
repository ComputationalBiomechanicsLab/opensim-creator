#pragma once

#include <oscar/UI/Panels/IPanel.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/ParentPtr.h>

#include <memory>
#include <string_view>

namespace osc { class IMainUIStateAPI; }
namespace osc { class ISimulatorUIAPI; }

namespace osc
{
    class OutputPlotsPanel final : public IPanel {
    public:
        OutputPlotsPanel(
            std::string_view panelName,
            const ParentPtr<IMainUIStateAPI>&,
            ISimulatorUIAPI*
        );
        OutputPlotsPanel(const OutputPlotsPanel&) = delete;
        OutputPlotsPanel(OutputPlotsPanel&&) noexcept;
        OutputPlotsPanel& operator=(const OutputPlotsPanel&) = delete;
        OutputPlotsPanel& operator=(OutputPlotsPanel&&) noexcept;
        ~OutputPlotsPanel() noexcept;

    private:
        CStringView impl_get_name() const final;
        bool impl_is_open() const final;
        void impl_open() final;
        void impl_close() final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
