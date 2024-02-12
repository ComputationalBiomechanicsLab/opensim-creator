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
        OutputWatchesPanel(OutputWatchesPanel const&) = delete;
        OutputWatchesPanel(OutputWatchesPanel&&) noexcept;
        OutputWatchesPanel& operator=(OutputWatchesPanel const&) = delete;
        OutputWatchesPanel& operator=(OutputWatchesPanel&&) noexcept;
        ~OutputWatchesPanel() noexcept;

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
