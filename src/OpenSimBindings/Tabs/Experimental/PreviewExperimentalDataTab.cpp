#include "PreviewExperimentalDataTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Color.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Graphics/SceneDecorationFlags.hpp"
#include "src/Graphics/SceneRenderer.hpp"
#include "src/Graphics/SceneRendererParams.hpp"
#include "src/Graphics/ShaderCache.hpp"
#include "src/Maths/BVH.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/Maths/Rect.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Panels/LogViewerPanel.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Platform/os.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/CStringView.hpp"

#include <glm/vec2.hpp>
#include <imgui.h>
#include <IconsFontAwesome5.h>
#include <nonstd/span.hpp>
#include <OpenSim/Common/Storage.h>
#include <SDL_events.h>

#include <functional>
#include <iostream>
#include <regex>
#include <string>
#include <utility>

namespace
{
    // describes the type of data held in a column of the data file
    enum class ColumnDataType {
        Point = 0,
        PointForce,
        BodyForce,
        Orientation,
        Unknown,
        TOTAL,
    };

    // human-readable name of a data type
    static auto constexpr c_ColumnDataTypeStrings = osc::MakeSizedArray<osc::CStringView, static_cast<int>(ColumnDataType::TOTAL)>
    (
        "Point",
        "PointForce",
        "BodyForce",
        "Orientation",
        "Unknown"
    );

    // the number of floating-point values the column is backed by
    static auto constexpr c_ColumnDataSizes = osc::MakeSizedArray<int, static_cast<int>(ColumnDataType::TOTAL)>
    (
        3,
        6,
        3,
        4,
        1
    );

    // prints a human-readable representation of a column data type
    std::ostream& operator<<(std::ostream& o, ColumnDataType dt)
    {
        return o << c_ColumnDataTypeStrings.at(static_cast<size_t>(dt));
    }

    // returns the number of floating-point values the column is backed by
    constexpr int NumElementsIn(ColumnDataType dt)
    {
        return c_ColumnDataSizes.at(static_cast<size_t>(dt));
    }

    // a struct that describes how a sequence of N column labels matches up to a column data type (with size N)
    struct ColumnDataTypeMatcher final {

        ColumnDataTypeMatcher(
            ColumnDataType t_,
            std::vector<osc::CStringView> suffixes_) :

            columnDataType{std::move(t_)},
            suffixes{std::move(suffixes_)}
        {
            OSC_ASSERT(suffixes.size() == NumElementsIn(columnDataType));
            OSC_ASSERT(!suffixes.empty());
        }

        ColumnDataType columnDataType;
        std::vector<osc::CStringView> suffixes;
    };

    // a sequence of matchers to test against
    //
    // if the next N columns don't match any matchers, assume the column is `ColumnDataType::Unknown`
    std::vector<ColumnDataTypeMatcher> const& GetMatchers()
    {
        static std::vector<ColumnDataTypeMatcher> const s_Matchers =
        {
            {
                ColumnDataType::PointForce,
                {"_vx", "_vy", "_vz", "_px", "_py", "_pz"}
                },
            {
                ColumnDataType::Point,
                {"_vx", "_vy", "_vz"}
            },
            {
                ColumnDataType::Point,
                {"_tx", "_ty", "_tz"}
            },
            {
                ColumnDataType::Point,
                {"_px", "_py", "_pz"}
            },
            {
                ColumnDataType::Orientation,
                {"_1", "_2", "_3", "_4"}
            },
            {
                ColumnDataType::Point,
                {"_1", "_2", "_3"}
            },
            {
                ColumnDataType::BodyForce,
                {"_fx", "_fy", "_fz"}
            },
        };
        return s_Matchers;
    }

    // returns the number of columns the data type would require
    constexpr int NumColumnsRequiredBy(ColumnDataTypeMatcher const& matcher)
    {
        return NumElementsIn(matcher.columnDataType);
    }

    // describes the layout of a single column parsed from the data file
    struct ColumnDescription final {

        ColumnDescription(
            int offset_,
            std::string label_,
            ColumnDataType dataType_) :

