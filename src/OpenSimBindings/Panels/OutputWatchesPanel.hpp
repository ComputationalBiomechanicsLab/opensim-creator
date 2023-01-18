#pragma once

#include "src/Panels/Panel.hpp"
#include "src/Utils/CStringView.hpp"

#include <memory>
#include <string_view>

namespace osc { class MainUIStateAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class OutputWatchesPanel final : public Panel {
    public:
        OutputWatchesPanel(std::string_view panelName, std::shared_ptr<UndoableModelStatePair>, MainUIStateAPI*);
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
        void implDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}