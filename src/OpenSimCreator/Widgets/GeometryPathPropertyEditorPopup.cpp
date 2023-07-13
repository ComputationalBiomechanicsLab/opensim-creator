#include "GeometryPathPropertyEditorPopup.hpp"

#include "OpenSimCreator/Model/UndoableModelStatePair.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Widgets/StandardPopup.hpp>

#include <IconsFontAwesome5.h>
#include <OpenSim/Simulation/Model/AbstractPathPoint.h>
#include <OpenSim/Simulation/Model/GeometryPath.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PathPoint.h>

#include <array>
#include <functional>
#include <memory>
#include <string_view>
#include <utility>

namespace
{
    constexpr auto c_LocationInputIDs = osc::to_array<osc::CStringView>({ "##xinput", "##yinput", "##zinput" });
    static_assert(c_LocationInputIDs.size() == 3);

    OpenSim::GeometryPath InitGeometryPathFromPropOrDefault(std::function<OpenSim::ObjectProperty<OpenSim::GeometryPath> const* ()> const& accessor)
    {
        OpenSim::ObjectProperty<OpenSim::GeometryPath> const* maybeGeomPath = accessor();
        if (!maybeGeomPath)
        {
            return OpenSim::GeometryPath{};  // default it
        }
        else if (maybeGeomPath->size() >= 1)
        {
            return maybeGeomPath->getValue(0);
        }
        else
        {
            return OpenSim::GeometryPath{};  // ignore lists of geometry paths (too complicated)
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
            int pathPointIndex_) :

            type{type_},
            pathPointIndex{pathPointIndex_}
        {
        }

        void reset()
        {
            *this = {};
        }

        Type type = Type::None;
        int pathPointIndex = -1;
    };

    void ActionMovePathPointUp(OpenSim::PathPointSet& pps, int i)
    {
        if (0 < i && i < pps.getSize())
        {
            std::unique_ptr<OpenSim::AbstractPathPoint> tmp{pps.get(i).clone()};
            pps.set(i, pps.get(i-1));
            pps.set(i-1, tmp.release());
        }
    }

    void ActionMovePathPointDown(OpenSim::PathPointSet& pps, int i)
    {
        if (0 <= i && i+1 < pps.getSize())
        {
            std::unique_ptr<OpenSim::AbstractPathPoint> tmp{pps.get(i).clone()};
            pps.set(i, pps.get(i+1));
            pps.set(i+1, tmp.release());
        }
    }

    void ActionDeletePathPoint(OpenSim::PathPointSet& pps, int i)
    {
        if (0 <= i && i < pps.getSize())
        {
            pps.remove(i);
        }
    }

    void ActionSetPathPointFramePath(OpenSim::PathPointSet& pps, int i, std::string const& frameAbsPath)
    {
        pps.get(i).updSocket("parent_frame").setConnecteePath(frameAbsPath);
    }

    void ActionAddNewPathPoint(OpenSim::PathPointSet& pps)
    {
        std::string const frame = pps.getSize() == 0 ?
            "/ground" :
            pps.get(pps.getSize() - 1).getSocket("parent_frame").getConnecteePath();

        auto pp = std::make_unique<OpenSim::PathPoint>();
        pp->updSocket("parent_frame").setConnecteePath(frame);
        pps.adoptAndAppend(pp.release());
    }

    std::function<void(OpenSim::AbstractProperty&)> MakeGeometryPathPropertyOverwriter(
        OpenSim::GeometryPath const& editedPath)
    {
        return [editedPath](OpenSim::AbstractProperty& prop)
        {
            if (auto* gpProp = dynamic_cast<OpenSim::ObjectProperty<OpenSim::GeometryPath>*>(&prop))
            {
                if (gpProp->size() >= 1)
                {
                    gpProp->updValue() = editedPath;  // just overwrite it
                }
            }
        };
    }

