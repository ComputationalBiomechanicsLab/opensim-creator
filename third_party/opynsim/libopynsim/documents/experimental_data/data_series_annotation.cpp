#include "data_series_annotation.h"

#include <libopynsim/documents/experimental_data/data_point_type.h>
#include <libopynsim/utilities/simbody_x_oscar.h>

#include <liboscar/graphics/color.h>
#include <liboscar/maths/closed_interval.h>
#include <liboscar/maths/quaternion_functions.h>
#include <liboscar/utilities/assertions.h>
#include <OpenSim/Common/Storage.h>
#include <SimTKcommon/internal/DecorativeGeometry.h>

#include <span>
#include <vector>

using namespace opyn;

namespace
{
    inline constexpr float c_force_arrow_length_scale = 0.0025f;

    // defines a decoration generator for a particular data point type
    template<DataPointType Type>
    void generate_decorations(std::span<const double, num_elements_in(Type)>, SimTK::Array_<SimTK::DecorativeGeometry>&);

    template<>
    void generate_decorations<DataPointType::Point>(
        std::span<const double, 3> data,
        SimTK::Array_<SimTK::DecorativeGeometry>& out)
    {
        const SimTK::Vec3 position = {data[0], data[1], data[2]};
        if (not position.isNaN()) {
            SimTK::DecorativeSphere sphere{};
            sphere.setRadius(0.005);  // i.e. like little 1 cm diameter markers
            sphere.setTransform(position);
            sphere.setColor(osc::to<SimTK::Vec3>(osc::Color::blue()));
            out.push_back(sphere);
        }
    }

    template<>
    void generate_decorations<DataPointType::ForcePoint>(
        std::span<const double, 6> data,
        SimTK::Array_<SimTK::DecorativeGeometry>& out)
    {
        const SimTK::Vec3 force = {data[0], data[1], data[2]};
        const SimTK::Vec3 point = {data[3], data[4], data[5]};

        if (not force.isNaN() and force.normSqr() > SimTK::Eps and not point.isNaN()) {

            SimTK::DecorativeArrow arrow{
                point,
                point + c_force_arrow_length_scale * force,
            };
            arrow.setScaleFactors({1, 1, 0.00001});
            arrow.setColor(osc::to<SimTK::Vec3>(osc::Color::orange()));
            arrow.setLineThickness(0.01);
            arrow.setTipLength(0.1);
            out.push_back(arrow);
        }
    }

    template<>
    void generate_decorations<DataPointType::BodyForce>(
        std::span<const double, 3> data,
        SimTK::Array_<SimTK::DecorativeGeometry>& out)
    {
        const SimTK::Vec3 position = {data[0], data[1], data[2]};
        if (not position.isNaN() and position.normSqr() > SimTK::Eps) {
            SimTK::DecorativeArrow arrow{
                SimTK::Vec3(0.0),
                position.normalize(),
            };
            arrow.setScaleFactors({1, 1, 0.00001});
            arrow.setColor(osc::to<SimTK::Vec3>(osc::Color::orange()));
            arrow.setLineThickness(0.01);
            arrow.setTipLength(0.1);
            out.push_back(arrow);
        }
    }

    template<>
    void generate_decorations<DataPointType::Orientation>(
        std::span<const double, 4> data,
        SimTK::Array_<SimTK::DecorativeGeometry>& out)
    {
        const osc::Quaternion q = osc::normalize(osc::Quaternion{
            static_cast<float>(data[0]),
            static_cast<float>(data[1]),
            static_cast<float>(data[2]),
            static_cast<float>(data[3]),
        });
        out.push_back(SimTK::DecorativeArrow{
            SimTK::Vec3(0.0),
            osc::to<SimTK::Vec3>(q * osc::Vector3{0.0f, 1.0f, 0.0f}),
        });
    }
}

void opyn::generate_decorations(
    double time,
    const OpenSim::Storage& storage,
    const DataSeriesAnnotation& annotation,
    SimTK::Array_<SimTK::DecorativeGeometry>& out)
{
    const osc::ClosedInterval<double> storage_time_range{storage.getFirstTime(), storage.getLastTime()};
    if (not storage_time_range.contains(time)) {
        return;  // time out of range: generate no decorations
    }

    const auto data = extract_data_point(time, storage, annotation);
    OSC_ASSERT_ALWAYS(data.size() == num_elements_in(annotation.data_type));

    static_assert(osc::num_options<DataPointType>() == 5);
    switch (annotation.data_type) {
    case DataPointType::Point:       ::generate_decorations<DataPointType::Point>(       std::span<const double, num_elements_in(DataPointType::Point)>{data},       out); break;
    case DataPointType::ForcePoint:  ::generate_decorations<DataPointType::ForcePoint>(  std::span<const double, num_elements_in(DataPointType::ForcePoint)>{data},  out); break;
    case DataPointType::BodyForce:   ::generate_decorations<DataPointType::BodyForce>(   std::span<const double, num_elements_in(DataPointType::BodyForce)>{data},   out); break;
    case DataPointType::Orientation: ::generate_decorations<DataPointType::Orientation>( std::span<const double, num_elements_in(DataPointType::Orientation)>{data}, out); break;

    // case DataPointType::Unknown: break;  // do nothing
    default:                     break;     // do nothing
    }
}

std::vector<double> opyn::extract_data_point(
    double time,
    const OpenSim::Storage& storage,
    const DataSeriesAnnotation& annotation)
{
    // lol, `OpenSim::Storage` API, etc.
    const int an = annotation.data_column_offset + static_cast<int>(num_elements_in(annotation.data_type));
    std::vector<double> buffer(static_cast<size_t>(an));
    double* p = buffer.data();
    storage.getDataAtTime(time, an, &p);
    buffer.erase(buffer.begin(), buffer.begin() + annotation.data_column_offset);
    return buffer;
}
