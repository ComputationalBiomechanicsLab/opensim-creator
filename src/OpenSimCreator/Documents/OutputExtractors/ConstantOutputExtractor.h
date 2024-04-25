#pragma once

#include <OpenSimCreator/Documents/OutputExtractors/IOutputExtractor.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractorDataType.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputValueExtractor.h>

#include <oscar/Utils/CStringView.h>
#include <oscar/Variant/Variant.h>

#include <cstddef>
#include <string>
#include <string_view>

namespace osc
{
    // an `IOutputExtractor` that always emits the same value
    class ConstantOutputExtractor final : public IOutputExtractor {
    public:
        ConstantOutputExtractor(std::string_view name, float value) :
            m_Name{name},
            m_Value{value},
            m_Type{OutputExtractorDataType::Float}
        {}

        ConstantOutputExtractor(std::string_view name, Vec2 value) :
            m_Name{name},
            m_Value{value},
            m_Type{OutputExtractorDataType::Vec2}
        {}

        friend bool operator==(const ConstantOutputExtractor&, const ConstantOutputExtractor&) = default;
    private:
        CStringView implGetName() const override { return m_Name; }
        CStringView implGetDescription() const override { return {}; }
        OutputExtractorDataType implGetOutputType() const override { return m_Type; }
        OutputValueExtractor implGetOutputValueExtractor(OpenSim::Component const&) const override;
        size_t implGetHash() const override;
        bool implEquals(IOutputExtractor const&) const override;

        std::string m_Name;
        Variant m_Value;
        OutputExtractorDataType m_Type;
    };
}
