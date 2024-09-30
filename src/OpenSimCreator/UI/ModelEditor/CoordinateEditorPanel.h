#pragma once

#include <oscar/UI/Panels/IPanel.h>
#include <oscar/Utils/CStringView.h>

#include <memory>
#include <string_view>

namespace osc { class IEditorAPI; }
namespace osc { class IModelStatePair; }
namespace osc { class Widget; }

namespace osc
{
    class CoordinateEditorPanel final : public IPanel {
    public:
        CoordinateEditorPanel(
            std::string_view panelName,
            Widget&,
            IEditorAPI*,
            std::shared_ptr<IModelStatePair>
        );
        CoordinateEditorPanel(const CoordinateEditorPanel&) = delete;
        CoordinateEditorPanel(CoordinateEditorPanel&&) noexcept;
        CoordinateEditorPanel& operator=(const CoordinateEditorPanel&) = delete;
        CoordinateEditorPanel& operator=(CoordinateEditorPanel&&) noexcept;
        ~CoordinateEditorPanel() noexcept;

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
