#include "PreviewExperimentalDataTab.h"

#include <OpenSimCreator/Documents/Model/UndoableModelActions.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Documents/PreviewExperimentalData/DataPointType.h>
#include <OpenSimCreator/Documents/PreviewExperimentalData/FileBackedStorage.h>
#include <OpenSimCreator/UI/IPopupAPI.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/UI/Shared/ModelEditorViewerPanel.h>
#include <OpenSimCreator/UI/Shared/ModelEditorViewerPanelParameters.h>
#include <OpenSimCreator/UI/Shared/NavigatorPanel.h>
#include <OpenSimCreator/UI/Shared/ObjectPropertiesEditor.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Common/MarkerData.h>
#include <OpenSim/Common/Storage.h>
#include <OpenSim/Simulation/Model/ExternalLoads.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/ModelComponent.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Graphics/Scene/SceneDecorationFlags.h>
#include <oscar/Graphics/Scene/SceneHelpers.h>
#include <oscar/Graphics/Scene/SceneRenderer.h>
#include <oscar/Graphics/Scene/SceneRendererParams.h>
#include <oscar/Maths.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/IconCodepoints.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/IconCache.h>
#include <oscar/UI/Panels/LogViewerPanel.h>
#include <oscar/UI/Panels/PanelManager.h>
#include <oscar/UI/Panels/StandardPanelImpl.h>
#include <oscar/UI/Tabs/StandardTabImpl.h>
#include <oscar/UI/Widgets/WindowMenu.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/StringHelpers.h>
#include <oscar_simbody/SimTKHelpers.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <ranges>
#include <regex>
#include <span>
#include <sstream>
#include <string>
#include <utility>

namespace rgs = std::ranges;

using namespace osc;
using namespace osc::literals;

// Low-level, UI-independent, datastructures.
namespace
{
    // Describes how a pattern of individual column headers in the source data could
    // be used to materialize a series of datapoints of type `DataPointType`.
    class DataSeriesPattern final {
    public:

        // Returns a `DataSeriesPattern` for the given `DataType`.
        template<DataPointType DataType, typename... ColumnHeaderStrings>
        requires
            (sizeof...(ColumnHeaderStrings) == numElementsIn(DataType)) and
            (std::constructible_from<CStringView, ColumnHeaderStrings> && ...)
        static DataSeriesPattern forDatatype(ColumnHeaderStrings&&... headerSuffixes)
        {
            return DataSeriesPattern{DataType, std::initializer_list<CStringView>{CStringView{std::forward<ColumnHeaderStrings>(headerSuffixes)}...}};  // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
        }

        // Returns the `DataPointType` matched by this pattern.
        DataPointType datatype() const { return m_Type; }

        // Returns `true` if the given column headers match this pattern.
        bool matches(std::span<const std::string> headers) const
        {
            if (headers.size() < m_HeaderSuffixes.size()) {
                return false;
            }
            for (size_t i = 0; i < m_HeaderSuffixes.size(); ++i) {
                if (not headers[i].ends_with(m_HeaderSuffixes[i])) {
                    return false;
                }
            }
            return true;
        }

        // If the given `columnHeader` matches a suffix in this pattern, returns a substring view
        // of the provided string view, minus the suffix. Otherwise, returns the provided string
        // view.
        std::string_view remove_suffix(std::string_view columHeader) const
        {
            for (const auto& suffix : m_HeaderSuffixes) {
                if (columHeader.ends_with(suffix)) {
                    return columHeader.substr(0, columHeader.size() - suffix.size());
                }
            }
            return columHeader;  // couldn't remove it
        }
    private:
        DataSeriesPattern(DataPointType type, std::initializer_list<CStringView> header_suffxes) :
            m_Type{type},
            m_HeaderSuffixes{header_suffxes}
        {}

        DataPointType m_Type;
        std::vector<CStringView> m_HeaderSuffixes;
    };

    // Describes a collection of patterns that _might_ match against the column headers
    // of the source data.
    //
    // Note: These patterns are based on how OpenSim 4.5 matches data in the 'Preview
    //       Experimental Data' part of the official OpenSim GUI.
    class DataSeriesPatterns final {
    public:

