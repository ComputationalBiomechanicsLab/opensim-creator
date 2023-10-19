#pragma once

namespace osc
{
    class PropertyDescription final {
    private:
        std::string m_Name;
        VariantType m_Type;
        Variant m_Default;
    };
}
