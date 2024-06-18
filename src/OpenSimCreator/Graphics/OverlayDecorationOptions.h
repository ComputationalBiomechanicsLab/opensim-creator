#pragma once

#include <OpenSimCreator/Graphics/OverlayDecorationOptionFlags.h>

#include <oscar/Platform/AppSettingValue.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>
#include <cstdint>
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

        void forEachOptionAsAppSettingValue(std::function<void(std::string_view, const AppSettingValue&)> const&) const;
        void tryUpdFromValues(std::string_view keyPrefix, std::unordered_map<std::string, AppSettingValue> const&);

        friend bool operator==(const OverlayDecorationOptions&, const OverlayDecorationOptions&) = default;

    private:
        OverlayDecorationOptionFlags m_Flags = OverlayDecorationOptionFlags::Default;
    };


}