        // If the given headers matches a pattern, returns a pointer to the pattern. Otherwise,
        // returns `nullptr`.
        const DataSeriesPattern* try_match(std::span<const std::string> headers) const
        {
            const auto it = rgs::find_if(m_Patterns, [&headers](const auto& pattern) { return pattern.matches(headers); });
            return it != m_Patterns.end() ? &(*it) : nullptr;
        }
    private:
        std::vector<DataSeriesPattern> m_Patterns = {
            DataSeriesPattern::forDatatype<DataPointType::ForcePoint>("_vx", "_vy", "_vz", "_px", "_py", "_pz"),
            DataSeriesPattern::forDatatype<DataPointType::Point>("_vx", "_vy", "_vz"),
            DataSeriesPattern::forDatatype<DataPointType::Point>("_tx", "_ty", "_tz"),
            DataSeriesPattern::forDatatype<DataPointType::Point>("_px", "_py", "_pz"),
            DataSeriesPattern::forDatatype<DataPointType::Orientation>("_1", "_2", "_3", "_4"),
            DataSeriesPattern::forDatatype<DataPointType::Point>("_1", "_2", "_3"),
            DataSeriesPattern::forDatatype<DataPointType::BodyForce>("_fx", "_fy", "_fz"),

            // extra
            DataSeriesPattern::forDatatype<DataPointType::Point>("_x", "_y", "_z"),
            DataSeriesPattern::forDatatype<DataPointType::Point>("x", "y", "z"),
        };
    };

    // A single data annotation that describes some kind of substructure (series) in
    // columnar data.
    struct DataSeriesAnnotation final {
        int dataColumnOffset = 0;
        std::string label;
        DataPointType dataType = DataPointType::Unknown;
    };

    // Stores the higher-level schema associated with an `OpenSim::Storage`.
    class StorageSchema final {
    public:

        // Returns a `StorageSchema` by parsing (the column labels of) the
        // provided `OpenSim::Storage`.
        static StorageSchema parse(const OpenSim::Storage& storage)
        {
            const DataSeriesPatterns patterns;
            const auto& labels = storage.getColumnLabels();  // includes time

            std::vector<DataSeriesAnnotation> annotations;
            int offset = 1;  // offset 0 == "time" (skip it)

            while (offset < labels.size()) {
                const std::span<std::string> remainingLabels{&labels[offset], static_cast<size_t>(labels.size() - offset)};
                if (const DataSeriesPattern* pattern = patterns.try_match(remainingLabels)) {
                    annotations.push_back({
                        .dataColumnOffset = offset-1,  // drop time for this index
                        .label = std::string{pattern->remove_suffix(remainingLabels.front())},
                        .dataType = pattern->datatype(),
                    });
                    offset += static_cast<int>(numElementsIn(pattern->datatype()));
                }
                else {
                    annotations.push_back({
                        .dataColumnOffset = offset-1,  // drop time for this index
                        .label = remainingLabels.front(),
                        .dataType = DataPointType::Unknown,
                    });
                    offset += 1;
                }
            }
            return StorageSchema{std::move(annotations)};
        }

        const std::vector<DataSeriesAnnotation>& annotations() const { return m_Annotations; }
    private:
        explicit StorageSchema(std::vector<DataSeriesAnnotation> annotations) :
            m_Annotations{std::move(annotations)}
        {}

        std::vector<DataSeriesAnnotation> m_Annotations;
    };

    // Returns the elements associated with one datapoint (e.g. [x, y, z])
    std::vector<double> extractDataPoint(
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
}

// ui-independent graphics helpers
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

