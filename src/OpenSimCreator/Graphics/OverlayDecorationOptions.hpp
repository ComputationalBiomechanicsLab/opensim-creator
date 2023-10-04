#pragma once

#include <OpenSimCreator/Graphics/OverlayDecorationOptionFlags.hpp>

#include <oscar/Utils/CStringView.hpp>

#include <cstddef>
#include <cstdint>

namespace osc
{
    class OverlayDecorationOptions final {
    public:
        size_t getNumOptions() const;
        bool getOptionValue(ptrdiff_t) const;
        void setOptionValue(ptrdiff_t, bool);
        CStringView getOptionLabel(ptrdiff_t) const;
        CStringView getOptionGroupLabel(ptrdiff_t) const;

        bool getDrawXZGrid() const;
        void setDrawXZGrid(bool);

        bool getDrawXYGrid() const;
        void setDrawXYGrid(bool);

        bool getDrawYZGrid() const;
        void setDrawYZGrid(bool);

        bool getDrawAxisLines() const;
        void setDrawAxisLines(bool);

        bool getDrawAABBs() const;
        void setDrawAABBs(bool);

        bool getDrawBVH() const;
        void setDrawBVH(bool);

    private:
        friend bool operator==(OverlayDecorationOptions const&, OverlayDecorationOptions const&) noexcept;
        friend bool operator!=(OverlayDecorationOptions const&, OverlayDecorationOptions const&) noexcept;

        OverlayDecorationOptionFlags m_Flags = OverlayDecorationOptionFlags::Default;
    };

    inline bool operator==(OverlayDecorationOptions const& a, OverlayDecorationOptions const& b) noexcept
    {
        return a.m_Flags == b.m_Flags;
    }

    inline bool operator!=(OverlayDecorationOptions const& a, OverlayDecorationOptions const& b) noexcept
    {
        return !(a == b);
    }
}
