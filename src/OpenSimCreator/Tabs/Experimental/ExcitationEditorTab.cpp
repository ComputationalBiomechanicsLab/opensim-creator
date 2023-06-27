#include "ExcitationEditorTab.hpp"

#include "OpenSimCreator/Model/UndoableModelStatePair.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Maths/CollisionTests.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Panels/PanelManager.hpp>
#include <oscar/Panels/StandardPanel.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>
#include <oscar/Utils/UndoRedo.hpp>

#include <glm/vec2.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <implot.h>
#include <SDL_events.h>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

// top-level globals etc.
namespace
{
    constexpr osc::CStringView c_TabStringID = "ExcitationEditorTab";
}

// document state (model)
namespace
{
    struct LinearlyInterpolatedLineStyle final {};
    using LineStyle = std::variant<LinearlyInterpolatedLineStyle>;

    struct ExcitationCurveSegment {
        explicit ExcitationCurveSegment(
            glm::vec2 startPosition_) :

            startPosition{startPosition_},
            lineStyleToNextPoint{LinearlyInterpolatedLineStyle{}}
        {
        }

        ExcitationCurveSegment(
            glm::vec2 startPosition_,
            LineStyle lineStyleToNextPoint_) :

            startPosition{startPosition_},
            lineStyleToNextPoint{std::move(lineStyleToNextPoint_)}
        {
        }

        glm::vec2 startPosition;
        LineStyle lineStyleToNextPoint;
    };

    struct IDedExcitationCurveSegment : public ExcitationCurveSegment {
        IDedExcitationCurveSegment(
            glm::vec2 startPosition_,
            LineStyle lineStyleToNextPoint_,
            osc::UID id_) :

            ExcitationCurveSegment{startPosition_, std::move(lineStyleToNextPoint_)},
            id{id_}
        {
        }

        osc::UID id;
    };

    bool HasLowerXStartingPosition(ExcitationCurveSegment const& a, ExcitationCurveSegment const& b) noexcept
    {
        return a.startPosition.x < b.startPosition.x;
    }

    std::unordered_map<osc::UID, ExcitationCurveSegment> CreateCurveSegmentLookup(
        std::initializer_list<ExcitationCurveSegment> segments)
    {
        std::unordered_map<osc::UID, ExcitationCurveSegment> rv;
        rv.reserve(segments.size());
        for (ExcitationCurveSegment const& segment : segments)
        {
            rv.insert_or_assign(osc::UID{}, segment);
        }
        return rv;
    }

    class Curve final {
    public:
        explicit Curve(std::initializer_list<ExcitationCurveSegment> curveSegments) :
            m_SegmentsByID{CreateCurveSegmentLookup(curveSegments)}
        {
        }

        void eraseCurveSegmentByID(osc::UID id)
        {
            m_SegmentsByID.erase(id);
        }

        osc::UID addSegment(ExcitationCurveSegment segment)
        {
            osc::UID id;
            m_SegmentsByID.insert_or_assign(id, std::move(segment));
            return id;
        }

        std::vector<IDedExcitationCurveSegment> getIDedUnorderedCurveSegments() const
        {
            std::vector<IDedExcitationCurveSegment> rv;
            rv.reserve(m_SegmentsByID.size());
            for (auto const& [id, segment] : m_SegmentsByID)
            {
                rv.emplace_back(segment.startPosition, segment.lineStyleToNextPoint, id);
            }
            return rv;
        }

    private:
        std::unordered_map<osc::UID, ExcitationCurveSegment> m_SegmentsByID;
    };

    class ExcitationPattern final {
    public:
        osc::CStringView getComponentAbsPath() const
        {
            return m_ComponentAbsPath;
        }

        Curve const& getMinCurve() const
        {
            return m_MinCurve;
        }

        Curve const& getMaxCurve() const
        {
            return m_MaxCurve;
        }

        Curve const& getSignalCurve() const
        {
            return m_SignalCurve;
        }

    private:
        std::string m_ComponentAbsPath;

        Curve m_MinCurve
        {
            ExcitationCurveSegment{{0.0f, 0.0f}},
            ExcitationCurveSegment{{1.0f, 0.0f}},
        };

        Curve m_MaxCurve
        {
            ExcitationCurveSegment{{0.0f, 1.0f}},
            ExcitationCurveSegment{{1.0f, 1.0f}},
        };

