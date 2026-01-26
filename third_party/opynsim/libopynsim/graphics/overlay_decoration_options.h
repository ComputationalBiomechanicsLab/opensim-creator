#pragma once

#include <libopynsim/graphics/overlay_decoration_option_flags.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/variant/variant.h>

#include <cstddef>
#include <functional>
#include <string_view>
#include <unordered_map>

namespace opyn
{
    class OverlayDecorationOptions final {
    public:
        size_t getNumOptions() const;
        bool getOptionValue(ptrdiff_t) const;
        void setOptionValue(ptrdiff_t, bool);
        osc::CStringView getOptionLabel(ptrdiff_t) const;
        osc::CStringView getOptionGroupLabel(ptrdiff_t) const;

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

        void forEachOptionAsAppSettingValue(const std::function<void(std::string_view, const osc::Variant&)>&) const;
        void tryUpdFromValues(std::string_view keyPrefix, const std::unordered_map<std::string, osc::Variant>&);

        friend bool operator==(const OverlayDecorationOptions&, const OverlayDecorationOptions&) = default;

    private:
        OverlayDecorationOptionFlags m_Flags = OverlayDecorationOptionFlags::Default;
    };
}
