#include "annotated_motion.h"

#include <libopynsim/Documents/experimental_data/data_series_annotation.h>
#include <libopynsim/Documents/experimental_data/storage_schema.h>

#include <liboscar/platform/log.h>
#include <OpenSim/Common/MarkerData.h>
#include <OpenSim/Common/Storage.h>
#include <OpenSim/Common/Units.h>
#include <OpenSim/Simulation/Model/ModelComponent.h>

#include <filesystem>
#include <memory>
#include <string>
#include <utility>

using namespace osc;

namespace
{
    // Refers to one data series within one annotated motion.
    class DataSeries final : public OpenSim::ModelComponent {
        OpenSim_DECLARE_CONCRETE_OBJECT(DataSeries, OpenSim::ModelComponent)
    public:
        OpenSim_DECLARE_PROPERTY(type, std::string, "the datatype of the data series")
        OpenSim_DECLARE_PROPERTY(column_offset, int, "index of the first column (excl. time) that contains this data series")

        explicit DataSeries(
            std::shared_ptr<const OpenSim::Storage> storage,
            const DataSeriesAnnotation& annotation) :

            m_Storage{std::move(storage)},
            m_Annotation{annotation}
        {
            setName(annotation.label);
            constructProperty_type(std::string{labelFor(annotation.dataType)});
            constructProperty_column_offset(annotation.dataColumnOffset);
        }

    private:
        void generateDecorations(
            bool,
            const OpenSim::ModelDisplayHints&,
            const SimTK::State& state,
            SimTK::Array_<SimTK::DecorativeGeometry>& out) const override
        {
            ::generateDecorations(state.getTime(), *m_Storage, m_Annotation, out);
        }

        std::shared_ptr<const OpenSim::Storage> m_Storage;
        DataSeriesAnnotation m_Annotation;
    };
}

osc::AnnotatedMotion::AnnotatedMotion(const std::filesystem::path& path) :
    AnnotatedMotion{loadPathIntoStorage(path)}
{
    setName(path.filename().string());
}

std::shared_ptr<OpenSim::Storage> osc::AnnotatedMotion::loadPathIntoStorage(const std::filesystem::path& path)
{
    if (path.extension() == ".trc") {
        // use `MarkerData`, same as OpenSim GUI's `FileLoadDataAction.java`
        OpenSim::MarkerData markerData{path.string()};
        markerData.convertToUnits(OpenSim::Units::Meters);

        auto storage = std::make_shared<OpenSim::Storage>();
        markerData.makeRdStorage(*storage);
        return storage;
    }
    else {
        return std::make_shared<OpenSim::Storage>(path.string());
    }
}

osc::AnnotatedMotion::AnnotatedMotion(std::shared_ptr<OpenSim::Storage> storage) :
    m_Storage{std::move(storage)}
{
    setName(m_Storage->getName());

    const auto schema = StorageSchema::parse(*m_Storage);
    for (const auto& annotation : schema.annotations()) {
        // Handle issue #1068
        //
        // A data series loaded from an `OpenSim::Storage` may not be mappable to
        // an `OpenSim::Component` because the series' name may contain invalid
        // characters (for an `OpenSim::Component` name, at least).
        //
        // In those (edge) cases, the implementation should filter out the data series
        // and warn the user what's happened, rather than failing. This is because
        // OpenSim GUI can load this kind of data: it separates "The OpenSim
        // model being viewed" (in OSC: `UndoableModelStatePair`) from "The
        // renderable UI tree that the GUI is showing" (in OpenSim GUI:
        // `ExperimentalMarkerNode` and `OpenSimNode`).
        try {
            auto series = std::make_unique<DataSeries>(m_Storage, annotation);
            series->finalizeFromProperties();
            addComponent(series.release());
        }
        catch (const std::exception& ex) {
            log_warn("Error loading a data series from %s: %s", getName().c_str(), ex.what());
        }
    }
}

size_t osc::AnnotatedMotion::getNumDataSeries() const
{
    const auto lst = getComponentList<DataSeries>();
    return std::distance(lst.begin(), lst.end());
}
