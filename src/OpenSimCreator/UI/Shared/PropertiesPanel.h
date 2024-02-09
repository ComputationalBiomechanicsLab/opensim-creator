#pragma once

#include <oscar/UI/Panels/IPanel.h>
#include <oscar/Utils/CStringView.h>

#include <string_view>
#include <memory>

namespace osc { class IEditorAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class PropertiesPanel final : public IPanel {
    public:
        PropertiesPanel(
            std::string_view panelName,
            IEditorAPI*,
            std::shared_ptr<UndoableModelStatePair>
        );
        PropertiesPanel(PropertiesPanel const&) = delete;
        PropertiesPanel(PropertiesPanel&&) noexcept;
        PropertiesPanel& operator=(PropertiesPanel const&) = delete;
        PropertiesPanel& operator=(PropertiesPanel&&) noexcept;
        ~PropertiesPanel() noexcept;

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
