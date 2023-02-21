#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc { class MainUIStateAPI; }
namespace osc { class TabHost; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ModelEditorTab final : public Tab {
    public:
        ModelEditorTab(MainUIStateAPI*, std::unique_ptr<UndoableModelStatePair>);
        ModelEditorTab(ModelEditorTab const&) = delete;
        ModelEditorTab(ModelEditorTab&&) noexcept;
        ModelEditorTab& operator=(ModelEditorTab const&) = delete;
        ModelEditorTab& operator=(ModelEditorTab&&) noexcept;
        ~ModelEditorTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        bool implIsUnsaved() const final;
        bool implTrySave() final;
        void implOnMount() final;
        void implOnUnmount() final;
        bool implOnEvent(SDL_Event const&) final;
        void implOnTick() final;
        void implOnDrawMainMenu() final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
