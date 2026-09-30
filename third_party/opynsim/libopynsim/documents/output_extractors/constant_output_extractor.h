#pragma once

#include <libopynsim/documents/output_extractors/output_extractor.h>
#include <libopynsim/documents/output_extractors/output_extractor_data_type.h>
#include <libopynsim/documents/output_extractors/output_value_extractor.h>

#include <liboscar/utilities/c_string_view.h>
#include <liboscar/variant/variant.h>

#include <cstddef>
#include <string>
#include <string_view>

namespace opyn
{
    // an `OutputExtractor` that always emits the same value
    class ConstantOutputExtractor final : public OutputExtractor {
    public:
        ConstantOutputExtractor(std::string_view name, float value) :
            name_{name},
            value_{value},
            type_{OutputExtractorDataType::Float}
        {}

        ConstantOutputExtractor(std::string_view name, osc::Vector2 value) :
            name_{name},
            value_{value},
            type_{OutputExtractorDataType::Vector2}
        {}

        friend bool operator==(const ConstantOutputExtractor&, const ConstantOutputExtractor&) = default;
    private:
        osc::CStringView impl_name() const override { return name_; }
        osc::CStringView impl_description() const override { return {}; }
        OutputExtractorDataType impl_output_type() const override { return type_; }
        OutputValueExtractor impl_output_value_extractor(const OpenSim::Component&) const override;
        size_t impl_hash() const override;
        bool impl_equals(const OutputExtractor&) const override;

        std::string name_;
        osc::Variant value_;
        OutputExtractorDataType type_;
    };
}
