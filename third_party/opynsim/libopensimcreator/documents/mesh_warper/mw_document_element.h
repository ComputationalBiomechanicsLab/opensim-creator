#pragma once

#include <liboscar/utilities/c_string_view.h>

namespace osc { class StringName; }

namespace osc
{
    /// Represents a single named element of the mesh warper document.
    class MwDocumentElement {
    protected:
        MwDocumentElement() = default;
        MwDocumentElement(const MwDocumentElement&) = default;
        MwDocumentElement(MwDocumentElement&&) noexcept = default;
        MwDocumentElement& operator=(const MwDocumentElement&) = default;
        MwDocumentElement& operator=(MwDocumentElement&&) noexcept = default;
    public:
        virtual ~MwDocumentElement() noexcept = default;

        CStringView getName() const { return implGetName(); }
    private:
        virtual CStringView implGetName() const = 0;
    };
}
