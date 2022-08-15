#pragma once

#include <memory>
#include <string_view>

namespace OpenSim { class Geometry; }

namespace osc
{
    class SelectGeometryPopup final {
    public:
        explicit SelectGeometryPopup(std::string_view popupName);
        SelectGeometryPopup(SelectGeometryPopup const&) = delete;
        SelectGeometryPopup(SelectGeometryPopup&&) noexcept;
        SelectGeometryPopup& operator=(SelectGeometryPopup const&) = delete;
        SelectGeometryPopup& operator=(SelectGeometryPopup&&) noexcept;
        ~SelectGeometryPopup() noexcept;

        void open();
        void close();

        // returns non-nullptr when user selects an OpenSim::Mesh
        std::unique_ptr<OpenSim::Geometry> draw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
