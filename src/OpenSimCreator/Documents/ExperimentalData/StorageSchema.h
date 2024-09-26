#pragma once

#include <OpenSimCreator/Documents/ExperimentalData/DataSeriesAnnotation.h>

#include <utility>
#include <vector>

namespace OpenSim { class Storage; }

namespace osc
{
    // Stores the higher-level schema associated with an `OpenSim::Storage`.
    class StorageSchema final {
    public:

        // Returns a `StorageSchema` by parsing (the column labels of) the
        // provided `OpenSim::Storage`.
        static StorageSchema parse(const OpenSim::Storage&);

        const std::vector<DataSeriesAnnotation>& annotations() const { return m_Annotations; }
    private:
        explicit StorageSchema(std::vector<DataSeriesAnnotation> annotations) :
            m_Annotations{std::move(annotations)}
        {}

        std::vector<DataSeriesAnnotation> m_Annotations;
    };
}
