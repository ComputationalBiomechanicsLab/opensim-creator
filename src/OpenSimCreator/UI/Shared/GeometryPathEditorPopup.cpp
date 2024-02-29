#include "GeometryPathEditorPopup.h"

#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <IconsFontAwesome5.h>
#include <OpenSim/Simulation/Model/AbstractPathPoint.h>
#include <OpenSim/Simulation/Model/GeometryPath.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PathPoint.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Widgets/StandardPopup.h>
#include <oscar/Utils/CStringView.h>

#include <array>
#include <functional>
#include <memory>
#include <string_view>
#include <utility>

using namespace osc;

namespace
{
    constexpr auto c_LocationInputIDs = std::to_array<CStringView>({ "##xinput", "##yinput", "##zinput" });
    static_assert(c_LocationInputIDs.size() == 3);

    OpenSim::GeometryPath CopyOrDefaultGeometryPath(std::function<OpenSim::GeometryPath const*()> const& accessor)
    {
        OpenSim::GeometryPath const* maybeGeomPath = accessor();
        if (maybeGeomPath)
        {
            return *maybeGeomPath;
        }
        else
        {
            return OpenSim::GeometryPath{};
        }
    }

    struct RequestedAction final {
        enum class Type {
            MoveUp,
            MoveDown,
            Delete,
            None,
        };

        RequestedAction() = default;

        RequestedAction(
            Type type_,
            ptrdiff_t pathPointIndex_) :

            type{type_},
            pathPointIndex{pathPointIndex_}
        {
        }

        void reset()
        {
            *this = {};
        }

        Type type = Type::None;
        ptrdiff_t pathPointIndex = -1;
    };

    void ActionMovePathPointUp(OpenSim::PathPointSet& pps, ptrdiff_t i)
    {
        if (1 <= i && i < ssize(pps))
        {
            auto tmp = Clone(At(pps, i));
            Assign(pps, i, At(pps, i-1));
            Assign(pps, i-1, std::move(tmp));
        }
    }

    void ActionMovePathPointDown(OpenSim::PathPointSet& pps, ptrdiff_t i)
    {
        if (0 <= i && i < ssize(pps)-1)
        {
            auto tmp = Clone(At(pps, i));
            Assign(pps, i, At(pps, i+1));
            Assign(pps, i+1, std::move(tmp));
        }
    }

    void ActionDeletePathPoint(OpenSim::PathPointSet& pps, ptrdiff_t i)
    {
        if (0 <= i && i < ssize(pps))
        {
            EraseAt(pps, i);
        }
    }

    void ActionSetPathPointFramePath(
        OpenSim::PathPointSet& pps,
        ptrdiff_t i,
        std::string const& frameAbsPath)
    {
        At(pps, i).updSocket("parent_frame").setConnecteePath(frameAbsPath);
    }

    void ActionAddNewPathPoint(OpenSim::PathPointSet& pps)
    {
        std::string const frame = empty(pps) ?
            "/ground" :
            At(pps, size(pps)-1).getSocket("parent_frame").getConnecteePath();

        auto pp = std::make_unique<OpenSim::PathPoint>();
        pp->updSocket("parent_frame").setConnecteePath(frame);

        Append(pps, std::move(pp));
    }
}

class osc::GeometryPathEditorPopup::Impl final : public StandardPopup {
public:
    Impl(
        std::string_view popupName_,
        std::shared_ptr<UndoableModelStatePair const> targetModel_,
        std::function<OpenSim::GeometryPath const*()> geometryPathGetter_,
        std::function<void(OpenSim::GeometryPath const&)> onLocalCopyEdited_) :

        StandardPopup{popupName_, {768.0f, 0.0f}, ImGuiWindowFlags_AlwaysAutoResize},
        m_TargetModel{std::move(targetModel_)},
        m_GeometryPathGetter{std::move(geometryPathGetter_)},
        m_OnLocalCopyEdited{std::move(onLocalCopyEdited_)},
        m_EditedGeometryPath{CopyOrDefaultGeometryPath(m_GeometryPathGetter)}
    {
    }
private:
    void implDrawContent() final
    {
        if (m_GeometryPathGetter() == nullptr)
        {
            // edge-case: the geometry path that this popup is editing no longer
            // exists (e.g. because a muscle was deleted or similar), so it should
            // announce the problem and close itself
            ui::Text("The GeometryPath no longer exists - closing this popup");
            requestClose();
            return;
        }
        // else: the geometry path exists, but this UI should edit the cached
        // `m_EditedGeometryPath`, which is independent of the original data
        // and the target model (so that edits can be applied transactionally)

        ui::Text("Path Points:");
        ImGui::Separator();
        drawPathPointEditorTable();
        ImGui::Separator();
        drawAddPathPointButton();
        ImGui::NewLine();
        drawBottomButtons();
    }

