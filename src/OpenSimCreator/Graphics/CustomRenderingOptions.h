#pragma once

#include <OpenSimCreator/Graphics/CustomRenderingOptionFlags.h>

#include <oscar/Platform/AppSettingValue.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>
#include <unordered_map>

namespace osc
{
    class CustomRenderingOptions final {
    public:
        size_t getNumOptions() const;
        bool getOptionValue(ptrdiff_t) const;
        void setOptionValue(ptrdiff_t, bool);
        CStringView getOptionLabel(ptrdiff_t) const;

        bool getDrawFloor() const;
        void setDrawFloor(bool);

        bool getDrawMeshNormals() const;
        void setDrawMeshNormals(bool);

        bool getDrawShadows() const;
        void setDrawShadows(bool);

        bool getDrawSelectionRims() const;
        void setDrawSelectionRims(bool);

        void forEachOptionAsAppSettingValue(std::function<void(std::string_view, const AppSettingValue&)> const&) const;
        void tryUpdFromValues(std::string_view keyPrefix, std::unordered_map<std::string, AppSettingValue> const&);

        friend bool operator==(const CustomRenderingOptions&, const CustomRenderingOptions&) = default;

    private:
        CustomRenderingOptionFlags m_Flags = CustomRenderingOptionFlags::Default;
    };
}
