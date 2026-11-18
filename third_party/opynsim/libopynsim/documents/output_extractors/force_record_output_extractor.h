#pragma once

#include <libopynsim/documents/output_extractors/output_extractor.h>
#include <libopynsim/documents/output_extractors/output_value_extractor.h>

#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/clone_ptr.h>

#include <cstddef>

namespace OpenSim
{
    class Force;
}

namespace opyn
{
    // An `OutputExtractor` that extracts the nth record from an `OpenSim::Force`'s
    // record values.
    class ForceRecordOutputExtractor final : public OutputExtractor {
    public:
        explicit ForceRecordOutputExtractor(const OpenSim::Force&, int record_index);
        ForceRecordOutputExtractor(const ForceRecordOutputExtractor&);
        ForceRecordOutputExtractor(ForceRecordOutputExtractor&&) noexcept;
        ForceRecordOutputExtractor& operator=(const ForceRecordOutputExtractor&);
        ForceRecordOutputExtractor& operator=(ForceRecordOutputExtractor&&) noexcept;
        ~ForceRecordOutputExtractor() noexcept override;

    private:
        osc::CStringView impl_name() const override;
        osc::CStringView impl_description() const override;
        OutputExtractorDataType impl_output_type() const override;
        OutputValueExtractor impl_output_value_extractor(const OpenSim::Component&) const override;
        size_t impl_hash() const override;
        bool impl_equals(const OutputExtractor&) const override;

        class Impl;
        osc::ClonePtr<Impl> impl_;
    };
}
