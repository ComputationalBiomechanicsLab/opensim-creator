#pragma once

#include <memory>
#include <string_view>

namespace osc { class MainUIStateAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class OutputWatchesPanel final {
    public:
        OutputWatchesPanel(std::string_view panelName, std::shared_ptr<UndoableModelStatePair>, MainUIStateAPI*);
        OutputWatchesPanel(OutputWatchesPanel const&) = delete;
        OutputWatchesPanel(OutputWatchesPanel&&) noexcept;
        OutputWatchesPanel& operator=(OutputWatchesPanel const&) = delete;
        OutputWatchesPanel& operator=(OutputWatchesPanel&&) noexcept;
        ~OutputWatchesPanel() noexcept;

        bool isOpen() const;
        void open();
        void close();
        bool draw();

    private:
        class Impl;
        Impl* m_Impl;
    };
}