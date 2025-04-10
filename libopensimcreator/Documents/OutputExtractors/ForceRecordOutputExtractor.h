#pragma once

#include <libopensimcreator/Documents/OutputExtractors/IOutputExtractor.h>
#include <libopensimcreator/Documents/OutputExtractors/OutputValueExtractor.h>

#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/ClonePtr.h>

#include <cstddef>

namespace OpenSim { class Force; }

namespace osc
{
    // An `IOutputExtractor` that extracts the nth record from an `OpenSim::Force`'s
    // record values.
    class ForceRecordOutputExtractor final : public IOutputExtractor {
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
        bool implEquals(const IOutputExtractor&) const final;

        class Impl;
        ClonePtr<Impl> m_Impl;
    };
}