            offset{std::move(offset_)},
            label{std::move(label_)},
            dataType{std::move(dataType_)}
        {
        }

        int offset;
        std::string label;
        ColumnDataType dataType;
    };

    // prints a human-readable representation of a column description
    std::ostream& operator<<(std::ostream& o, ColumnDescription const& desc)
    {
        return o << "ColumnDescription(Offset=" << desc.offset << ", DataType = " << desc.dataType << ", Label = \"" << desc.label << "\")";
    }

    // returns true if `s` ends with the given `suffix`
    bool EndsWith(std::string const& s, std::string_view suffix)
    {
        if (suffix.length() > s.length())
        {
            return false;
        }

        return s.compare(s.length() - suffix.length(), suffix.length(), suffix) == 0;
    }

    // returns `true` if the provided labels [offset..offset+matcher.Suffixes.size()] all match up
    bool IsMatch(OpenSim::Array<std::string> const& labels, int offset, ColumnDataTypeMatcher const& matcher)
    {
        int colsRemaining = labels.size() - offset;
        int numColsToTest = NumColumnsRequiredBy(matcher);

        if (numColsToTest > colsRemaining)
        {
            return false;
        }

        for (int i = 0; i < numColsToTest; ++i)
        {
            std::string const& label = labels[offset + i];
            osc::CStringView requiredSuffix = matcher.suffixes[i];

            if (!EndsWith(label, requiredSuffix))
            {
                return false;
            }
        }
        return true;
    }

    // returns the matching column data type for the next set of columns - if a match can be found
    std::optional<ColumnDataTypeMatcher> TryMatchColumnsWithType(OpenSim::Array<std::string> const& labels, int offset)
    {
        for (ColumnDataTypeMatcher const& matcher : GetMatchers())
        {
            if (IsMatch(labels, offset, matcher))
            {
                return matcher;
            }
        }
        return std::nullopt;
    }

    // returns a string that has had the
    std::string RemoveLastNCharacters(std::string const& s, size_t n)
    {
        if (n > s.length())
        {
            return {};
        }

        return s.substr(0, s.length() - n);
    }

    // returns a sequence of parsed column descriptions, based on header labels
    std::vector<ColumnDescription> ParseColumnDescriptions(OpenSim::Array<std::string> const& labels)
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
    size_t NumRows(LoadedMotion const& lm)
    {
        return lm.data.size() / lm.rowStride;
    }

    // prints `LoadedMotion` in a human-readable format
    std::ostream& operator<<(std::ostream& o, LoadedMotion const& mot)
    {
        o << "LoadedMotion(";
        o << "\n    ColumnDescriptions = [";
        for (ColumnDescription const& d : mot.columnDescriptions)
        {
            o << "\n        " << d;
        }
        o << "\n    ],";
        o << "\n    RowStride = " << mot.rowStride << ',';
        o << "\n    Data = [... " << mot.data.size() << " values (" << NumRows(mot) << " rows)...]";
        o << "\n)";

        return o;
    }

    // get the time value for a given row
    double GetTime(LoadedMotion const& m, size_t row)
    {
        return m.data.at(row * m.rowStride);
    }

    // get data values for a given row
    nonstd::span<double const> GetData(LoadedMotion const& m, size_t row)
    {
        OSC_ASSERT((row + 1) * m.rowStride <= m.data.size());

        size_t start = (row * m.rowStride) + 1;
        size_t numValues = m.rowStride - 1;

        return nonstd::span<double const>(m.data.data() + start, numValues);
    }

    // compute the stride of the data columns
    size_t CalcDataStride(nonstd::span<ColumnDescription const> descriptions)
    {
        size_t sum = 0;
        for (ColumnDescription const& d : descriptions)
        {
            sum += NumElementsIn(d.dataType);
        }
        return sum;
    }

    // compute the total row stride (time + data columns)
    size_t CalcRowStride(nonstd::span<ColumnDescription const> descriptions)
    {
        return 1 + CalcDataStride(descriptions);
    }