        Curve m_SignalCurve
        {
            ExcitationCurveSegment{{0.0f, 0.5f}},
            ExcitationCurveSegment{{1.0f, 0.5f}},
        };
    };

    // constraint form of vector indicating one of 8 directions in 2D space
    //
    // left-handed (screen-space) coordinate system, with Y pointing down and X pointing right
    class GridDirection final {
    public:
        constexpr static GridDirection North() noexcept { return {0, -1}; }
        constexpr static GridDirection NorthEast() noexcept { return {1, -1}; }
        constexpr static GridDirection East() noexcept { return {1, 0}; }
        constexpr static GridDirection SouthEast() noexcept { return {1, 1}; }
        constexpr static GridDirection South() noexcept { return {0, 1}; }
        constexpr static GridDirection SouthWest() noexcept { return {-1, 1}; }
        constexpr static GridDirection West() noexcept { return {-1, 0}; }
        constexpr static GridDirection NorthWest() noexcept { return {-1, -1}; }
    private:
        friend constexpr glm::ivec2 ToVec2(GridDirection const&) noexcept;
        friend constexpr std::optional<GridDirection> FromVec2(glm::ivec2) noexcept;

        constexpr GridDirection(int x, int y) : m_Offset{x, y} {}

        glm::ivec2 m_Offset;
    };

    constexpr glm::ivec2 ToVec2(GridDirection const& d) noexcept
    {
        return d.m_Offset;
    }

    constexpr std::optional<GridDirection> FromVec2(glm::ivec2 v) noexcept
    {
        v = glm::clamp(v, -1, 1);
        return v != glm::ivec2{0, 0} ? GridDirection{v.x, v.y} : std::optional<GridDirection>{};
    }

    constexpr bool IsNorthward(GridDirection const& d) noexcept
    {
        return ToVec2(d).y == -1;
    }

    constexpr bool IsEastward(GridDirection const& d) noexcept
    {
        return ToVec2(d).x == 1;
    }

    constexpr bool IsSouthward(GridDirection const& d) noexcept
    {
        return ToVec2(d).y == 1;
    }

    constexpr bool IsWestward(GridDirection const& d) noexcept
    {
        return ToVec2(d).x == -1;
    }

    constexpr bool IsDiagonal(GridDirection const& d) noexcept
    {
        glm::ivec2 const v = ToVec2(d);
        return v.x*v.y != 0;
    }

    enum GridOperation {
        None,
        Move,
        Swap,
        Add,
    };

    class GridLayout final {
    public:
        size_t getNumRows() const
        {
            if (m_NumColumns != 0)
            {
                return (m_Cells.size() + (m_NumColumns - 1)) / m_NumColumns;
            }
            else
            {
                return 0;
            }
        }

        size_t getNumColumns() const
        {
            return m_NumColumns;
        }

        glm::ivec2 getDimensions() const
        {
            return glm::ivec2
            {
                static_cast<glm::ivec2::value_type>(getNumColumns()),
                static_cast<glm::ivec2::value_type>(getNumRows()),
            };
        }

        osc::UID getCellID(glm::ivec2 coord) const
        {
            return m_Cells.at(toCellIndex(coord));
        }

        void setCellID(glm::ivec2 coord, osc::UID newID)
        {
            m_Cells.at(toCellIndex(coord)) = newID;
        }

        GridOperation calcAvaliableDirectionalOperation(glm::ivec2 gridCoord, GridDirection direction) const
        {
            return lookupDirectionalOperation(gridCoord, direction).operationType;
        }

        void doDirectionalOperation(glm::ivec2 gridCoord, GridDirection direction)
        {
            lookupDirectionalOperation(gridCoord, direction).execute(*this, gridCoord, direction);
        }

        void removeCell(osc::UID id)
        {
            auto const it = std::find(m_Cells.begin(), m_Cells.end(), id);
            if (it != m_Cells.end())
            {
                *it = osc::UID::empty();
            }
        }
    private:
        static void NoneDirectionalOp(
            GridLayout&,
            glm::ivec2,
            GridDirection)
        {
            // noop
        }

        static void MoveDirectionalOp(
            GridLayout&,
            glm::ivec2,
            GridDirection)
        {
            //glm::ivec2 const adjacentCoord = gridCoord + ToVec2(direction);

            // TODO: insert
        }

