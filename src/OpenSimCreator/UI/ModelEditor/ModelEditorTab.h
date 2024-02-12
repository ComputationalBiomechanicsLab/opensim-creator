#pragma once

#include <SDL_events.h>
#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <memory>

namespace osc { class IMainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ModelEditorTab final : public ITab {
    public:
        ModelEditorTab(
            ParentPtr<IMainUIStateAPI> const&,
            std::unique_ptr<UndoableModelStatePair>
        );
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