    void generateDecorations(
        const SimTK::State& state,
        const OpenSim::Storage& storage,
        const DataSeriesAnnotation& annotation,
        SimTK::Array_<SimTK::DecorativeGeometry>& out)
    {
        const auto time = state.getTime();
        const ClosedInterval<double> storageTimeRange{storage.getFirstTime(), storage.getLastTime()};
        if (not storageTimeRange.contains(time)) {
            return;  // time out of range: generate no decorations
        }

        const auto data = extractDataPoint(time, storage, annotation);
        OSC_ASSERT_ALWAYS(data.size() == numElementsIn(annotation.dataType));

        static_assert(num_options<DataPointType>() == 5);
        switch (annotation.dataType) {
        case DataPointType::Point:       generateDecorations<DataPointType::Point>(       std::span<const double, numElementsIn(DataPointType::Point)>{data},       out); break;
        case DataPointType::ForcePoint:  generateDecorations<DataPointType::ForcePoint>(  std::span<const double, numElementsIn(DataPointType::ForcePoint)>{data},  out); break;
        case DataPointType::BodyForce:   generateDecorations<DataPointType::BodyForce>(   std::span<const double, numElementsIn(DataPointType::BodyForce)>{data},   out); break;
        case DataPointType::Orientation: generateDecorations<DataPointType::Orientation>( std::span<const double, numElementsIn(DataPointType::Orientation)>{data}, out); break;

        case DataPointType::Unknown: break;  // do nothing
        default:                     break;  // do nothing
        }
    }
}

// Specialized `OpenSim::Component`s for this UI
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
            ::generateDecorations(state, *m_Storage, m_Annotation, out);
        }

        std::shared_ptr<const OpenSim::Storage> m_Storage;
        DataSeriesAnnotation m_Annotation;
    };

    // Holds an annotated motion track.
    //
    // Note: This is similar to OpenSim GUI (4.5)'s `AnnotatedMotion.java` class. The
    //       reason it's reproduced here is to provide like-for-like behavior between
    //       OSC's 'preview experimental data' and OpenSim's.
    class AnnotatedMotion final : public OpenSim::ModelComponent {
        OpenSim_DECLARE_CONCRETE_OBJECT(AnnotatedMotion, OpenSim::ModelComponent)
    public:
        // Constructs an `AnnotationMotion` that was loaded from the given filesystem
        // path, or throws an `std::exception` if any error occurs.
        explicit AnnotatedMotion(const std::filesystem::path& path) :
            AnnotatedMotion{loadPathIntoStorage(path)}
        {
            setName(path.filename().string());
        }

    private:
        static std::shared_ptr<OpenSim::Storage> loadPathIntoStorage(const std::filesystem::path& path)
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

        explicit AnnotatedMotion(std::shared_ptr<OpenSim::Storage> storage) :
            m_Storage{std::move(storage)},
            m_Schema{StorageSchema::parse(*m_Storage)}
        {
            setName(m_Storage->getName());

            for (const auto& annotation : m_Schema.annotations()) {
                addComponent(std::make_unique<DataSeries>(m_Storage, annotation).release());
            }
        }
        std::shared_ptr<OpenSim::Storage> m_Storage;
        StorageSchema m_Schema;
    };
}

// Top-level UI state that's share-able between various panels in the
// preview experimental data UI.
namespace
{
    class PreviewExperimentalDataUiState final {
    public:
        const std::shared_ptr<UndoableModelStatePair>& updSharedModelPtr() const { return m_Model; }
        UndoableModelStatePair& updModel() { return *m_Model; }

        void loadModelFile(const std::filesystem::path& p)
        {
            m_Model->setModel(std::make_unique<OpenSim::Model>(p.string()));
            m_Model->setFilesystemPath(p);
            reinitializeModelFromBackingData("loaded model");
        }

        void reloadAll(std::string_view label = "reloaded model")
        {
            // reload/reset model
            if (m_Model->hasFilesystemLocation()) {
                SceneCache dummy;
                ActionReloadOsimFromDisk(*m_Model, dummy);
            }
            else {
                m_Model->updModel() = OpenSim::Model{};
            }

            // if applicable, reload associated trajectory
            if (m_AssociatedTrajectory) {
                m_AssociatedTrajectory->reloadFromDisk(m_Model->getModel());
            }

            // reinitialize everything else
            reinitializeModelFromBackingData(label);
        }

        void loadModelTrajectoryFile(const std::filesystem::path& path)
        {
            m_AssociatedTrajectory = FileBackedStorage{m_Model->getModel(), path};
            reloadAll("loaded trajactory");
        }