        static void SwapDirectionalOp(
            GridLayout&,
            glm::ivec2,
            GridDirection)
        {
            // TODO: swapsies
        }

        static void AddDirectionalOp(GridLayout& grid, glm::ivec2 gridCoord, GridDirection direction)
        {
            glm::ivec2 const adjacentCoord = gridCoord + ToVec2(direction);
            if (adjacentCoord.x >= grid.getNumColumns())
            {
                grid.addColumnToRight();
            }
            else if (adjacentCoord.y >= grid.getNumRows())
            {
                grid.addRowToBottom();
            }
            else
            {
                // TODO: add to empty slot
            }
        }

        struct DirectionalOperation final {
            static DirectionalOperation None() { return {GridOperation::None, NoneDirectionalOp}; }
            static DirectionalOperation Move() { return {GridOperation::Move, MoveDirectionalOp}; }
            static DirectionalOperation Swap() { return {GridOperation::Swap, SwapDirectionalOp}; }
            static DirectionalOperation Add() { return {GridOperation::Add, AddDirectionalOp}; }

        private:
            DirectionalOperation(
                GridOperation operationType_,
                void (*execute_)(GridLayout&, glm::ivec2, GridDirection)) :

                operationType{operationType_},
                execute{execute_}
            {
            }
        public:
            GridOperation operationType = GridOperation::None;
            void (*execute)(GridLayout&, glm::ivec2, GridDirection) = NoneDirectionalOp;
        };

        DirectionalOperation lookupDirectionalOperation(glm::ivec2 gridCoord, GridDirection direction) const
        {
            glm::ivec2 const adjacentCoord = gridCoord + ToVec2(direction);

            if (isWithinGridBounds(adjacentCoord))
            {
                // ... if the cell in `direction` is within the grid...
                bool const isOccupied =
                    getCellID(adjacentCoord) != osc::UID::empty();

                if (isOccupied)
                {
                    // ... and the cell in that direction is occupied...
                    if (IsDiagonal(direction))
                    {
                        // ... and the direction is diagonal, then the
                        //     operation will move it under that cell
                        return DirectionalOperation::Move();
                    }
                    else
                    {
                        // ... and the non-diagonal operation swaps it
                        return DirectionalOperation::Swap();
                    }
                }
                else
                {
                    // ... and the cell is empty, then you can move there ...
                    return DirectionalOperation::Move();
                }
            }
            else if (!IsDiagonal(direction) && (adjacentCoord.x >= 0 && adjacentCoord.y >= 0))
            {
                // ... if cell in non-diagonal `direction` lies outside the grid
                //     and is in the bottom- or right-direction...
                return DirectionalOperation::Add();
            }
            else
            {
                // ... the cell in `direction` lies outside the grid and isn't in
                //     a suitable direction to support `Add`ing
                return DirectionalOperation::None();
            }
            return DirectionalOperation::None();  // (defensive: this shouldn't be reached)
        }

        size_t toCellIndex(glm::ivec2 coord) const
        {
            return coord.y*m_NumColumns + coord.x;
        }

        size_t toCellIndex(glm::ivec2 coord, size_t numCols) const
        {
            return coord.y*numCols + coord.x;
        }

        std::optional<size_t> tryGetIndexByID(osc::UID id) const
        {
            auto const it = std::find(m_Cells.begin(), m_Cells.end(), id);
            if (it != m_Cells.end())
            {
                return std::distance(m_Cells.begin(), it);
            }
            else
            {
                return std::nullopt;
            }
        }

        std::optional<glm::ivec2> tryGetCoordinateByID(osc::UID id) const
        {
            std::optional<size_t> const maybeIndex = tryGetIndexByID(id);

            if (maybeIndex)
            {
                return glm::ivec2
                {
                    *maybeIndex % m_NumColumns,
                    *maybeIndex / m_NumColumns,
                };
            }
            else
            {
                return std::nullopt;
            }
        }

        bool isWithinGridBounds(glm::ivec2 coord) const
        {
            return
                0 <= coord.x && coord.x < getNumColumns() &&
                0 <= coord.y && coord.y < getNumRows();
        }

        void addRowToBottom()
        {
            m_Cells.resize(m_Cells.size() + m_NumColumns, osc::UID::empty());
        }

