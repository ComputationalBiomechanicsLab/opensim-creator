#include "PreviewExperimentalDataTab.h"

#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/UI/Shared/ModelEditorViewerPanel.h>
#include <OpenSimCreator/UI/Shared/ModelEditorViewerPanelParameters.h>
#include <OpenSimCreator/UI/Shared/NavigatorPanel.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Graphics/Scene/SceneDecorationFlags.h>
#include <oscar/Graphics/Scene/SceneHelpers.h>
#include <oscar/Graphics/Scene/SceneRenderer.h>
#include <oscar/Graphics/Scene/SceneRendererParams.h>
#include <oscar/Maths.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/LogViewerPanel.h>
#include <oscar/UI/Panels/PanelManager.h>
#include <oscar/UI/Tabs/StandardTabImpl.h>
#include <oscar/UI/Widgets/WindowMenu.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/StringHelpers.h>

#include <IconsFontAwesome5.h>
#include <OpenSim/Common/Storage.h>
#include <OpenSim/Simulation/Model/ModelComponent.h>
#include <SDL_events.h>
#include <oscar_simbody/SimTKHelpers.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <ranges>
#include <regex>
#include <span>
#include <string>
#include <utility>

namespace rgs = std::ranges;

using namespace osc;
using namespace osc::literals;

// Low-level, UI-independent, datastructures.
namespace
{
    // Describes the type of data held by [1..N] columns in the source data.
    enum class DataPointType {
        Point = 0,
        PointForce,
        BodyForce,
        Orientation,
        Unknown,
        NUM_OPTIONS,
    };

    // A compile-time typelist of all possible `DataPointType`s.
    using DataPointTypeList = OptionList<DataPointType,
        DataPointType::Point,
        DataPointType::PointForce,
        DataPointType::BodyForce,
        DataPointType::Orientation,
        DataPointType::Unknown
    >;

    // Compile-time traits associated with a `DataPointType`.
    template<DataPointType>
    struct ColumnDataTypeTraits;

    template<>
    struct ColumnDataTypeTraits<DataPointType::Point> final {
        static constexpr CStringView label = "Point";
        static constexpr size_t num_elements = 3;
    };

    template<>
    struct ColumnDataTypeTraits<DataPointType::PointForce> final {
        static constexpr CStringView label = "PointForce";
        static constexpr size_t num_elements = 6;
    };

    template<>
    struct ColumnDataTypeTraits<DataPointType::BodyForce> final {
        static constexpr CStringView label = "BodyForce";
        static constexpr size_t num_elements = 3;
    };

    template<>
    struct ColumnDataTypeTraits<DataPointType::Orientation> final {
        static constexpr CStringView label = "Orientation";
        static constexpr size_t num_elements = 4;
    };

    template<>
    struct ColumnDataTypeTraits<DataPointType::Unknown> final {
        static constexpr CStringView label = "Unknown";
        static constexpr size_t num_elements = 1;
    };

    // Returns the number of elements in a given `DataPointType`.
    constexpr size_t numElementsIn(DataPointType t)
    {
        constexpr auto lut = []<DataPointType... Types>(OptionList<DataPointType, Types...>)
        {
            return std::to_array({ ColumnDataTypeTraits<Types>::num_elements... });
        }(DataPointTypeList{});

        return lut.at(to_index(t));
    }

    // Returns a human-readable label for a given `DataPointType`.
    constexpr CStringView labelFor(DataPointType t)
    {
        constexpr auto lut = []<DataPointType... Types>(OptionList<DataPointType, Types...>)
        {
            return std::to_array({ ColumnDataTypeTraits<Types>::label... });
        }(DataPointTypeList{});

        return lut.at(to_index(t));
    }

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
            return DataSeriesPattern{DataType, {CStringView{std::forward<ColumnHeaderStrings>(headerSuffixes)}...}};
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
            m_HeaderSuffixes{std::move(header_suffxes)}
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
            DataSeriesPattern::forDatatype<DataPointType::PointForce>("_vx", "_vy", "_vz", "_px", "_py", "_pz"),
            DataSeriesPattern::forDatatype<DataPointType::Point>("_vx", "_vy", "_vz"),
            DataSeriesPattern::forDatatype<DataPointType::Point>("_tx", "_ty", "_tz"),
            DataSeriesPattern::forDatatype<DataPointType::Point>("_px", "_py", "_pz"),
            DataSeriesPattern::forDatatype<DataPointType::Orientation>("_1", "_2", "_3", "_4"),
            DataSeriesPattern::forDatatype<DataPointType::Point>("_1", "_2", "_3"),
            DataSeriesPattern::forDatatype<DataPointType::BodyForce>("_fx", "_fy", "_fz"),
        };
    };

    // A single data annotation that describes some kind of substructure (series) in
    // columnar data.
    struct DataSeriesAnnotation final {
        int firstColumnOffset = 0;
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
            const auto& labels = storage.getColumnLabels();

            std::vector<DataSeriesAnnotation> annotations;
            int offset = 1;  // offset 0 == "time" (skip it)

            while (offset < labels.size()) {
                const std::span<std::string> remainingLabels{&labels[offset], static_cast<size_t>(labels.size() - offset)};
                if (const DataSeriesPattern* pattern = patterns.try_match(remainingLabels)) {
                    annotations.push_back({
                        .firstColumnOffset = offset,
                        .label = std::string{pattern->remove_suffix(remainingLabels.front())},
                        .dataType = pattern->datatype(),
                    });
                    offset += static_cast<int>(numElementsIn(pattern->datatype()));
                }
                else {
                    annotations.push_back({
                        .firstColumnOffset = offset,
                        .label = remainingLabels.front(),
                        .dataType = DataPointType::Unknown,
                    });
                    offset += 1;
                }
            }
            return StorageSchema{std::move(annotations)};
        }

    private:
        explicit StorageSchema(std::vector<DataSeriesAnnotation> annotations) :
            m_Annotations{std::move(annotations)}
        {}

        std::vector<DataSeriesAnnotation> m_Annotations;
    };
}