        void loadMotionFiles(std::span<const std::filesystem::path> paths)
        {
            if (paths.empty()) {
                return;
            }

            m_AssociatedMotionFiles.insert(m_AssociatedMotionFiles.end(), paths.begin(), paths.end());
            reloadAll(paths.size() == 1 ? "loaded motion" : "loaded motions");
        }

        void loadXMLAsOpenSimDocument(std::span<const std::filesystem::path> paths)
        {
            if (paths.empty()) {
                return;
            }

            m_AssociatedXMLDocuments.insert(m_AssociatedXMLDocuments.end(), paths.begin(), paths.end());
            reloadAll(paths.size() == 1 ? "loaded XML document" : "loaded XML documents");
        }

        ClosedInterval<float> getTimeRange() const
        {
            return m_TimeRange;
        }

        void setTimeRange(ClosedInterval<float> newTimeRange)
        {
            m_TimeRange = newTimeRange;
        }

        double getScrubTime() const
        {
            return m_ScrubTime;
        }

        void setScrubTime(double newTime)
        {
            SimTK::State& state = m_Model->updState();
            state.setTime(newTime);

            if (m_AssociatedTrajectory) {
                UpdateStateFromStorageTime(
                    m_Model->updModel(),
                    state,
                    m_AssociatedTrajectory->mapper(),
                    m_AssociatedTrajectory->storage(),
                    newTime
                );
                // m_Model->updModel().assemble(state);
                // m_Model->updModel().equilibrateMuscles(state);
                m_Model->getModel().realizeReport(state);
            }
            else {
                // no associated motion: only change the time part of the state and re-realize
                m_Model->updModel().equilibrateMuscles(state);
                m_Model->updModel().realizeDynamics(state);
            }
            m_ScrubTime = static_cast<float>(newTime);
        }

        void rollbackModel()
        {
            m_Model->rollback();
        }
    private:
        void reinitializeModelFromBackingData(std::string_view label)
        {
            // hide forces that are computed from the model, because it's assumed that the
            // user only wants to visualize forces that come from externally-supplied data
            for (auto& force : m_Model->updModel().updComponentList<OpenSim::Force>()) {
                force.set_appliesForce(false);
            }

            // (re)load associated trajectory
            if (m_AssociatedTrajectory) {
                InitializeModel(m_Model->updModel());
                m_AssociatedTrajectory->reloadFromDisk(m_Model->getModel());
            }

            // (re)load motions
            for (const std::filesystem::path& path : m_AssociatedMotionFiles) {
                m_Model->updModel().addModelComponent(std::make_unique<AnnotatedMotion>(path).release());
            }

            // (re)load associated XML files (e.g. `ExternalLoads`)
            bool anyObjectIsExternalLoads = false;
            for (const std::filesystem::path& path : m_AssociatedXMLDocuments) {
                auto ptr = std::unique_ptr<OpenSim::Object>{OpenSim::Object::makeObjectFromFile(path.string())};
                if (dynamic_cast<const OpenSim::ExternalLoads*>(ptr.get())) {
                    // HACK: we need to know this so that we don't commit the model, because
                    // ExternalLoads stupidly depends on an auto-resetting `Object::_document`
                    // member.
                    anyObjectIsExternalLoads = true;
                }
                if (dynamic_cast<OpenSim::ModelComponent*>(ptr.get())) {
                    m_Model->updModel().addModelComponent(dynamic_cast<OpenSim::ModelComponent*>(ptr.release()));
                }
            }

            // care: state initialization is dependent on `m_AssociatedTrajectory`
            InitializeModel(m_Model->updModel());
            InitializeState(m_Model->updModel());
            if (not anyObjectIsExternalLoads) {  // see HACK above
                m_Model->commit(label);
            }

            setScrubTime(m_ScrubTime);
        }

        std::shared_ptr<UndoableModelStatePair> m_Model = std::make_shared<UndoableModelStatePair>();
        std::optional<FileBackedStorage> m_AssociatedTrajectory;
        std::vector<std::filesystem::path> m_AssociatedMotionFiles;
        std::vector<std::filesystem::path> m_AssociatedXMLDocuments;
        ClosedInterval<float> m_TimeRange = {0.0f, 10.0f};
        float m_ScrubTime = 0.0f;
    };

