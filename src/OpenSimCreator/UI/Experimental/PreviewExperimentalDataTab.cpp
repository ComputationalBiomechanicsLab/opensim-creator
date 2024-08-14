#include "PreviewExperimentalDataTab.h"

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
#include <oscar/UI/Tabs/StandardTabImpl.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/StringHelpers.h>

#include <IconsFontAwesome5.h>
#include <OpenSim/Common/Storage.h>
#include <SDL_events.h>

#include <array>
#include <functional>
#include <iostream>
#include <regex>
#include <span>
#include <string>
#include <utility>

using namespace osc;
using namespace osc::literals;

namespace
{
    // describes the type of data held in a column of the data file
    enum class ColumnDataType {
        Point = 0,
        PointForce,
        BodyForce,
        Orientation,
        Unknown,
        NUM_OPTIONS,
    };

    // human-readable name of a data type
    constexpr auto c_ColumnDataTypeStrings = std::to_array<CStringView>({
        "Point",
        "PointForce",
        "BodyForce",
        "Orientation",
        "Unknown",
    });
    static_assert(c_ColumnDataTypeStrings.size() == num_options<ColumnDataType>());

    // the number of floating-point values the column is backed by
    constexpr auto c_ColumnDataSizes = std::to_array<int>({
        3,
        6,
        3,
        4,
        1,
    });
    static_assert(c_ColumnDataSizes.size() == num_options<ColumnDataType>());

    // returns the number of floating-point values the column is backed by
    constexpr int NumElementsIn(ColumnDataType dt)
    {
        return c_ColumnDataSizes.at(static_cast<size_t>(dt));
    }

    // a struct that describes how a sequence of N column labels matches up to a column data type (with size N)
    struct ColumnDataTypeMatcher final {

        ColumnDataTypeMatcher(
            ColumnDataType t_,
            std::vector<CStringView> suffixes_) :

            columnDataType{t_},
            suffixes{std::move(suffixes_)}
        {
            OSC_ASSERT(suffixes.size() == static_cast<size_t>(NumElementsIn(columnDataType)));
            OSC_ASSERT(!suffixes.empty());
        }

        ColumnDataType columnDataType;
        std::vector<CStringView> suffixes;
    };

    // a sequence of matchers to test against
    //
    // if the next N columns don't match any matchers, assume the column is `ColumnDataType::Unknown`
    const std::vector<ColumnDataTypeMatcher>& GetMatchers()
    {
        static const std::vector<ColumnDataTypeMatcher> s_Matchers = {
            {
                ColumnDataType::PointForce,
                {"_vx", "_vy", "_vz", "_px", "_py", "_pz"},
            },
            {
                ColumnDataType::Point,
                {"_vx", "_vy", "_vz"},
            },
            {
                ColumnDataType::Point,
                {"_tx", "_ty", "_tz"},
            },
            {
                ColumnDataType::Point,
                {"_px", "_py", "_pz"},
            },
            {
                ColumnDataType::Orientation,
                {"_1", "_2", "_3", "_4"},
            },
            {
                ColumnDataType::Point,
                {"_1", "_2", "_3"},
            },
            {
                ColumnDataType::BodyForce,
                {"_fx", "_fy", "_fz"},
            },
        };
        return s_Matchers;
    }

    // returns the number of columns the data type would require
    constexpr int NumColumnsRequiredBy(const ColumnDataTypeMatcher& matcher)
    {
        return NumElementsIn(matcher.columnDataType);
    }

    // describes the layout of a single column parsed from the data file
    struct ColumnDescription final {

        ColumnDescription(
            int offset_,
            std::string label_,
            ColumnDataType dataType_) :

            offset{offset_},
            label{std::move(label_)},
            dataType{dataType_}
        {}

        int offset;
        std::string label;
        ColumnDataType dataType;
    };

    // returns true if `s` ends with the given `suffix`
    bool EndsWith(const std::string& s, std::string_view suffix)
    {
        if (suffix.length() > s.length())
        {
            return false;
        }

        return s.compare(s.length() - suffix.length(), suffix.length(), suffix) == 0;
    }

    // returns `true` if the provided labels [offset..offset+matcher.Suffixes.size()] all match up
    bool IsMatch(const OpenSim::Array<std::string>& labels, int offset, const ColumnDataTypeMatcher& matcher)
    {
        int colsRemaining = labels.size() - offset;
        int numColsToTest = NumColumnsRequiredBy(matcher);

        if (numColsToTest > colsRemaining)
        {
            return false;
        }

        for (int i = 0; i < numColsToTest; ++i)
        {
            const std::string& label = labels[offset + i];
            CStringView requiredSuffix = matcher.suffixes[i];

            if (!EndsWith(label, requiredSuffix))
            {
                return false;
            }
        }
        return true;
    }