    osc::ObjectPropertyEdit MakeObjectPropertyEdit(
        OpenSim::ObjectProperty<OpenSim::GeometryPath> const& prop,
        OpenSim::GeometryPath const& editedPath)
    {
        return {prop, MakeGeometryPathPropertyOverwriter(editedPath)};
    }
}

class osc::GeometryPathPropertyEditorPopup::Impl final : public osc::StandardPopup {
public:
    Impl(
        std::string_view popupName_,
        std::shared_ptr<UndoableModelStatePair const> targetModel_,
        std::function<OpenSim::ObjectProperty<OpenSim::GeometryPath> const*()> accessor_,
        std::function<void(ObjectPropertyEdit)> onEditCallback_) :

        StandardPopup{popupName_, {768.0f, 0.0f}, ImGuiWindowFlags_AlwaysAutoResize},
        m_TargetModel{std::move(targetModel_)},
        m_Accessor{std::move(accessor_)},
        m_OnEditCallback{std::move(onEditCallback_)},
        m_EditedGeometryPath{InitGeometryPathFromPropOrDefault(m_Accessor)}
    {
    }
private:
    void implDrawContent() final
    {
        if (m_Accessor() == nullptr)
        {
            // edge-case: the geometry path that this popup is editing no longer
            // exists (e.g. because a muscle was deleted or similar), so it should
            // announce the problem and close itself
            ImGui::Text("The GeometryPath no longer exists - closing this popup");
            requestClose();
            return;
        }
        // else: the geometry path exists, but this UI should edit the cached
        // `m_EditedGeometryPath`, which is independent of the original data
        // and the target model (so that edits can be applied transactionally)

        ImGui::Text("Path Points:");
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

        ImGuiTableFlags const tableFlags =
            ImGuiTableFlags_NoSavedSettings |
            ImGuiTableFlags_BordersInner |
            ImGuiTableFlags_SizingStretchSame;

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

            int imguiID = 0;
            for (int i = 0; i < pps.getSize(); ++i)
            {
                ImGui::PushID(imguiID++);
                drawIthPathPointTableRow(pps, i);
                ImGui::PopID();
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
        if (ImGui::Button(ICON_FA_PLUS_CIRCLE " Add Point"))
        {
            ActionAddNewPathPoint(m_EditedGeometryPath.updPathPointSet());
        }
    }

    void drawIthPathPointTableRow(OpenSim::PathPointSet& pps, int i)
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

    void drawIthPathPointActionsCell(OpenSim::PathPointSet& pps, int i)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {2.0f, 0.0f});

        if (i <= 0)
        {
            ImGui::BeginDisabled();
        }
        if (ImGui::SmallButton(ICON_FA_ARROW_UP))
        {
            m_RequestedAction = RequestedAction{RequestedAction::Type::MoveUp, i};
        }
        if (i <= 0)
        {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        if (i+1 >= pps.getSize())
        {
            ImGui::BeginDisabled();
        }
        if (ImGui::SmallButton(ICON_FA_ARROW_DOWN))
        {
            m_RequestedAction = RequestedAction{RequestedAction::Type::MoveDown, i};
        }
        if (i+1 >= pps.getSize())
        {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Text, {0.7f, 0.0f, 0.0f, 1.0f});
        if (ImGui::SmallButton(ICON_FA_TIMES))
        {
            m_RequestedAction = RequestedAction{RequestedAction::Type::Delete, i};
        }
        ImGui::PopStyleColor();

        ImGui::PopStyleVar();
    }

    void drawIthPathPointTypeCell(OpenSim::PathPointSet const& pps, int i)
    {
        ImGui::TextDisabled("%s", pps.get(i).getConcreteClassName().c_str());
    }