    class ReadonlyPropertiesEditorPanel final : public StandardPanelImpl {
    public:
        ReadonlyPropertiesEditorPanel(
            std::string_view panelName,
            IPopupAPI* api,
            const std::shared_ptr<const UndoableModelStatePair>& targetModel) :

            StandardPanelImpl{panelName},
            m_PropertiesEditor{api, targetModel, [model = targetModel](){ return model->getSelected(); }}
        {}
    private:
        void impl_draw_content() final
        {
            ui::begin_disabled();
            m_PropertiesEditor.onDraw();
            ui::end_disabled();
        }
        ObjectPropertiesEditor m_PropertiesEditor;
    };
}

class osc::PreviewExperimentalDataTab::Impl final :
    public StandardTabImpl,
    public IPopupAPI {
public:
    explicit Impl(const ParentPtr<ITabHost>&) :
        StandardTabImpl{OSC_ICON_DOT_CIRCLE " Experimental Data"}
    {
        m_PanelManager->register_toggleable_panel(
            "Navigator",
            [this](std::string_view panelName)
            {
                return std::make_shared<NavigatorPanel>(
                    panelName,
                    m_UiState->updSharedModelPtr(),
                    [](const OpenSim::ComponentPath&) {}
                );
            }
        );
        m_PanelManager->register_spawnable_panel(
            "viewer",
            [this](std::string_view panelName)
            {
                auto onRightClick = [](const ModelEditorViewerPanelRightClickEvent&) {};
                ModelEditorViewerPanelParameters panelParams{m_UiState->updSharedModelPtr(), onRightClick};
                panelParams.updRenderParams().decorationOptions.setMuscleColoringStyle(MuscleColoringStyle::Default);
                return std::make_shared<ModelEditorViewerPanel>(panelName, panelParams);
            },
            1  // have one viewer open at the start
        );
        m_PanelManager->register_toggleable_panel(
            "Log",
            [](std::string_view panelName)
            {
                return std::make_shared<LogViewerPanel>(panelName);
            }
        );
        m_PanelManager->register_toggleable_panel(
            "Properties",
            [this](std::string_view panelName)
            {
                return std::make_shared<ReadonlyPropertiesEditorPanel>(panelName, this, m_UiState->updSharedModelPtr());
            }
        );
    }
private:
    void impl_on_mount() final
    {
        m_PanelManager->on_mount();
    }

    void impl_on_unmount() final
    {
        m_PanelManager->on_unmount();
    }

    void impl_on_tick() final
    {
        m_PanelManager->on_tick();
    }

    void impl_on_draw_main_menu() final
    {
        m_WindowMenu.on_draw();
    }

    void impl_on_draw() final
    {
        try {
            ui::enable_dockspace_over_main_viewport();
            drawToolbar();
            m_PanelManager->on_draw();
            m_ThrewExceptionLastFrame = false;
        }
        catch (const std::exception& ex) {
            if (m_ThrewExceptionLastFrame) {
                throw;
            }

            m_ThrewExceptionLastFrame = true;
            log_error("error detected: %s", ex.what());
            log_error("rolling back model");
            m_UiState->rollbackModel();
        }
    }

    void implPushPopup(std::unique_ptr<IPopup>) final
    {
    }

    void drawToolbar()
    {
        if (BeginToolbar("##PreviewExperimentalDataToolbar", Vec2{5.0f, 5.0f}))
        {
            // load/reload etc.
            {
                if (ui::draw_button("load model")) {
                    if (const auto path = prompt_user_to_select_file({"osim"})) {
                        m_UiState->loadModelFile(*path);
                    }
                }

                ui::same_line();
                if (ui::draw_button("load model trajectory/states")) {
                    if (const auto path = prompt_user_to_select_file({"sto", "mot"})) {
                        m_UiState->loadModelTrajectoryFile(*path);
                    }
                }

                ui::same_line();
                if (ui::draw_button("load raw data file")) {
                    m_UiState->loadMotionFiles(prompt_user_to_select_files({"sto", "mot", "trc"}));
                }

                ui::same_line();
                if (ui::draw_button("load OpenSim XML")) {
                    m_UiState->loadXMLAsOpenSimDocument(prompt_user_to_select_files({"xml"}));
                }

                ui::same_line();
                if (ui::draw_button(OSC_ICON_RECYCLE " reload all")) {
                    m_UiState->reloadAll();
                }
            }

            // scrubber
            ui::draw_same_line_with_vertical_separator();
            {
                ClosedInterval<float> tr = m_UiState->getTimeRange();
                ui::set_next_item_width(ui::calc_text_size("<= xxxx").x);
                if (ui::draw_float_input("<=", &tr.lower)) {
                    m_UiState->setTimeRange(tr);
                }

                ui::same_line();
                auto t = static_cast<float>(m_UiState->getScrubTime());
                ui::set_next_item_width(ui::calc_text_size("----------------------------------------------------------------").x);
                if (ui::draw_float_slider("t", &t, static_cast<float>(tr.lower), static_cast<float>(tr.upper), "%.6f")) {
                    m_UiState->setScrubTime(t);
                }

                ui::same_line();
                ui::draw_text("<=");
                ui::same_line();
                ui::set_next_item_width(ui::calc_text_size("<= xxxx").x);
                if (ui::draw_float_input("##<=", &tr.upper)) {
                    m_UiState->setTimeRange(tr);
                }
            }

            // scaling, visualization toggles.
            ui::draw_same_line_with_vertical_separator();
            {
                DrawSceneScaleFactorEditorControls(m_UiState->updModel());
                if (not m_IconCache) {
                    m_IconCache = App::singleton<IconCache>(
                        App::resource_loader().with_prefix("icons/"),
                        ui::get_text_line_height()/128.0f
                    );
                }
                ui::same_line();
                DrawAllDecorationToggleButtons(m_UiState->updModel(), *m_IconCache);
            }


            ui::draw_same_line_with_vertical_separator();
        }
        ui::end_panel();
    }

    std::shared_ptr<PreviewExperimentalDataUiState> m_UiState = std::make_shared<PreviewExperimentalDataUiState>();
    std::shared_ptr<PanelManager> m_PanelManager = std::make_shared<PanelManager>();

    WindowMenu m_WindowMenu{m_PanelManager};
    std::shared_ptr<IconCache> m_IconCache;
    bool m_ThrewExceptionLastFrame = false;
};


