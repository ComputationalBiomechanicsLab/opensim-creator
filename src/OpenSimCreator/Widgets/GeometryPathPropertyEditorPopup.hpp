#pragma once

#include "OpenSimCreator/Model/ObjectPropertyEdit.hpp"

#include <oscar/Widgets/Popup.hpp>

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class GeometryPath; }
namespace OpenSim { template<typename TObject>class ObjectProperty; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{

    class GeometryPathPropertyEditorPopup final : public Popup {
    public:
        GeometryPathPropertyEditorPopup(
            std::string_view popupName_,
            std::shared_ptr<UndoableModelStatePair const> targetModel_,
            std::function<OpenSim::ObjectProperty<OpenSim::GeometryPath> const*()> accessor_,
            std::function<void(ObjectPropertyEdit)> onEditCallback_
        );
        GeometryPathPropertyEditorPopup(GeometryPathPropertyEditorPopup const&) = delete;
        GeometryPathPropertyEditorPopup(GeometryPathPropertyEditorPopup&&) noexcept;
        GeometryPathPropertyEditorPopup& operator=(GeometryPathPropertyEditorPopup const&) = delete;
        GeometryPathPropertyEditorPopup& operator=(GeometryPathPropertyEditorPopup&&) noexcept;
        ~GeometryPathPropertyEditorPopup() noexcept;

    private:
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        bool implBeginPopup() final;
        void implDrawPopupContent() final;
        void implEndPopup() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}