// ui-independent graphics helpers
namespace
{
    // defines a decoration generator for a particular data point type
    template<DataPointType Type>
    void generateDecorations(const SimTK::State&, std::span<const double, numElementsIn(Type)>, SimTK::Array_<SimTK::DecorativeGeometry>&);

    template<>
    void generateDecorations<DataPointType::Point>(
        const SimTK::State&,
        std::span<const double, 3>,
        SimTK::Array_<SimTK::DecorativeGeometry>&)
    {
        // TODO
    }

    template<>
    void generateDecorations<DataPointType::PointForce>(
        const SimTK::State&,
        std::span<const double, 6>,
        SimTK::Array_<SimTK::DecorativeGeometry>&)
    {
        // TODO
    }

    template<>
    void generateDecorations<DataPointType::BodyForce>(
        const SimTK::State&,
        std::span<const double, 3>,
        SimTK::Array_<SimTK::DecorativeGeometry>&)
    {
        // TODO
    }

    template<>
    void generateDecorations<DataPointType::Orientation>(
        const SimTK::State&,
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

    template<>
    void generateDecorations<DataPointType::Unknown>(
        const SimTK::State&,
        std::span<const double, 1>,
        SimTK::Array_<SimTK::DecorativeGeometry>&)
    {
        // (do nothing)
    }

    void generateDecorations(
        const SimTK::State&,
        const OpenSim::Storage&,
        const DataSeriesAnnotation&,
        SimTK::Array_<SimTK::DecorativeGeometry>&)
    {
        // TODO
    }
}

// Specialized `OpenSim::Component`s for this UI
namespace
{
    // Refers to one data series within one annotated motion.
    class DataSeries final : public OpenSim::ModelComponent {
    public:
        explicit DataSeries(
            const std::shared_ptr<const OpenSim::Storage>& storage,
            const DataSeriesAnnotation& annotation) :

            m_Storage{storage},
            m_Annotation{annotation}
        {}
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
        OpenSim_DECLARE_CONCRETE_OBJECT(AnnotatedMotion, OpenSim::ModelComponent);
    public:

        // Returns an `AnnotationMotion` that was loaded from the given filesystem
        // path, or throws an `std::exception` if any error occurs.
        static AnnotatedMotion fromFile(const std::filesystem::path& path)
        {
            return AnnotatedMotion{OpenSim::Storage{path.string()}};
        }
    private:
        explicit AnnotatedMotion(OpenSim::Storage storage) :
            m_Storage{std::move(storage)},
            m_Schema{StorageSchema::parse(m_Storage)}
        {}

        OpenSim::Storage m_Storage;
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
    private:
        std::shared_ptr<UndoableModelStatePair> m_Model = std::make_shared<UndoableModelStatePair>();
        double m_ScrubTime = 0.0;
    };
}

class osc::PreviewExperimentalDataTab::Impl final : public StandardTabImpl {
public:
    explicit Impl(const ParentPtr<ITabHost>&) :
        StandardTabImpl{ICON_FA_DOT_CIRCLE " Experimental Data"}
    {
        m_PanelManager->register_toggleable_panel(
            "Navigator",
            [this](std::string_view panelName)
            {
                return std::make_shared<NavigatorPanel>(
                    panelName,
                    m_UiState->updSharedModelPtr(),
                    [this](const OpenSim::ComponentPath&) {}
                );
            }
        );
        m_PanelManager->register_spawnable_panel(
            "viewer",
            [this](std::string_view panelName)
            {
                auto onRightClick = [](const ModelEditorViewerPanelRightClickEvent&) {};
                ModelEditorViewerPanelParameters panelParams{m_UiState->updSharedModelPtr(), onRightClick};
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
        ui::enable_dockspace_over_main_viewport();
        drawToolbar();
        m_PanelManager->on_draw();
    }

    void drawToolbar()
    {
        if (BeginToolbar("##PreviewExperimentalDataToolbar", Vec2{5.0f, 5.0f}))
        {
            ui::draw_button("todo");
        }
        ui::end_panel();
    }

    std::shared_ptr<PreviewExperimentalDataUiState> m_UiState = std::make_shared<PreviewExperimentalDataUiState>();
    std::shared_ptr<PanelManager> m_PanelManager = std::make_shared<PanelManager>();

    WindowMenu m_WindowMenu{m_PanelManager};
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

bool osc::PreviewExperimentalDataTab::impl_on_event(const SDL_Event& e)
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
