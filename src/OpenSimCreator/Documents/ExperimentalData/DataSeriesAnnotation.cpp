#include "DataSeriesAnnotation.h"

#include <OpenSimCreator/Documents/ExperimentalData/DataPointType.h>

#include <OpenSim/Common/Storage.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/ClosedInterval.h>
#include <oscar/Maths/QuaternionFunctions.h>
#include <oscar/Utils/Assertions.h>
#include <oscar_simbody/SimTKConverters.h>
#include <Simbody.h>

#include <span>
#include <vector>

using namespace osc;

namespace
{
    inline constexpr float c_ForceArrowLengthScale = 0.0025f;

    // defines a decoration generator for a particular data point type
    template<DataPointType Type>
    void generateDecorations(std::span<const double, numElementsIn(Type)>, SimTK::Array_<SimTK::DecorativeGeometry>&);

    template<>
    void generateDecorations<DataPointType::Point>(
        std::span<const double, 3> data,
        SimTK::Array_<SimTK::DecorativeGeometry>& out)
    {
        const SimTK::Vec3 position = {data[0], data[1], data[2]};
        if (not position.isNaN()) {
            SimTK::DecorativeSphere sphere{};
            sphere.setRadius(0.005);  // i.e. like little 1 cm diameter markers
            sphere.setTransform(position);
            sphere.setColor(to<SimTK::Vec3>(Color::blue()));
            out.push_back(sphere);
        }
    }

    template<>
    void generateDecorations<DataPointType::ForcePoint>(
        std::span<const double, 6> data,
        SimTK::Array_<SimTK::DecorativeGeometry>& out)
    {
        const SimTK::Vec3 force = {data[0], data[1], data[2]};
        const SimTK::Vec3 point = {data[3], data[4], data[5]};

        if (not force.isNaN() and force.normSqr() > SimTK::Eps and not point.isNaN()) {

            SimTK::DecorativeArrow arrow{
                point,
                point + c_ForceArrowLengthScale * force,
            };
            arrow.setScaleFactors({1, 1, 0.00001});
            arrow.setColor(to<SimTK::Vec3>(Color::orange()));
            arrow.setLineThickness(0.01);
            arrow.setTipLength(0.1);
            out.push_back(arrow);
        }
    }

    template<>
    void generateDecorations<DataPointType::BodyForce>(
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
            arrow.setColor(to<SimTK::Vec3>(Color::orange()));
            arrow.setLineThickness(0.01);
            arrow.setTipLength(0.1);
            out.push_back(arrow);
        }
    }

    template<>
    void generateDecorations<DataPointType::Orientation>(
        std::span<const double, 4> data,
        SimTK::Array_<SimTK::DecorativeGeometry>& out)
    {
        const Quat q = normalize(Quat{
            static_cast<float>(data[0]),
            static_cast<float>(data[1]),
            static_cast<float>(data[2]),
            static_cast<float>(data[3]),
        });
        out.push_back(SimTK::DecorativeArrow{
            SimTK::Vec3(0.0),
            to<SimTK::Vec3>(q * Vec3{0.0f, 1.0f, 0.0f}),
        });
    }
}

void osc::generateDecorations(
    double time,
    const OpenSim::Storage& storage,
    const DataSeriesAnnotation& annotation,
    SimTK::Array_<SimTK::DecorativeGeometry>& out)
{
    const ClosedInterval<double> storageTimeRange{storage.getFirstTime(), storage.getLastTime()};
    if (not storageTimeRange.contains(time)) {
        return;  // time out of range: generate no decorations
    }

    const auto data = extractDataPoint(time, storage, annotation);
    OSC_ASSERT_ALWAYS(data.size() == numElementsIn(annotation.dataType));

    static_assert(num_options<DataPointType>() == 5);
    switch (annotation.dataType) {
    case DataPointType::Point:       ::generateDecorations<DataPointType::Point>(       std::span<const double, numElementsIn(DataPointType::Point)>{data},       out); break;
    case DataPointType::ForcePoint:  ::generateDecorations<DataPointType::ForcePoint>(  std::span<const double, numElementsIn(DataPointType::ForcePoint)>{data},  out); break;
    case DataPointType::BodyForce:   ::generateDecorations<DataPointType::BodyForce>(   std::span<const double, numElementsIn(DataPointType::BodyForce)>{data},   out); break;
    case DataPointType::Orientation: ::generateDecorations<DataPointType::Orientation>( std::span<const double, numElementsIn(DataPointType::Orientation)>{data}, out); break;

    case DataPointType::Unknown: break;  // do nothing
    default:                     break;  // do nothing
    }
}

std::vector<double> osc::extractDataPoint(
    double time,
    const OpenSim::Storage& storage,
    const DataSeriesAnnotation& annotation)
{
    // lol, `OpenSim::Storage` API, etc.
    int aN = annotation.dataColumnOffset + static_cast<int>(numElementsIn(annotation.dataType));
    std::vector<double> buffer(aN);
    double* p = buffer.data();
    storage.getDataAtTime(time, aN, &p);
    buffer.erase(buffer.begin(), buffer.begin() + annotation.dataColumnOffset);
    return buffer;
}