    // load raw row values from an OpenSim::Storage class
    std::vector<double> LoadRowValues(OpenSim::Storage const& storage, size_t rowStride)
    {
        size_t numDataCols = rowStride - 1;
        int numRows = storage.getSize();
        OSC_ASSERT(numRows > 0);

        std::vector<double> rv;
        rv.reserve(numRows * rowStride);

        // pack the data into the data vector
        for (int row = 0; row < storage.getSize(); ++row)
        {
            OpenSim::StateVector const& v = *storage.getStateVector(row);
            double t = v.getTime();
            OpenSim::Array<double> const& vs = v.getData();
            size_t numCols = std::min(static_cast<size_t>(v.getSize()), numDataCols);

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
    using DecorationConsumer = std::function<void(osc::SceneDecoration const&)>;

    // retuns a scene decoration for the floor grid
    osc::SceneDecoration GenerateFloorGrid()
    {
        osc::Transform t;
        t.rotation = glm::angleAxis(osc::fpi2, glm::vec3{-1.0f, 0.0f, 0.0f});
        t.scale = {50.0f, 50.0f, 1.0f};
        osc::Color const color = {128.0f/255.0f, 128.0f/255.0f, 128.0f/255.0f, 1.0f};

        return osc::SceneDecoration
        {
            osc::App::singleton<osc::MeshCache>()->get100x100GridMesh(),
            t,
            color,
            std::string{},
            osc::SceneDecorationFlags_None
        };
    }

    // high-level caller-provided description of an arrow that they would like to generate
    // decorations for
    struct DecorativeArrow final {

        DecorativeArrow() = default;

        glm::vec3 p0 = {};
        glm::vec3 p1 = {};
        osc::Color color = osc::Color::white();
        float neckThickness = 0.025f;
        float headThickness = 0.05f;
        float percentageHead = 0.15f;
        std::string label;
    };

    // writes relevant geometry to output for drawing an arrow between two points in space
    void GenerateDecorations(DecorativeArrow const& arrow, DecorationConsumer& out)
    {
        // calculate arrow vectors/directions
        const glm::vec3 startToFinishVec = arrow.p1 - arrow.p0;
        const float startToFinishLength = glm::length(startToFinishVec);
        const glm::vec3 startToFinishDir = (1.0f / startToFinishLength) * startToFinishVec;

        // calculate arrow length in worldspace
        const float neckPercentage = 1.0f - arrow.percentageHead;
        const float neckLength = neckPercentage * startToFinishLength;
        const float headLength = arrow.percentageHead * startToFinishLength;

        // calculate mesh-to-arrow rotation (meshes point along Y)
        const glm::quat rotation = glm::rotation(glm::vec3{0.0f, 1.0f, 0.0f}, startToFinishDir);

        // calculate arrow (head/neck) midpoints for translation
        const float neckMidpointPercentage = 0.5f * neckPercentage;
        const glm::vec3 neckMidpoint = arrow.p0 + (neckMidpointPercentage * startToFinishVec);
        const float headMidpointPercentage = 0.5f * (1.0f + neckPercentage);
        const glm::vec3 headMidpoint = arrow.p0 + (headMidpointPercentage * startToFinishVec);

        // emit neck (note: meshes have a height of 2 in mesh-space)
        {
            osc::Transform t;
            t.scale = {arrow.neckThickness, 0.5f * neckLength, arrow.neckThickness};
            t.rotation = rotation;
            t.position = neckMidpoint;

            out(osc::SceneDecoration
            {
                osc::App::singleton<osc::MeshCache>()->getCylinderMesh(),
                t,
                arrow.color,
                arrow.label,
                osc::SceneDecorationFlags_None,
            });
        }

        // emit head (note: meshes have a height of 2 in mesh-space)
        {
            osc::Transform t;
            t.scale = {arrow.headThickness, 0.5f * headLength, arrow.headThickness};
            t.rotation = rotation;
            t.position = headMidpoint;

            out(osc::SceneDecoration
            {
                osc::App::singleton<osc::MeshCache>()->getConeMesh(),
                t,
                arrow.color,
                arrow.label,
                osc::SceneDecorationFlags_None,
            });
        }
    }

    // templated: generate decorations for a compile-time-known type of column data (requires specialization)
    template<ColumnDataType T>
    void GenerateDecorations(LoadedMotion const& motion, size_t row, ColumnDescription const& columnDescription, DecorationConsumer& out);

    // template specialization: generate decorations for orientation data
    template<>
    void GenerateDecorations<ColumnDataType::Orientation>(LoadedMotion const& motion, size_t row, ColumnDescription const& columnDescription, DecorationConsumer& out)
    {
        OSC_ASSERT(columnDescription.dataType == ColumnDataType::Orientation);

        size_t const dataStart = motion.rowStride * row + columnDescription.offset;
        glm::quat q
        {
            static_cast<float>(motion.data.at(dataStart)),
            static_cast<float>(motion.data.at(dataStart + 1)),
            static_cast<float>(motion.data.at(dataStart + 2)),
            static_cast<float>(motion.data.at(dataStart + 3)),
        };
        q = glm::normalize(q);

        // draw Y axis arrow
        DecorativeArrow arrow;
        arrow.p0 = {0.0f, 0.0f, 0.0f};
        arrow.p1 = q * glm::vec3{0.0f, 1.0f, 0.0f};
        arrow.color = {0.0f, 1.0f, 0.0f, 1.0f};
        arrow.label = columnDescription.label;

        GenerateDecorations(arrow, out);
    }

    // generic: generate decorations for a runtime-checked type of column data
    void GenerateDecorations(LoadedMotion const& motion, size_t row, ColumnDescription const& desc, DecorationConsumer& out)
    {
        if (desc.dataType == ColumnDataType::Orientation)
        {
            GenerateDecorations<ColumnDataType::Orientation>(motion, row, desc, out);
        }
    }

    // generate decorations for a all columns of a particular row in the provided motion data
    void GenerateDecorations(LoadedMotion const& motion, size_t row, DecorationConsumer& out)
    {
        // generate decorations for each "column" in the row
        for (ColumnDescription const& desc : motion.columnDescriptions)
        {
            GenerateDecorations(motion, row, desc, out);
        }
    }

    // returns a parsed motion, read from disk motion from disk
    LoadedMotion LoadData(std::filesystem::path const& sourceFile)
    {
        OpenSim::Storage const storage{sourceFile.string()};

        LoadedMotion rv;
        rv.columnDescriptions = ParseColumnDescriptions(storage.getColumnLabels());
        rv.rowStride = CalcRowStride(rv.columnDescriptions);
        rv.data = LoadRowValues(storage, rv.rowStride);
        return rv;
    }

    // tries to load the given path; otherwise, falls back to asking the user for a file
    LoadedMotion TryLoadOrPrompt(std::filesystem::path const& sourceFile)
    {
        if (std::filesystem::exists(sourceFile))
        {
            return LoadData(sourceFile);
        }
        else
        {
            std::optional<std::filesystem::path> const maybePath = osc::PromptUserForFile("sto,mot");
            if (maybePath)
            {
                return LoadData(*maybePath);
            }
            else
            {
                return LoadedMotion{};
            }
        }
    }

    // annotations associated with the current scene (what's selected, etc.)
    struct SceneAnnotations final {
        std::string hovered;
        std::string selected;
    };
}

// osc::Tab implementation for the visualizer
class osc::PreviewExperimentalDataTab::Impl final {
public:

    Impl()
    {
        log::info("%s", StreamToString(m_Motion).c_str());
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return ICON_FA_DOT_CIRCLE " Experimental Data";
    }

    void onDraw()
    {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        ImGui::Begin("render");
        glm::vec2 dims = ImGui::GetContentRegionAvail();
        if (m_RenderIsMousedOver)
        {
            osc::UpdatePolarCameraFromImGuiMouseInputs(dims, m_Camera);
        }

        if (m_ActiveRow < NumRows(*m_Motion))
        {
            osc::DrawTextureAsImGuiImage(render3DScene(dims), dims);
            m_RenderIsMousedOver = ImGui::IsItemHovered();
        }
        else
        {
            ImGui::Text("no rows found in the given data? Cannot render");
            m_RenderIsMousedOver = false;
        }

        ImGui::End();

        m_LogViewer.draw();
    }


private:
    RenderTexture& render3DScene(glm::vec2 dims)
    {
        SceneRendererParams params = generateRenderParams(dims);

        if (params != m_LastRendererParams)
        {
            generateSceneDecorations();
            m_Renderer.draw(m_Decorations, params);
            m_LastRendererParams = params;
        }

        return m_Renderer.updRenderTexture();
    }

    SceneRendererParams generateRenderParams(glm::vec2 dims) const
    {
        SceneRendererParams params{m_LastRendererParams};
        params.dimensions = dims;
        params.samples = osc::App::get().getMSXAASamplesRecommended();
        params.drawRims = true;
        params.drawFloor = false;
        params.viewMatrix = m_Camera.getViewMtx();
        params.projectionMatrix = m_Camera.getProjMtx(AspectRatio(params.dimensions));
        params.nearClippingPlane = m_Camera.znear;
        params.farClippingPlane = m_Camera.zfar;
        params.viewPos = m_Camera.getPos();
        params.lightDirection = osc::RecommendedLightDirection(m_Camera);
        params.lightColor = Color::white();
        params.backgroundColor = {96.0f / 255.0f, 96.0f / 255.0f, 96.0f / 255.0f, 1.0f};
        return params;
    }

    void generateSceneDecorations()
    {
        m_Decorations.clear();
        m_Decorations.push_back(GenerateFloorGrid());
        DecorationConsumer c = [this](SceneDecoration const& d) { m_Decorations.push_back(d); };
        GenerateDecorations(*m_Motion, m_ActiveRow, c);
        UpdateSceneBVH(m_Decorations, m_SceneBVH);
    }

    void updateScene3DHittest()
    {
        if (!m_RenderIsMousedOver)
        {
            return;  // only hittest while the user is moused over the viewport
        }

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) ||
            ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ||
            ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            return;  // don't hittest while a user is dragging around
        }

        // get camera ray
        // intersect it with scene
        // get closest collision
        // use it to set scene annotations based on whether user is clicking or not
    }