        void addColumnToRight()
        {
            size_t const numRows = getNumRows();
            size_t const oldNumCols = getNumColumns();
            size_t const newNumCols = oldNumCols + 1;

            std::vector<osc::UID> newCells(numRows*newNumCols, osc::UID::empty());
            for (size_t row = 0; row < numRows; ++row)
            {
                for (size_t col = 0; col < oldNumCols; ++col)
                {
                    glm::ivec2 const coord =
                    {
                        static_cast<glm::ivec2::value_type>(col),
                        static_cast<glm::ivec2::value_type>(row),
                    };
                    size_t const oldIndex = toCellIndex(coord, oldNumCols);
                    size_t const newIndex = toCellIndex(coord, newNumCols);
                    newCells.at(newIndex) = m_Cells.at(oldIndex);
                }
            }

            m_NumColumns = newNumCols;
            m_Cells = std::move(newCells);
        }

        size_t m_NumColumns = 1;
        std::vector<osc::UID> m_Cells = {osc::UID::empty()};
    };

    class ExcitationDocument final {
    public:
        ExcitationPattern const* tryGetExcitationPatternByID(osc::UID id) const
        {
            auto const it = m_ExcitationPatternsByID.find(id);
            return it != m_ExcitationPatternsByID.end() ? &it->second : nullptr;
        }

        ExcitationPattern* tryUpdExcitationPatternByID(osc::UID id)
        {
            auto const it = m_ExcitationPatternsByID.find(id);
            return it != m_ExcitationPatternsByID.end() ? &it->second : nullptr;
        }

        GridLayout const& getGridLayout() const
        {
            return m_GridLayout;
        }

        GridLayout& updGridLayout()
        {
            return m_GridLayout;
        }
    private:
        std::unordered_map<osc::UID, ExcitationPattern> m_ExcitationPatternsByID;
        GridLayout m_GridLayout;
    };
}  // end of document state

// editor state
namespace
{
    class ExcitationEditorSharedState final {
    public:
        explicit ExcitationEditorSharedState(
            std::shared_ptr<osc::UndoableModelStatePair const> model_) :

            m_Model{std::move(model_)}
        {
        }

        GridLayout& updGridLayout()
        {
            return m_UndoableDocument->updScratch().updGridLayout();
        }

        GridLayout const& getGridLayout() const
        {
            return m_UndoableDocument->getScratch().getGridLayout();
        }

    private:
        std::shared_ptr<osc::UndoableModelStatePair const> m_Model;
        std::shared_ptr<osc::UndoRedoT<ExcitationDocument>> m_UndoableDocument =
            std::make_shared<osc::UndoRedoT<ExcitationDocument>>();
    };

    class ExcitationPlotsPanel final : public osc::StandardPanel {
    public:
        explicit ExcitationPlotsPanel(
            std::string_view panelName_,
            std::shared_ptr<ExcitationEditorSharedState> shared_) :

            StandardPanel{panelName_},
            m_Shared{std::move(shared_)}
        {
        }
    private:
        void implBeforeImGuiBegin() final
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
        }

        void implAfterImGuiBegin() final
        {
            ImGui::PopStyleVar();
        }

        void implDrawContent() final
        {
            osc::Rect const contentScreenRect = osc::ContentRegionAvailScreenRect();
            glm::vec2 const contentDims = osc::Dimensions(contentScreenRect);
            glm::ivec2 const gridDims = m_Shared->getGridLayout().getDimensions();
            glm::vec2 const cellDims = contentDims / glm::vec2{gridDims};

            int imguiID = 0;
            for (int row = 0; row < gridDims.y; ++row)
            {
                for (int col = 0; col < gridDims.x; ++col)
                {
                    // compute screen rect for each cell then draw it in that rect
                    glm::ivec2 const gridCoord = {col, row};
                    glm::vec2 const cellScreenTopLeft = contentScreenRect.p1 + glm::vec2{gridCoord}*cellDims;
                    glm::vec2 const cellScreenBottomRight = cellScreenTopLeft + cellDims;
                    osc::Rect const cellScreenRect = {cellScreenTopLeft, cellScreenBottomRight};

                    ImGui::PushID(imguiID++);
                    drawCell(gridCoord, cellScreenRect);
                    ImGui::PopID();
                }
            }
        }

        void drawCell(glm::ivec2 gridCoord, osc::Rect const& screenSpaceRect)
        {
            drawCellContent(gridCoord, screenSpaceRect);
            drawCellBorder(screenSpaceRect);
            if (osc::IsPointInRect(screenSpaceRect, ImGui::GetIO().MousePos))
            {
                drawCellOverlays(gridCoord, screenSpaceRect);
            }
        }

