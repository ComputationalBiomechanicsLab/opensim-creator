#pragma once

#include <oscar/UI/Panels/IPanel.h>
#include <oscar/Utils/CStringView.h>

#include <memory>
#include <string>
#include <string_view>

namespace OpenSim { class ComponentPath; }
namespace osc { class IEditorAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ModelMusclePlotPanel final : public IPanel {
    public:
        ModelMusclePlotPanel(
            IEditorAPI*,
            std::shared_ptr<UndoableModelStatePair>,
            std::string_view panelName
        );
        ModelMusclePlotPanel(
            IEditorAPI*,
            std::shared_ptr<UndoableModelStatePair>,
            std::string_view panelName,
            OpenSim::ComponentPath const& coordPath,
            OpenSim::ComponentPath const& musclePath
        );
        ModelMusclePlotPanel(ModelMusclePlotPanel const&) = delete;
        ModelMusclePlotPanel(ModelMusclePlotPanel&&) noexcept;
        ModelMusclePlotPanel& operator=(ModelMusclePlotPanel const&) = delete;
        ModelMusclePlotPanel& operator=(ModelMusclePlotPanel&&) noexcept;
        ~ModelMusclePlotPanel() noexcept;

    private:
        CStringView impl_get_name() const;
        bool impl_is_open() const;
        void impl_open();
        void impl_close();
        void impl_on_draw();

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