    UID m_TabID;

    // scene state
    std::shared_ptr<LoadedMotion const> m_Motion = std::make_shared<LoadedMotion>(TryLoadOrPrompt(R"(E:\OneDrive\work_current\Gijs - IMU fitting\abduction_bad2.sto)"));
    int m_ActiveRow = NumRows(*m_Motion) <= 0 ? -1 : 0;

    // extra scene state
    SceneAnnotations m_Annotations;

    // rendering state
    std::vector<SceneDecoration> m_Decorations;
    BVH m_SceneBVH;
    PolarPerspectiveCamera m_Camera;
    SceneRendererParams m_LastRendererParams;
    SceneRenderer m_Renderer
    {
        App::config(),
        *App::singleton<MeshCache>(),
        *App::singleton<ShaderCache>()
    };
    bool m_RenderIsMousedOver = false;

    // 2D UI state
    LogViewerPanel m_LogViewer{"Log"};
};


// public API (PIMPL)

osc::CStringView osc::PreviewExperimentalDataTab::id() noexcept
{
    return "OpenSim/PreviewExperimentalData";
}

osc::PreviewExperimentalDataTab::PreviewExperimentalDataTab(std::weak_ptr<TabHost>) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::PreviewExperimentalDataTab::PreviewExperimentalDataTab(PreviewExperimentalDataTab&&) noexcept = default;
osc::PreviewExperimentalDataTab& osc::PreviewExperimentalDataTab::operator=(PreviewExperimentalDataTab&&) noexcept = default;
osc::PreviewExperimentalDataTab::~PreviewExperimentalDataTab() noexcept = default;

osc::UID osc::PreviewExperimentalDataTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::PreviewExperimentalDataTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::PreviewExperimentalDataTab::implOnDraw()
{
    m_Impl->onDraw();
}
