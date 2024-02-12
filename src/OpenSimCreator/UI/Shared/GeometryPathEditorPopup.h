#pragma once

#include <oscar/UI/Widgets/IPopup.h>

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class GeometryPath; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{

    class GeometryPathEditorPopup final : public IPopup {
    public:
        GeometryPathEditorPopup(
            std::string_view popupName_,
            std::shared_ptr<UndoableModelStatePair const> targetModel_,
            std::function<OpenSim::GeometryPath const*()> geometryPathGetter_,
            std::function<void(OpenSim::GeometryPath const&)> onLocalCopyEdited_
        );
        GeometryPathEditorPopup(GeometryPathEditorPopup const&) = delete;
        GeometryPathEditorPopup(GeometryPathEditorPopup&&) noexcept;
        GeometryPathEditorPopup& operator=(GeometryPathEditorPopup const&) = delete;
        GeometryPathEditorPopup& operator=(GeometryPathEditorPopup&&) noexcept;
        ~GeometryPathEditorPopup() noexcept;

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
