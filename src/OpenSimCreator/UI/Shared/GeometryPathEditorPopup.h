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
            std::function<void(const OpenSim::GeometryPath&)> onLocalCopyEdited_
        );
        GeometryPathEditorPopup(const GeometryPathEditorPopup&) = delete;
        GeometryPathEditorPopup(GeometryPathEditorPopup&&) noexcept;
        GeometryPathEditorPopup& operator=(const GeometryPathEditorPopup&) = delete;
        GeometryPathEditorPopup& operator=(GeometryPathEditorPopup&&) noexcept;
        ~GeometryPathEditorPopup() noexcept;

    private:
        bool impl_is_open() const final;
        void impl_open() final;
        void impl_close() final;
        bool impl_begin_popup() final;
        void impl_on_draw() final;
        void impl_end_popup() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
