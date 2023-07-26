#pragma once

#include <oscar/Panels/Panel.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <memory>
#include <string>
#include <string_view>

namespace OpenSim { class ComponentPath; }
namespace osc { class EditorAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ModelMusclePlotPanel final : public Panel {
    public:
        ModelMusclePlotPanel(
            EditorAPI*,
            std::shared_ptr<UndoableModelStatePair>,
            std::string_view panelName
        );
        ModelMusclePlotPanel(
            EditorAPI*,
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
        CStringView implGetName() const;
        bool implIsOpen() const;
        void implOpen();
        void implClose();
        void implOnDraw();

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