    // try, because the path point type might not actually have a set location
    //
    // (e.g. `MovingPathPoint`s)
    void tryDrawIthPathPointLocationEditorCells(OpenSim::PathPointSet& pps, int i, int& column)
    {
        OpenSim::AbstractPathPoint& app = pps.get(i);

        if (auto* const pp = dynamic_cast<OpenSim::PathPoint*>(&app))
        {
            float const inputWidth = ImGui::CalcTextSize("0.00000").x;

            SimTK::Vec3& location = pp->upd_location();

            static_assert(c_LocationInputIDs.size() == 3);
            for (int dim = 0; dim < c_LocationInputIDs.size(); ++dim)
            {
                auto v = static_cast<float>(location[dim]);

                ImGui::TableSetColumnIndex(column++);
                ImGui::SetNextItemWidth(inputWidth);
                if (ImGui::InputFloat(c_LocationInputIDs[dim].c_str(), &v))
                {
                    location[dim] = static_cast<double>(v);
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

    void drawIthPathPointFrameCell(OpenSim::PathPointSet& pps, int i)
    {
        float const width = ImGui::CalcTextSize("/bodyset/a_typical_body_name").x;

        std::string const& label = pps.get(i).getSocket("parent_frame").getConnecteePath();

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
        if (ImGui::Button("cancel"))
        {
            requestClose();
        }

        ImGui::SameLine();

        if (ImGui::Button("save"))
        {
            OpenSim::ObjectProperty<OpenSim::GeometryPath> const* maybeProp = m_Accessor();
            if (maybeProp)
            {
                m_OnEditCallback(MakeObjectPropertyEdit(*maybeProp, m_EditedGeometryPath));
            }
            else
            {
                log::error("cannot update geometry path: it no longer exists");
            }
            requestClose();
        }
    }

    void tryExecuteRequestedAction(OpenSim::PathPointSet& pps)
    {
        if (!(0 <= m_RequestedAction.pathPointIndex && m_RequestedAction.pathPointIndex < pps.getSize()))
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

        m_RequestedAction.reset();  // action handled: reset
    }

    std::shared_ptr<UndoableModelStatePair const> m_TargetModel;
    std::function<OpenSim::ObjectProperty<OpenSim::GeometryPath> const*()> m_Accessor;
    std::function<void(ObjectPropertyEdit)> m_OnEditCallback;

    OpenSim::GeometryPath m_EditedGeometryPath;
    RequestedAction m_RequestedAction;
};


// public API (PIMPL)

osc::GeometryPathPropertyEditorPopup::GeometryPathPropertyEditorPopup(
    std::string_view popupName_,
    std::shared_ptr<UndoableModelStatePair const> targetModel_,
    std::function<OpenSim::ObjectProperty<OpenSim::GeometryPath> const*()> accessor_,
    std::function<void(ObjectPropertyEdit)> onEditCallback_) :

    m_Impl{std::make_unique<Impl>(std::move(popupName_), std::move(targetModel_), std::move(accessor_), std::move(onEditCallback_))}
{
}
osc::GeometryPathPropertyEditorPopup::GeometryPathPropertyEditorPopup(GeometryPathPropertyEditorPopup&&) noexcept = default;
osc::GeometryPathPropertyEditorPopup& osc::GeometryPathPropertyEditorPopup::operator=(GeometryPathPropertyEditorPopup&&) noexcept = default;
osc::GeometryPathPropertyEditorPopup::~GeometryPathPropertyEditorPopup() noexcept = default;

bool osc::GeometryPathPropertyEditorPopup::implIsOpen() const
{
    return m_Impl->isOpen();
}
void osc::GeometryPathPropertyEditorPopup::implOpen()
{
    m_Impl->open();
}
void osc::GeometryPathPropertyEditorPopup::implClose()
{
    m_Impl->close();
}
bool osc::GeometryPathPropertyEditorPopup::implBeginPopup()
{
    return m_Impl->beginPopup();
}
void osc::GeometryPathPropertyEditorPopup::implDrawPopupContent()
{
    m_Impl->drawPopupContent();
}
void osc::GeometryPathPropertyEditorPopup::implEndPopup()
{
    m_Impl->endPopup();
}