        void drawCellContent(glm::ivec2, osc::Rect const& screenSpaceRect)
        {
            osc::Rect const actualRect =
            {
                screenSpaceRect.p1 + 25.0f,
                screenSpaceRect.p2 - 25.0f,
            };

            size_t constexpr nFakeDataPoints = 100;
            std::vector<glm::vec2> fakeData;
            fakeData.reserve(nFakeDataPoints);
            for (size_t i = 0; i < nFakeDataPoints; ++i)
            {
                float const x = static_cast<float>(i)/static_cast<float>(nFakeDataPoints-1);
                float const y = 0.5f*(std::sin(x*30.0f) + 1.0f);
                fakeData.push_back({x, y});
            }

            ImGui::SetCursorScreenPos(actualRect.p1);
            ImPlotFlags const flags = ImPlotFlags_CanvasOnly | ImPlotFlags_NoInputs | ImPlotFlags_NoFrame;
            ImPlot::PushStyleColor(ImPlotCol_AxisBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
            ImPlot::PushStyleColor(ImPlotCol_FrameBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
            ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
            if (ImPlot::BeginPlot("##demoplot", osc::Dimensions(actualRect), flags))
            {
                ImPlot::SetupAxes(
                    "x",
                    "y",
                    ImPlotAxisFlags_Lock,
                    ImPlotAxisFlags_Lock
                );
                ImPlot::SetupAxisLimits(
                    ImAxis_X1,
                    -0.02,
                    1.02
                );
                ImPlot::SetupAxisLimits(
                    ImAxis_Y1,
                    -0.02,
                    1.02
                );
                ImPlot::SetupAxisTicks(
                    ImAxis_X1,
                    0.0,
                    1.0,
                    2
                );
                ImPlot::SetupAxisTicks(
                    ImAxis_Y1,
                    0.0,
                    1.0,
                    2
                );
                ImPlot::SetupFinish();

                ImPlot::PlotLine(
                    "series",
                    &fakeData.front().x,
                    &fakeData.front().y,
                    static_cast<int>(fakeData.size()),
                    0,
                    0,
                    sizeof(decltype(fakeData)::value_type)
                );

                ImPlot::EndPlot();
            }
            ImPlot::PopStyleColor();
            ImPlot::PopStyleColor();
            ImPlot::PopStyleColor();
        }

        void drawCellBorder(osc::Rect const& screenSpaceRect)
        {
            glm::vec4 bgColor = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
            bgColor *= 0.2f;

            ImGui::GetWindowDrawList()->AddRect(
                screenSpaceRect.p1,
                screenSpaceRect.p2,
                ImGui::ColorConvertFloat4ToU32(bgColor)
            );
        }

        void drawCellOverlays(glm::ivec2 gridCoord, osc::Rect const& screenSpaceRect)
        {
            int imguiID = 0;
            for (int rowDirection = -1; rowDirection <= 1; ++rowDirection)
            {
                for (int colDirection = -1; colDirection <= 1; ++colDirection)
                {
                   std::optional<GridDirection> const maybeDir = FromVec2({colDirection, rowDirection});
                   if (maybeDir)
                   {
                       ImGui::PushID(imguiID++);
                       drawCellOverlay(gridCoord, screenSpaceRect, *maybeDir);
                       ImGui::PopID();
                   }
                }
            }
        }

        osc::CStringView calcOverlayIconText(GridDirection, GridOperation operation) const
        {
            switch (operation)
            {
            case GridOperation::Add: return "+";
            case GridOperation::Move: return "M";
            case GridOperation::Swap: return "S";
            default: return "?";
            }
        }

        void drawCellOverlay(glm::ivec2 gridCoord, osc::Rect screenSpaceRect, GridDirection direction)
        {
            GridOperation const operation = m_Shared->getGridLayout().calcAvaliableDirectionalOperation(gridCoord, direction);
            if (operation == GridOperation::None)
            {
                return;
            }

            osc::CStringView const iconText = calcOverlayIconText(direction, operation);

            glm::vec2 const padding = ImGui::GetStyle().FramePadding;
            glm::vec2 const cellHalfDims = 0.5f * osc::Dimensions(screenSpaceRect);
            glm::vec2 const cellSpaceMidpoint = osc::Midpoint(screenSpaceRect) - screenSpaceRect.p1;
            glm::vec2 const cellSpaceLabelDirection = ToVec2(direction);
            glm::vec2 const cellSpaceOutwardPoint = cellSpaceMidpoint + cellSpaceLabelDirection*(cellHalfDims - padding);
            glm::vec2 const buttonDims = osc::CalcButtonSize(iconText);
            glm::vec2 const cellSpaceDirectionCorrection = -(cellSpaceLabelDirection+1.0f)/2.0f;
            glm::vec2 const cellSpaceLabelTopRight = cellSpaceOutwardPoint + buttonDims*cellSpaceDirectionCorrection;
            glm::vec2 const screenSpaceLabelTopRight = screenSpaceRect.p1 + cellSpaceLabelTopRight;

            glm::vec4 buttonColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);
            buttonColor.a *= 0.25f;
            glm::vec4 textColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
            textColor.a *= 0.25f;

            ImGui::SetCursorScreenPos(screenSpaceLabelTopRight);
            ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
            ImGui::PushStyleColor(ImGuiCol_Text, textColor);
            if (ImGui::Button(iconText.c_str()))
            {
                m_Shared->updGridLayout().doDirectionalOperation(gridCoord, direction);
            }
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
        }

        std::shared_ptr<ExcitationEditorSharedState> m_Shared;
    };
}

