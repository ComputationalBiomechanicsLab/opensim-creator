#pragma once

#include "src/Widgets/VirtualPopup.hpp"

#include <memory>
#include <string_view>

namespace OpenSim { class Component; }
namespace osc { class EditorAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class AddComponentPopup final : public VirtualPopup {
    public:
        AddComponentPopup(
            EditorAPI*,
            std::shared_ptr<UndoableModelStatePair>,
            std::unique_ptr<OpenSim::Component> prototype,
            std::string_view popupName);
        AddComponentPopup(AddComponentPopup const&) = delete;
        AddComponentPopup(AddComponentPopup&&) noexcept;
        AddComponentPopup& operator=(AddComponentPopup const&) = delete;
        AddComponentPopup& operator=(AddComponentPopup&&) noexcept;
        ~AddComponentPopup() noexcept;

        bool isOpen() const override;
        void open() override;
        void close() override;
        bool beginPopup() override;
        void drawPopupContent() override;
        void endPopup() override;

    private:
        class Impl;
        Impl* m_Impl;
    };
}
