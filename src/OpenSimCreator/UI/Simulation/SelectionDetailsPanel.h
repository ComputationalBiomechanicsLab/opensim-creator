#pragma once

#include <oscar/UI/Panels/IPanel.h>
#include <oscar/Utils/CStringView.h>

#include <memory>
#include <string_view>

namespace osc { class ISimulatorUIAPI; }

namespace osc
{
    class SelectionDetailsPanel final : public IPanel {
    public:
        SelectionDetailsPanel(
            std::string_view panelName,
            ISimulatorUIAPI*
        );
        SelectionDetailsPanel(SelectionDetailsPanel const&) = delete;
        SelectionDetailsPanel(SelectionDetailsPanel&&) noexcept;
        SelectionDetailsPanel& operator=(SelectionDetailsPanel const&) = delete;
        SelectionDetailsPanel& operator=(SelectionDetailsPanel&&) noexcept;
        ~SelectionDetailsPanel() noexcept;

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
