#pragma once

#include <liboscar/utilities/c_string_view.h>

namespace osc
{
    class StringName;

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

        CStringView get_name() const { return impl_get_name(); }

    private:
        virtual CStringView impl_get_name() const = 0;
    };
}
