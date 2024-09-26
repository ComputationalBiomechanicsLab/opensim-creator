#pragma once

#include <oscar/UI/Panels/IPanel.h>
#include <oscar/Utils/CStringView.h>

#include <string_view>
#include <memory>

namespace osc { class IEditorAPI; }
namespace osc { class IModelStatePair; }

namespace osc
{
    class PropertiesPanel final : public IPanel {
    public:
        PropertiesPanel(
            std::string_view panelName,
            IEditorAPI*,
            std::shared_ptr<IModelStatePair>
        );
        PropertiesPanel(const PropertiesPanel&) = delete;
        PropertiesPanel(PropertiesPanel&&) noexcept;
        PropertiesPanel& operator=(const PropertiesPanel&) = delete;
        PropertiesPanel& operator=(PropertiesPanel&&) noexcept;
        ~PropertiesPanel() noexcept;

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
