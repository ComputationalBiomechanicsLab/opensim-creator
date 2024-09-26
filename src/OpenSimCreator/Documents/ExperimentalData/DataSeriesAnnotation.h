#pragma once

#include <OpenSimCreator/Documents/ExperimentalData/DataPointType.h>

#include <string>
#include <vector>

namespace OpenSim { class Storage; }
namespace SimTK { template<class, class> class Array_; }
namespace SimTK { class DecorativeGeometry; }

namespace osc
{
    // A single data annotation that describes some kind of substructure (series) in
    // columnar data.
    struct DataSeriesAnnotation final {
        int dataColumnOffset = 0;
        std::string label;
        DataPointType dataType = DataPointType::Unknown;
    };

    // Returns the elements associated with one datapoint (e.g. [x, y, z])
    std::vector<double> extractDataPoint(
        double time,
        const OpenSim::Storage&,
        const DataSeriesAnnotation&
    );

    void generateDecorations(
        double time,
        const OpenSim::Storage&,
        const DataSeriesAnnotation&,
        SimTK::Array_<SimTK::DecorativeGeometry, unsigned>& out
    );
}