    // returns the matching column data type for the next set of columns - if a match can be found
    std::optional<ColumnDataTypeMatcher> TryMatchColumnsWithType(const OpenSim::Array<std::string>& labels, int offset)
    {
        for (const ColumnDataTypeMatcher& matcher : GetMatchers())
        {
            if (IsMatch(labels, offset, matcher))
            {
                return matcher;
            }
        }
        return std::nullopt;
    }

    // returns a string that has had the
    std::string RemoveLastNCharacters(const std::string& s, size_t n)
    {
        if (n > s.length())
        {
            return {};
        }

        return s.substr(0, s.length() - n);
    }

    // returns a sequence of parsed column descriptions, based on header labels
    std::vector<ColumnDescription> ParseColumnDescriptions(const OpenSim::Array<std::string>& labels)
    {
        std::vector<ColumnDescription> rv;
        int offset = 1;  // offset 0 == "time" (skip it)

        while (offset < labels.size())
        {
            if (std::optional<ColumnDataTypeMatcher> match = TryMatchColumnsWithType(labels, offset))
            {
                std::string baseName = RemoveLastNCharacters(labels[offset], match->suffixes.front().size());

                rv.emplace_back(offset, baseName, match->columnDataType);
                offset += NumElementsIn(match->columnDataType);
            }
            else
            {
                rv.emplace_back(offset, labels[offset], ColumnDataType::Unknown);
                offset += 1;
            }
        }
        return rv;
    }

    // motions that were parsed from the file
    struct LoadedMotion final {
        std::vector<ColumnDescription> columnDescriptions;
        size_t rowStride = 1;
        std::vector<double> data;
    };

    // returns the number of rows a loaded motion has
    size_t NumRows(const LoadedMotion& lm)
    {
        return lm.data.size() / lm.rowStride;
    }

    // compute the stride of the data columns
    size_t CalcDataStride(std::span<const ColumnDescription> descriptions)
    {
        size_t sum = 0;
        for (const ColumnDescription& d : descriptions)
        {
            sum += NumElementsIn(d.dataType);
        }
        return sum;
    }

    // compute the total row stride (time + data columns)
    size_t CalcRowStride(std::span<const ColumnDescription> descriptions)
    {
        return 1 + CalcDataStride(descriptions);
    }

    // load raw row values from an OpenSim::Storage class
    std::vector<double> LoadRowValues(const OpenSim::Storage& storage, size_t rowStride)
    {
        size_t numDataCols = rowStride - 1;
        int numRows = storage.getSize();
        OSC_ASSERT(numRows > 0);

        std::vector<double> rv;
        rv.reserve(numRows * rowStride);

        // pack the data into the data vector
        for (int row = 0; row < storage.getSize(); ++row)
        {
            const OpenSim::StateVector& v = *storage.getStateVector(row);
            double t = v.getTime();
            const OpenSim::Array<double>& vs = v.getData();
            size_t numCols = min(static_cast<size_t>(v.getSize()), numDataCols);

            rv.push_back(t);
            for (size_t col = 0; col < numCols; ++col)
            {
                rv.push_back(vs[static_cast<int>(col)]);
            }
            for (size_t missing = numCols; missing < numDataCols; ++missing)
            {
                rv.push_back(0.0);  // pack any missing values
            }
        }
        OSC_ASSERT(rv.size() == numRows * rowStride);

        return rv;
    }

    // defines a "consumer" that "eats" decorations emitted from the various helper methods
    using DecorationConsumer = std::function<void(const SceneDecoration&)>;

    // retuns a scene decoration for the floor grid
    SceneDecoration GenerateFloorGrid()
    {
        return SceneDecoration{
            .mesh = App::singleton<SceneCache>(App::resource_loader())->grid_mesh(),
            .transform = {
                .scale = {50.0f, 50.0f, 1.0f},
                .rotation = angle_axis(180_deg, Vec3{-1.0f, 0.0f, 0.0f}),
            },
            .color = Color::half_grey(),
            .flags = SceneDecorationFlag::None,
        };
    }

    // high-level caller-provided description of an arrow that they would like to generate
    // decorations for
    struct DecorativeArrow final {

        DecorativeArrow() = default;

        Vec3 p0 = {};
        Vec3 p1 = {};
        Color color = Color::white();
        float neck_thickness = 0.025f;
        float head_thickness = 0.05f;
        float percentageHead = 0.15f;
        std::string label;
    };

