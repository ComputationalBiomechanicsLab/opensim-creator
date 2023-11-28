#pragma once

namespace osc { class StringName; }

namespace osc
{
    class TPSDocumentElement {
    protected:
        TPSDocumentElement() = default;
        TPSDocumentElement(TPSDocumentElement const&) = default;
        TPSDocumentElement(TPSDocumentElement&&) noexcept = default;
        TPSDocumentElement& operator=(TPSDocumentElement const&) = default;
        TPSDocumentElement& operator=(TPSDocumentElement&&) noexcept = default;
    public:
        virtual ~TPSDocumentElement() noexcept = default;

        StringName const& getID() const
        {
            return implGetID();
        }
    private:
        virtual StringName const& implGetID() const = 0;
    };
}
