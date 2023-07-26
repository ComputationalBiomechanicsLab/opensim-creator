#pragma once

#include <oscar/Widgets/Popup.hpp>

#include <memory>
#include <string_view>

namespace OpenSim { class Component; }
namespace osc { class EditorAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class AddComponentPopup final : public Popup {
    public:
        AddComponentPopup(
            EditorAPI*,
            std::shared_ptr<UndoableModelStatePair>,
            std::unique_ptr<OpenSim::Component> prototype,
            std::string_view popupName
        );
        AddComponentPopup(AddComponentPopup const&) = delete;
        AddComponentPopup(AddComponentPopup&&) noexcept;
        AddComponentPopup& operator=(AddComponentPopup const&) = delete;
        AddComponentPopup& operator=(AddComponentPopup&&) noexcept;
        ~AddComponentPopup() noexcept;

    private:
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        bool implBeginPopup() final;
        void implOnDraw() final;
        void implEndPopup() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
