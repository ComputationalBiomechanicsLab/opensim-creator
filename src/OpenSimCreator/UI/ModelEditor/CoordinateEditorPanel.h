#pragma once

#include <oscar/UI/Panels/IPanel.h>
#include <oscar/Utils/CStringView.h>

#include <memory>
#include <string_view>

namespace osc { class IEditorAPI; }
namespace osc { class IMainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class CoordinateEditorPanel final : public IPanel {
    public:
        CoordinateEditorPanel(
            std::string_view panelName,
            ParentPtr<IMainUIStateAPI> const&,
            IEditorAPI*,
            std::shared_ptr<UndoableModelStatePair>
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
