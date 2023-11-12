#pragma once

#include <OpenSimCreator/Graphics/OverlayDecorationOptionFlags.hpp>

#include <oscar/Platform/AppSettingValue.hpp>
#include <oscar/Utils/CStringView.hpp>

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

        void forEachOptionAsAppSettingValue(std::function<void(std::string_view, AppSettingValue const&)> const&) const;
        void tryUpdFromValues(std::string_view keyPrefix, std::unordered_map<std::string, AppSettingValue> const&);

        friend bool operator==(OverlayDecorationOptions const&, OverlayDecorationOptions const&) = default;

    private:
        OverlayDecorationOptionFlags m_Flags = OverlayDecorationOptionFlags::Default;
    };


}