    // writes relevant geometry to output for drawing an arrow between two points in space
    void GenerateDecorations(const DecorativeArrow& arrow, DecorationConsumer& out)
    {
        // calculate arrow vectors/directions
        const Vec3 startToFinishVec = arrow.p1 - arrow.p0;
        const float startToFinishLength = length(startToFinishVec);
        const Vec3 startToFinishDir = (1.0f / startToFinishLength) * startToFinishVec;

        // calculate arrow length in worldspace
        const float neckPercentage = 1.0f - arrow.percentageHead;
        const float neckLength = neckPercentage * startToFinishLength;
        const float headLength = arrow.percentageHead * startToFinishLength;

        // calculate mesh-to-arrow rotation (meshes point along Y)
        const Quat rotation = osc::rotation(Vec3{0.0f, 1.0f, 0.0f}, startToFinishDir);

        // calculate arrow (head/neck) midpoints for translation
        const float neckMidpointPercentage = 0.5f * neckPercentage;
        const Vec3 neckMidpoint = arrow.p0 + (neckMidpointPercentage * startToFinishVec);
        const float headMidpointPercentage = 0.5f * (1.0f + neckPercentage);
        const Vec3 headMidpoint = arrow.p0 + (headMidpointPercentage * startToFinishVec);

        // emit neck (note: meshes have a height of 2 in mesh-space)
        {
            out(SceneDecoration{
                .mesh = App::singleton<SceneCache>(App::resource_loader())->cylinder_mesh(),
                .transform = {
                    .scale = {arrow.neck_thickness, 0.5f * neckLength, arrow.neck_thickness},
                    .rotation = rotation,
                    .position = neckMidpoint,
                },
                .color = arrow.color,
                .id = arrow.label,
                .flags = SceneDecorationFlag::None,
            });
        }

        // emit head (note: meshes have a height of 2 in mesh-space)
        {
            out(SceneDecoration{
                .mesh = App::singleton<SceneCache>(App::resource_loader())->cone_mesh(),
                .transform = {
                    .scale = {arrow.head_thickness, 0.5f * headLength, arrow.head_thickness},
                    .rotation = rotation,
                    .position = headMidpoint,
                },
                .color = arrow.color,
                .id = arrow.label,
                .flags = SceneDecorationFlag::None,
            });
        }
    }

    // templated: generate decorations for a compile-time-known type of column data (requires specialization)
    template<ColumnDataType T>
    void GenerateDecorations(const LoadedMotion& motion, size_t row, const ColumnDescription& columnDescription, DecorationConsumer& out);

    // template specialization: generate decorations for orientation data
    template<>
    void GenerateDecorations<ColumnDataType::Orientation>(const LoadedMotion& motion, size_t row, const ColumnDescription& columnDescription, DecorationConsumer& out)
    {
        OSC_ASSERT(columnDescription.dataType == ColumnDataType::Orientation);

        const size_t dataStart = motion.rowStride * row + columnDescription.offset;
        Quat q
        {
            static_cast<float>(motion.data.at(dataStart)),
            static_cast<float>(motion.data.at(dataStart + 1)),
            static_cast<float>(motion.data.at(dataStart + 2)),
            static_cast<float>(motion.data.at(dataStart + 3)),
        };
        q = normalize(q);

        // draw Y axis arrow
        DecorativeArrow arrow;
        arrow.p0 = {0.0f, 0.0f, 0.0f};
        arrow.p1 = q * Vec3{0.0f, 1.0f, 0.0f};
        arrow.color = Color::green();
        arrow.label = columnDescription.label;

        GenerateDecorations(arrow, out);
    }

    // generic: generate decorations for a runtime-checked type of column data
    void GenerateDecorations(const LoadedMotion& motion, size_t row, const ColumnDescription& desc, DecorationConsumer& out)
    {
        if (desc.dataType == ColumnDataType::Orientation)
        {
            GenerateDecorations<ColumnDataType::Orientation>(motion, row, desc, out);
        }
    }

    // generate decorations for a all columns of a particular row in the provided motion data
    void GenerateDecorations(const LoadedMotion& motion, size_t row, DecorationConsumer& out)
    {
        // generate decorations for each "column" in the row
        for (const ColumnDescription& desc : motion.columnDescriptions)
        {
            GenerateDecorations(motion, row, desc, out);
        }
    }

    // returns a parsed motion, read from disk motion from disk
    [[maybe_unused]] LoadedMotion LoadData(const std::filesystem::path& sourceFile)
    {
        const OpenSim::Storage storage{sourceFile.string()};

        LoadedMotion rv;
        rv.columnDescriptions = ParseColumnDescriptions(storage.getColumnLabels());
        rv.rowStride = CalcRowStride(rv.columnDescriptions);
        rv.data = LoadRowValues(storage, rv.rowStride);
        return rv;
    }

