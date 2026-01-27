#pragma once

#include <libopynsim/graphics/custom_rendering_option_flags.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/variant/variant.h>

#include <cstddef>
#include <functional>
#include <string_view>
#include <unordered_map>

namespace osc { struct SceneRendererParams; }

namespace opyn
{
    class CustomRenderingOptions final {
    public:
        size_t getNumOptions() const;
        bool getOptionValue(ptrdiff_t) const;
        void setOptionValue(ptrdiff_t, bool);
        osc::CStringView getOptionLabel(ptrdiff_t) const;

        bool getDrawFloor() const;
        void setDrawFloor(bool);

        bool getDrawMeshNormals() const;
        void setDrawMeshNormals(bool);

        bool getDrawShadows() const;
        void setDrawShadows(bool);

        bool getDrawSelectionRims() const;
        void setDrawSelectionRims(bool);

        bool getOrderIndependentTransparency() const;
        void setOrderIndependentTransparency(bool);

        void forEachOptionAsAppSettingValue(const std::function<void(std::string_view, const osc::Variant&)>&) const;
        void tryUpdFromValues(std::string_view keyPrefix, const std::unordered_map<std::string, osc::Variant>&);

        void applyTo(osc::SceneRendererParams&) const;

        friend bool operator==(const CustomRenderingOptions&, const CustomRenderingOptions&) = default;

    private:
        CustomRenderingOptionFlags m_Flags = CustomRenderingOptionFlags::Default;
    };
}
