#pragma once

#include <oscar/Utils/CStringView.h>

namespace osc { class StringName; }

namespace osc
{
    class TPSDocumentElement {
    protected:
        TPSDocumentElement() = default;
        TPSDocumentElement(const TPSDocumentElement&) = default;
        TPSDocumentElement(TPSDocumentElement&&) noexcept = default;
        TPSDocumentElement& operator=(const TPSDocumentElement&) = default;
        TPSDocumentElement& operator=(TPSDocumentElement&&) noexcept = default;
    public:
        virtual ~TPSDocumentElement() noexcept = default;

        CStringView getName() const
        {
            return implGetName();
        }
    private:
        virtual CStringView implGetName() const = 0;
    };
}
