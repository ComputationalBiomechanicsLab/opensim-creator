#pragma once

#include <string_view>
#include <memory>

namespace osc { class EditorAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class PropertiesPanel final {
    public:
        PropertiesPanel(
            std::string_view panelName,
            EditorAPI*,
            std::shared_ptr<UndoableModelStatePair>
        );
        PropertiesPanel(PropertiesPanel const&) = delete;
        PropertiesPanel(PropertiesPanel&&) noexcept;
        PropertiesPanel& operator=(PropertiesPanel const&) = delete;
        PropertiesPanel& operator=(PropertiesPanel&&) noexcept;
        ~PropertiesPanel() noexcept;

        void draw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