    // annotations associated with the current scene (what's selected, etc.)
    struct SceneAnnotations final {
        std::string hovered;
        std::string selected;
    };
}

// osc::Tab implementation for the visualizer
class osc::PreviewExperimentalDataTab::Impl final : public StandardTabImpl {
public:

    explicit Impl(const ParentPtr<ITabHost>&) :
        StandardTabImpl{ICON_FA_DOT_CIRCLE " Experimental Data"}
    {}

private:
    void impl_on_draw() final
    {
        ui::enable_dockspace_over_viewport(ui::get_main_viewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        ui::begin_panel("render");
        Vec2 dims = ui::get_content_region_available();
        if (m_RenderIsMousedOver)
        {
            ui::update_polar_camera_from_mouse_inputs(m_Camera, dims);
        }

        if (static_cast<size_t>(m_ActiveRow) < NumRows(*m_Motion))
        {
            ui::draw_image(render3DScene(dims), dims);
            m_RenderIsMousedOver = ui::is_item_hovered();
        }
        else
        {
            ui::draw_text("no rows found in the given data? Cannot render");
            m_RenderIsMousedOver = false;
        }

        ui::end_panel();

        m_LogViewer.on_draw();
    }

    RenderTexture& render3DScene(Vec2 dims)
    {
        SceneRendererParams params = generateRenderParams(dims);

        if (params != m_LastRendererParams)
        {
            generateSceneDecorations();
            m_Renderer.render(m_Decorations, params);
            m_LastRendererParams = params;
        }

        return m_Renderer.upd_render_texture();
    }

    SceneRendererParams generateRenderParams(Vec2 dims) const
    {
        SceneRendererParams params{m_LastRendererParams};
        params.dimensions = dims;
        params.antialiasing_level = App::get().anti_aliasing_level();
        params.draw_rims = true;
        params.draw_floor = false;
        params.view_matrix = m_Camera.view_matrix();
        params.projection_matrix = m_Camera.projection_matrix(aspect_ratio_of(params.dimensions));
        params.near_clipping_plane = m_Camera.znear;
        params.far_clipping_plane = m_Camera.zfar;
        params.view_pos = m_Camera.position();
        params.light_direction = osc::recommended_light_direction(m_Camera);
        params.light_color = Color::white();
        params.background_color = {96.0f / 255.0f, 96.0f / 255.0f, 96.0f / 255.0f, 1.0f};
        return params;
    }

    void generateSceneDecorations()
    {
        m_Decorations.clear();
        m_Decorations.push_back(GenerateFloorGrid());
        DecorationConsumer c = [this](const SceneDecoration& d) { m_Decorations.push_back(d); };
        GenerateDecorations(*m_Motion, m_ActiveRow, c);
        update_scene_bvh(m_Decorations, m_SceneBVH);
    }

    void updateScene3DHittest() const
    {
        if (!m_RenderIsMousedOver)
        {
            return;  // only hittest while the user is moused over the viewport
        }

        if (ui::is_mouse_dragging(ImGuiMouseButton_Left) ||
            ui::is_mouse_dragging(ImGuiMouseButton_Middle) ||
            ui::is_mouse_dragging(ImGuiMouseButton_Right))
        {
            return;  // don't hittest while a user is dragging around
        }

        // get camera ray
        // intersect it with scene
        // get closest collision
        // use it to set scene annotations based on whether user is clicking or not
    }

    // scene state
    std::shared_ptr<const LoadedMotion> m_Motion = std::make_shared<LoadedMotion>();
    int m_ActiveRow = NumRows(*m_Motion) <= 0 ? -1 : 0;

    // extra scene state
    SceneAnnotations m_Annotations;

    // rendering state
    std::vector<SceneDecoration> m_Decorations;
    BVH m_SceneBVH;
    PolarPerspectiveCamera m_Camera;
    SceneRendererParams m_LastRendererParams;
    SceneRenderer m_Renderer{
        *App::singleton<SceneCache>(App::resource_loader()),
    };
    bool m_RenderIsMousedOver = false;

    // 2D UI state
    LogViewerPanel m_LogViewer{"Log"};
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

bool osc::PreviewExperimentalDataTab::impl_on_event(const SDL_Event& e)
{
    return m_Impl->on_event(e);
}

void osc::PreviewExperimentalDataTab::impl_on_draw()
{
    m_Impl->on_draw();
}
