#pragma once

#include <oscar/Utils/CStringView.hpp>

#include <cstddef>
#include <cstdint>

namespace osc
{
    class CustomRenderingOptions final {
    public:
        CustomRenderingOptions();

        CStringView getGroupLabel(ptrdiff_t) const;

        size_t getNumOptions() const;
        bool getOptionValue(ptrdiff_t) const;
        void setOptionValue(ptrdiff_t, bool);
        CStringView getOptionLabel(ptrdiff_t) const;
        ptrdiff_t getOptionGroupIndex(ptrdiff_t) const;

        bool getDrawFloor() const;
        void setDrawFloor(bool);

        bool getDrawMeshNormals() const;
        void setDrawMeshNormals(bool);

        bool getDrawShadows() const;
        void setDrawShadows(bool);

        bool getDrawSelectionRims() const;
        void setDrawSelectionRims(bool);

    private:
        friend bool operator==(CustomRenderingOptions const&, CustomRenderingOptions const&) noexcept;
        friend bool operator!=(CustomRenderingOptions const&, CustomRenderingOptions const&) noexcept;

        uint32_t m_Flags;
    };

    inline bool operator==(CustomRenderingOptions const& a, CustomRenderingOptions const& b) noexcept
    {
        return a.m_Flags == b.m_Flags;
    }

    inline bool operator!=(CustomRenderingOptions const& a, CustomRenderingOptions const& b) noexcept
    {
        return !(a == b);
    }
}