class osc::ExcitationEditorTab::Impl final {
public:

    Impl(
        std::weak_ptr<TabHost> parent_,
        std::shared_ptr<UndoableModelStatePair const> model_) :

        m_Parent{std::move(parent_)},
        m_Model{std::move(model_)}
    {
        m_PanelManager->registerToggleablePanel(
            "Excitation Plots",
            [this](std::string_view panelName)
            {
                return std::make_shared<ExcitationPlotsPanel>(
                    panelName,
                    m_Shared
                );
            }
        );
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return c_TabStringID;
    }

    void onMount()
    {
        m_PanelManager->onMount();
    }

    void onUnmount()
    {
        m_PanelManager->onUnmount();
    }

    bool onEvent(SDL_Event const&)
    {
        return false;
    }

    void onTick()
    {
        m_PanelManager->onTick();
    }

    void onDrawMainMenu()
    {
        // TODO: redundantly show new/save/open etc.
    }

    void onDraw()
    {
        ImGui::DockSpaceOverViewport(
            ImGui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode
        );
        m_PanelManager->onDraw();
        // TODO:
        //
        // - render a toolbar with new/save/open/undo/redo etc.
        //
        // - render a table using data from the `GridLayout`
        //   - occupied cells should contain an excitation plot
        //   - and occipied cells should show an overlay of
        //     appropriate directional operations around the edges
        //     (e.g. move things around, etc.)
        //   - empty cells should show a centered "Add Excitation Curve"
        //     button
    }

private:
    UID m_TabID;
    std::weak_ptr<TabHost> m_Parent;
    std::shared_ptr<UndoableModelStatePair const> m_Model;

    std::shared_ptr<ExcitationEditorSharedState> m_Shared =
        std::make_shared<ExcitationEditorSharedState>(m_Model);
    std::shared_ptr<PanelManager> m_PanelManager =
        std::make_shared<PanelManager>();
};


// public API (PIMPL)

osc::CStringView osc::ExcitationEditorTab::id() noexcept
{
    return c_TabStringID;
}

osc::ExcitationEditorTab::ExcitationEditorTab(
    std::weak_ptr<TabHost> parent_,
    std::shared_ptr<UndoableModelStatePair const> model_) :

    m_Impl{std::make_unique<Impl>(std::move(parent_), std::move(model_))}
{
}

osc::ExcitationEditorTab::ExcitationEditorTab(ExcitationEditorTab&&) noexcept = default;
osc::ExcitationEditorTab& osc::ExcitationEditorTab::operator=(ExcitationEditorTab&&) noexcept = default;
osc::ExcitationEditorTab::~ExcitationEditorTab() noexcept = default;

osc::UID osc::ExcitationEditorTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::ExcitationEditorTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::ExcitationEditorTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::ExcitationEditorTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::ExcitationEditorTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::ExcitationEditorTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::ExcitationEditorTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::ExcitationEditorTab::implOnDraw()
{
    m_Impl->onDraw();
}