    void drawPathPointEditorTable()
    {
        OpenSim::PathPointSet& pps = m_EditedGeometryPath.updPathPointSet();

        if (ImGui::BeginTable("##GeometryPathEditorTable", 6))
        {
            ImGui::TableSetupColumn("Actions");
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupColumn("X");
            ImGui::TableSetupColumn("Y");
            ImGui::TableSetupColumn("Z");
            ImGui::TableSetupColumn("Frame");
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (ptrdiff_t i = 0; i < ssize(pps); ++i)
            {
                PushID(i);
                drawIthPathPointTableRow(pps, i);
                PopID();
            }

            ImGui::EndTable();
        }

        // perform any actions after rendering the table: in case the action would
        // in some way screw with rendering (e.g. deleting a point midway
        // through rendering a row is probably a bad idea)
        tryExecuteRequestedAction(pps);
    }

    void drawAddPathPointButton()
    {
        if (ui::Button(ICON_FA_PLUS_CIRCLE " Add Point"))
        {
            ActionAddNewPathPoint(m_EditedGeometryPath.updPathPointSet());
        }
    }

    void drawIthPathPointTableRow(OpenSim::PathPointSet& pps, ptrdiff_t i)
    {
        int column = 0;

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(column++);
        drawIthPathPointActionsCell(pps, i);

        ImGui::TableSetColumnIndex(column++);
        drawIthPathPointTypeCell(pps, i);

        tryDrawIthPathPointLocationEditorCells(pps, i, column);

        ImGui::TableSetColumnIndex(column++);
        drawIthPathPointFrameCell(pps, i);
    }

