#pragma once

#include <libopensimcreator/Graphics/OverlayDecorationOptionFlags.h>

#include <liboscar/utils/CStringView.h>
#include <liboscar/variant/variant.h>

#include <cstddef>
#include <functional>
#include <string_view>
#include <unordered_map>

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

        void forEachOptionAsAppSettingValue(const std::function<void(std::string_view, const Variant&)>&) const;
        void tryUpdFromValues(std::string_view keyPrefix, const std::unordered_map<std::string, Variant>&);

        friend bool operator==(const OverlayDecorationOptions&, const OverlayDecorationOptions&) = default;

    private:
        OverlayDecorationOptionFlags m_Flags = OverlayDecorationOptionFlags::Default;
    };


}
