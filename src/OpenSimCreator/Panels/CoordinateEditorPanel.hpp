#pragma once

#include <oscar/Panels/Panel.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <memory>
#include <string_view>

namespace osc { class EditorAPI; }
namespace osc { class MainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class CoordinateEditorPanel final : public Panel {
    public:
        CoordinateEditorPanel(
            std::string_view panelName,
            ParentPtr<MainUIStateAPI> const&,
            EditorAPI*,
            std::shared_ptr<UndoableModelStatePair>
        );
        CoordinateEditorPanel(CoordinateEditorPanel const&) = delete;
        CoordinateEditorPanel(CoordinateEditorPanel&&) noexcept;
        CoordinateEditorPanel& operator=(CoordinateEditorPanel const&) = delete;
        CoordinateEditorPanel& operator=(CoordinateEditorPanel&&) noexcept;
        ~CoordinateEditorPanel() noexcept;

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
