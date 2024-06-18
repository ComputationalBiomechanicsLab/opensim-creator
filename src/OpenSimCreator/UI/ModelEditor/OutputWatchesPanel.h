#pragma once

#include <oscar/UI/Panels/IPanel.h>
#include <oscar/Utils/CStringView.h>

#include <memory>
#include <string_view>

namespace osc { class IMainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class OutputWatchesPanel final : public IPanel {
    public:
        OutputWatchesPanel(
            std::string_view panelName,
            std::shared_ptr<UndoableModelStatePair const>,
            ParentPtr<IMainUIStateAPI> const&
        );
        OutputWatchesPanel(const OutputWatchesPanel&) = delete;
        OutputWatchesPanel(OutputWatchesPanel&&) noexcept;
        OutputWatchesPanel& operator=(const OutputWatchesPanel&) = delete;
        OutputWatchesPanel& operator=(OutputWatchesPanel&&) noexcept;
        ~OutputWatchesPanel() noexcept;

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
