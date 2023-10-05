#pragma once

#include <OpenSimCreator/Graphics/CustomRenderingOptionFlags.hpp>

#include <oscar/Platform/AppSettingValue.hpp>
#include <oscar/Utils/CStringView.hpp>

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

        void forEachOptionAsAppSettingValue(std::function<void(std::string_view, AppSettingValue const&)> const&) const;
        void tryUpdFromValues(std::string_view keyPrefix, std::unordered_map<std::string, AppSettingValue> const&);

    private:
        friend bool operator==(CustomRenderingOptions const&, CustomRenderingOptions const&) noexcept;
        friend bool operator!=(CustomRenderingOptions const&, CustomRenderingOptions const&) noexcept;

        CustomRenderingOptionFlags m_Flags = CustomRenderingOptionFlags::Default;
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