    void drawIthPathPointActionsCell(OpenSim::PathPointSet& pps, ptrdiff_t i)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {2.0f, 0.0f});

        if (i <= 0)
        {
            ui::BeginDisabled();
        }
        if (ImGui::SmallButton(ICON_FA_ARROW_UP))
        {
            m_RequestedAction = RequestedAction{RequestedAction::Type::MoveUp, i};
        }
        if (i <= 0)
        {
            ui::EndDisabled();
        }

        ui::SameLine();

        if (i+1 >= ssize(pps))
        {
            ui::BeginDisabled();
        }
        if (ImGui::SmallButton(ICON_FA_ARROW_DOWN))
        {
            m_RequestedAction = RequestedAction{RequestedAction::Type::MoveDown, i};
        }
        if (i+1 >= ssize(pps))
        {
            ui::EndDisabled();
        }

        ui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Text, {0.7f, 0.0f, 0.0f, 1.0f});
        if (ImGui::SmallButton(ICON_FA_TIMES))
        {
            m_RequestedAction = RequestedAction{RequestedAction::Type::Delete, i};
        }
        ImGui::PopStyleColor();

        ImGui::PopStyleVar();
    }

    void drawIthPathPointTypeCell(OpenSim::PathPointSet const& pps, ptrdiff_t i)
    {
        ui::TextDisabled(At(pps, i).getConcreteClassName());
    }

    // try, because the path point type might not actually have a set location
    //
    // (e.g. `MovingPathPoint`s)
    void tryDrawIthPathPointLocationEditorCells(OpenSim::PathPointSet& pps, ptrdiff_t i, int& column)
    {
        OpenSim::AbstractPathPoint& app = At(pps, i);

        if (auto* const pp = dynamic_cast<OpenSim::PathPoint*>(&app))
        {
            float const inputWidth = ImGui::CalcTextSize("0.00000").x;

            SimTK::Vec3& location = pp->upd_location();

            static_assert(c_LocationInputIDs.size() == 3);
            for (size_t dim = 0; dim < c_LocationInputIDs.size(); ++dim)
            {
                auto v = static_cast<float>(location[static_cast<int>(dim)]);

                ImGui::TableSetColumnIndex(column++);
                ImGui::SetNextItemWidth(inputWidth);
                if (ImGui::InputFloat(c_LocationInputIDs[dim].c_str(), &v))
                {
                    location[static_cast<int>(dim)] = static_cast<double>(v);
                }
            }
        }
        else
        {
            // it's some other kind of path point, with no editable X, Y, or Z
            ImGui::TableSetColumnIndex(column++);
            ImGui::TableSetColumnIndex(column++);
            ImGui::TableSetColumnIndex(column++);
        }
    }

    void drawIthPathPointFrameCell(OpenSim::PathPointSet& pps, ptrdiff_t i)
    {
        float const width = ImGui::CalcTextSize("/bodyset/a_typical_body_name").x;

        std::string const& label = At(pps, i).getSocket("parent_frame").getConnecteePath();

        ImGui::SetNextItemWidth(width);
        if (ImGui::BeginCombo("##framesel", label.c_str()))
        {
            for (OpenSim::Frame const& frame : m_TargetModel->getModel().getComponentList<OpenSim::Frame>())
            {
                std::string const absPath = frame.getAbsolutePathString();
                if (ImGui::Selectable(absPath.c_str()))
                {
                    ActionSetPathPointFramePath(pps, i, absPath);
                }
            }
            ImGui::EndCombo();
        }
    }

    void drawBottomButtons()
    {
        if (ui::Button("cancel"))
        {
            requestClose();
        }

        ui::SameLine();

        if (ui::Button("save"))
        {
            m_OnLocalCopyEdited(m_EditedGeometryPath);
            requestClose();
        }
    }

    void tryExecuteRequestedAction(OpenSim::PathPointSet& pps)
    {
        if (!(0 <= m_RequestedAction.pathPointIndex && m_RequestedAction.pathPointIndex < ssize(pps)))
        {
            // edge-case: if the index is out of range, ignore the action
            m_RequestedAction.reset();
            return;
        }

        switch (m_RequestedAction.type)
        {
        case RequestedAction::Type::MoveUp:
            ActionMovePathPointUp(pps, m_RequestedAction.pathPointIndex);
            break;
        case RequestedAction::Type::MoveDown:
            ActionMovePathPointDown(pps, m_RequestedAction.pathPointIndex);
            break;
        case RequestedAction::Type::Delete:
            ActionDeletePathPoint(pps, m_RequestedAction.pathPointIndex);
            break;
        default:
            break;  // (unhandled/do nothing)
        }

        m_RequestedAction.reset();  // action handled: resets
    }

    std::shared_ptr<UndoableModelStatePair const> m_TargetModel;
    std::function<OpenSim::GeometryPath const*()> m_GeometryPathGetter;
    std::function<void(OpenSim::GeometryPath const&)> m_OnLocalCopyEdited;

    OpenSim::GeometryPath m_EditedGeometryPath;
    RequestedAction m_RequestedAction;
};


// public API (PIMPL)

osc::GeometryPathEditorPopup::GeometryPathEditorPopup(
    std::string_view popupName_,
    std::shared_ptr<UndoableModelStatePair const> targetModel_,
    std::function<OpenSim::GeometryPath const*()> geometryPathGetter_,
    std::function<void(OpenSim::GeometryPath const&)> onLocalCopyEdited_) :

    m_Impl{std::make_unique<Impl>(popupName_, std::move(targetModel_), std::move(geometryPathGetter_), std::move(onLocalCopyEdited_))}
{
}
osc::GeometryPathEditorPopup::GeometryPathEditorPopup(GeometryPathEditorPopup&&) noexcept = default;
osc::GeometryPathEditorPopup& osc::GeometryPathEditorPopup::operator=(GeometryPathEditorPopup&&) noexcept = default;
osc::GeometryPathEditorPopup::~GeometryPathEditorPopup() noexcept = default;

bool osc::GeometryPathEditorPopup::implIsOpen() const
{
    return m_Impl->isOpen();
}
void osc::GeometryPathEditorPopup::implOpen()
{
    m_Impl->open();
}
void osc::GeometryPathEditorPopup::implClose()
{
    m_Impl->close();
}
bool osc::GeometryPathEditorPopup::implBeginPopup()
{
    return m_Impl->beginPopup();
}
void osc::GeometryPathEditorPopup::implOnDraw()
{
    m_Impl->onDraw();
}
void osc::GeometryPathEditorPopup::implEndPopup()
{
    m_Impl->endPopup();
}
