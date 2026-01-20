#pragma once

#include <libopynsim/documents/output_extractors/output_extractor.h>
#include <libopynsim/documents/output_extractors/output_extractor_data_type.h>
#include <libopynsim/documents/output_extractors/output_value_extractor.h>

#include <liboscar/utils/c_string_view.h>
#include <liboscar/variant/variant.h>

#include <cstddef>
#include <string>
#include <string_view>

namespace osc
{
    // an `OutputExtractor` that always emits the same value
    class ConstantOutputExtractor final : public OutputExtractor {
    public:
        ConstantOutputExtractor(std::string_view name, float value) :
            m_Name{name},
            m_Value{value},
            m_Type{OutputExtractorDataType::Float}
        {}

        ConstantOutputExtractor(std::string_view name, Vector2 value) :
            m_Name{name},
            m_Value{value},
            m_Type{OutputExtractorDataType::Vector2}
        {}

        friend bool operator==(const ConstantOutputExtractor&, const ConstantOutputExtractor&) = default;
    private:
        CStringView implGetName() const override { return m_Name; }
        CStringView implGetDescription() const override { return {}; }
        OutputExtractorDataType implGetOutputType() const override { return m_Type; }
        OutputValueExtractor implGetOutputValueExtractor(const OpenSim::Component&) const override;
        size_t implGetHash() const override;
        bool implEquals(const OutputExtractor&) const override;

        std::string m_Name;
        Variant m_Value;
        OutputExtractorDataType m_Type;
    };
}
