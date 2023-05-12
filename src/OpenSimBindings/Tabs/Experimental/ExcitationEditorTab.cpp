#include "ExcitationEditorTab.hpp"

#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Panels/PanelManager.hpp"
#include "src/Panels/StandardPanel.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"
#include "src/Utils/UndoRedo.hpp"

#include <glm/vec2.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>
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
    class GridDirection final {
    public:
        constexpr static GridDirection North() noexcept { return {0, -1}; }
        constexpr static GridDirection NorthEast() noexcept { return {1, -1}; }
        constexpr static GridDirection East() noexcept { return {1, 0}; }
        constexpr static GridDirection SouthEast() noexcept { return {1, 1}; }
        constexpr static GridDirection South() noexcept { return {0, 1}; }
        constexpr static GridDirection SouthWest() noexcept { return {-1, 1}; }
        constexpr static GridDirection West() noexcept { return {-1, 0}; }
        constexpr static GridDirection NorthWest() noexcept { return {-1, 1}; }

    private:
        friend constexpr glm::ivec2 ToVec2(GridDirection const&) noexcept;

        constexpr GridDirection(int x, int y) : m_Offset{x, y} {}       

        glm::ivec2 m_Offset;
    };

    constexpr glm::ivec2 ToVec2(GridDirection const& d) noexcept
    {
        return d.m_Offset;
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
        return v.x*v.y == 0;
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
            return m_Cells.size() / m_NumColumns;
        }

        size_t getNumColumns() const
        {
            return m_NumColumns;
        }

        osc::UID getCellID(glm::ivec2 coord) const
        {
            return m_Cells.at(toCellIndex(coord));
        }

        void setCellID(glm::ivec2 coord, osc::UID newID)
        {
            m_Cells.at(toCellIndex(coord)) = newID;
        }

        GridOperation calcAvaliableDirectionalOperation(osc::UID id, GridDirection direction) const
        {
            return lookupDirectionalOperation(id, direction).operationType;
        }

        void doDirectionalOperation(osc::UID id, GridDirection direction)
        {
            lookupDirectionalOperation(id, direction).execute(*this, id, direction);
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
        static void NoneDirectionalOp(GridLayout& grid, osc::UID id, GridDirection direction)
        {
        }

        static void MoveDirectionalOp(GridLayout& grid, osc::UID id, GridDirection direction)
        {
        }

        static void SwapDirectionalOp(GridLayout&, osc::UID, GridDirection)
        {
        }

        static void AddDirectionalOp(GridLayout&, osc::UID, GridDirection)
        {
        }

        struct DirectionalOperation final {
            static DirectionalOperation None() { return {GridOperation::None, NoneDirectionalOp}; }
            static DirectionalOperation Move() { return {GridOperation::Move, MoveDirectionalOp}; }
            static DirectionalOperation Swap() { return {GridOperation::Swap, SwapDirectionalOp}; }
            static DirectionalOperation Add() { return {GridOperation::Add, AddDirectionalOp}; }

        private:
            DirectionalOperation(GridOperation operationType_, void (*execute_)(GridLayout&, osc::UID, GridDirection)) :
                operationType{operationType_}, execute{execute_}
            {
            }
        public:
            GridOperation operationType = GridOperation::None;
            void (*execute)(GridLayout&, osc::UID, GridDirection) = NoneDirectionalOp;
        };

        DirectionalOperation lookupDirectionalOperation(osc::UID id, GridDirection direction) const
        {
            std::optional<glm::ivec2> const maybeCoord = tryGetCoordinateByID(id);
            if (!maybeCoord)
            {
                return DirectionalOperation::None();  // element with given `id` not found within the grid
            }

            glm::ivec2 const adjacentCoord = *maybeCoord + ToVec2(direction);

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
                    *maybeIndex / m_NumColumns,
                    *maybeIndex % m_NumColumns,
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
    private:
        std::shared_ptr<osc::UndoableModelStatePair const> m_Model;
        std::shared_ptr<osc::UndoRedoT<ExcitationDocument>> m_UndoableDocument;
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
        void implDrawContent() final
        {
            ImGui::TextWrapped("Work in progress: this tab is just stubbed here while I develop the underlying code on-branch");
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
        m_PanelManager->activateAllDefaultOpenPanels();
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return c_TabStringID;
    }

    void onMount() {}
    void onUnmount() {}

    bool onEvent(SDL_Event const&)
    {
        return false;
    }

    void onTick()
    {
        m_PanelManager->garbageCollectDeactivatedPanels();
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
        m_PanelManager->drawAllActivatedPanels();
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