CStringView osc::PreviewExperimentalDataTab::id() noexcept
{
    return "OpenSim/Experimental/PreviewExperimentalData";
}

osc::PreviewExperimentalDataTab::PreviewExperimentalDataTab(const ParentPtr<ITabHost>& ptr) :
    m_Impl{std::make_unique<Impl>(ptr)}
{}
osc::PreviewExperimentalDataTab::PreviewExperimentalDataTab(PreviewExperimentalDataTab&&) noexcept = default;
PreviewExperimentalDataTab& osc::PreviewExperimentalDataTab::operator=(PreviewExperimentalDataTab&&) noexcept = default;
osc::PreviewExperimentalDataTab::~PreviewExperimentalDataTab() noexcept = default;

UID osc::PreviewExperimentalDataTab::impl_get_id() const
{
    return m_Impl->id();
}

CStringView osc::PreviewExperimentalDataTab::impl_get_name() const
{
    return m_Impl->name();
}

void osc::PreviewExperimentalDataTab::impl_on_mount()
{
    m_Impl->on_mount();
}

void osc::PreviewExperimentalDataTab::impl_on_unmount()
{
    m_Impl->on_unmount();
}

void osc::PreviewExperimentalDataTab::impl_on_tick()
{
    m_Impl->on_tick();
}

bool osc::PreviewExperimentalDataTab::impl_on_event(Event& e)
{
    return m_Impl->on_event(e);
}

void osc::PreviewExperimentalDataTab::impl_on_draw_main_menu()
{
    return m_Impl->on_draw_main_menu();
}

void osc::PreviewExperimentalDataTab::impl_on_draw()
{
    m_Impl->on_draw();
}
