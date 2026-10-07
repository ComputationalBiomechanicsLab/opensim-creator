#pragma once

#include <libopynsim/documents/experimental_data/data_point_type.h>

#include <string>
#include <vector>

namespace OpenSim { class Storage; }
namespace SimTK { template<class, class> class Array_; }
namespace SimTK { class DecorativeGeometry; }

namespace opyn
{
    // A single data annotation that describes some kind of substructure (series) in
    // columnar data.
    struct DataSeriesAnnotation final {
        int data_column_offset = 0;
        std::string label;
        DataPointType data_type = DataPointType::Unknown;
    };

    // Returns the elements associated with one datapoint (e.g. [x, y, z])
    std::vector<double> extract_data_point(
        double time,
        const OpenSim::Storage&,
        const DataSeriesAnnotation&
    );

    void generate_decorations(
        double time,
        const OpenSim::Storage&,
        const DataSeriesAnnotation&,
        SimTK::Array_<SimTK::DecorativeGeometry, unsigned>& out
    );
}
