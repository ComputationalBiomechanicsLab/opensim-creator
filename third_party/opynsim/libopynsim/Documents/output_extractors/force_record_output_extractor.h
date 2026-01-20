#pragma once

#include <libopynsim/Documents/output_extractors/output_extractor.h>
#include <libopynsim/Documents/output_extractors/output_value_extractor.h>

#include <liboscar/utils/c_string_view.h>
#include <liboscar/utils/clone_ptr.h>

#include <cstddef>

namespace OpenSim { class Force; }

namespace osc
{
    // An `OutputExtractor` that extracts the nth record from an `OpenSim::Force`'s
    // record values.
    class ForceRecordOutputExtractor final : public OutputExtractor {
    public:
        explicit ForceRecordOutputExtractor(
            const OpenSim::Force&,
            int recordIndex
        );
        ForceRecordOutputExtractor(const ForceRecordOutputExtractor&);
        ForceRecordOutputExtractor(ForceRecordOutputExtractor&&) noexcept;
        ForceRecordOutputExtractor& operator=(const ForceRecordOutputExtractor&);
        ForceRecordOutputExtractor& operator=(ForceRecordOutputExtractor&&) noexcept;
        ~ForceRecordOutputExtractor() noexcept override;

    private:
        CStringView implGetName() const final;
        CStringView implGetDescription() const final;
        OutputExtractorDataType implGetOutputType() const final;
        OutputValueExtractor implGetOutputValueExtractor(const OpenSim::Component&) const final;
        size_t implGetHash() const final;
        bool implEquals(const OutputExtractor&) const final;

        class Impl;
        ClonePtr<Impl> m_Impl;
    };
}
