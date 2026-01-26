#include "GeometryPathEditorPopup.h"

#include <libopensimcreator/Platform/msmicons.h>

#include <libopynsim/documents/model/component_accessor.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/popups/popup.h>
#include <liboscar/ui/popups/popup_private.h>
#include <liboscar/utils/c_string_view.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/AbstractPathPoint.h>
#include <OpenSim/Simulation/Model/GeometryPath.h>
#include <OpenSim/Simulation/Model/PathPoint.h>

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

    OpenSim::GeometryPath CopyOrDefaultGeometryPath(const std::function<const OpenSim::GeometryPath*()>& accessor)
    {
        const OpenSim::GeometryPath* maybeGeomPath = accessor();
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
            Type m_Type,
            ptrdiff_t pathPointIndex_) :

            type{m_Type},
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
        if (1 <= i && i < opyn::ssize(pps))
        {
            auto tmp = opyn::Clone(opyn::At(pps, i));
            opyn::Assign(pps, i, opyn::At(pps, i-1));
            opyn::Assign(pps, i-1, std::move(tmp));
        }
    }

    void ActionMovePathPointDown(OpenSim::PathPointSet& pps, ptrdiff_t i)
    {
        if (0 <= i && i < opyn::ssize(pps)-1)
        {
            auto tmp = opyn::Clone(opyn::At(pps, i));
            opyn::Assign(pps, i, opyn::At(pps, i+1));
            opyn::Assign(pps, i+1, std::move(tmp));
        }
    }

    void ActionDeletePathPoint(OpenSim::PathPointSet& pps, ptrdiff_t i)
    {
        if (0 <= i && i < opyn::ssize(pps))
        {
            opyn::EraseAt(pps, i);
        }
    }

    void ActionSetPathPointFramePath(
        OpenSim::PathPointSet& pps,
        ptrdiff_t i,
        const std::string& frameAbsPath)
    {
        opyn::At(pps, i).updSocket("parent_frame").setConnecteePath(frameAbsPath);
    }

    void ActionAddNewPathPoint(OpenSim::PathPointSet& pps)
    {
        const std::string frame = opyn::empty(pps) ?
            "/ground" :
            opyn::At(pps, opyn::size(pps)-1).getSocket("parent_frame").getConnecteePath();

        auto pp = std::make_unique<OpenSim::PathPoint>();
        pp->updSocket("parent_frame").setConnecteePath(frame);

        opyn::Append(pps, std::move(pp));
    }
}

class osc::GeometryPathEditorPopup::Impl final : public PopupPrivate {
public:
    explicit Impl(
        GeometryPathEditorPopup& owner,
        Widget* parent,
        std::string_view popupName_,
        std::shared_ptr<const ComponentAccessor> targetComponent_,
        std::function<const OpenSim::GeometryPath*()> geometryPathGetter_,
        std::function<void(const OpenSim::GeometryPath&)> onLocalCopyEdited_) :

        PopupPrivate{owner, parent, popupName_, {768.0f, 0.0f}, ui::PanelFlag::AlwaysAutoResize},
        m_TargetComponent{std::move(targetComponent_)},
        m_GeometryPathGetter{std::move(geometryPathGetter_)},
        m_OnLocalCopyEdited{std::move(onLocalCopyEdited_)},
        m_EditedGeometryPath{CopyOrDefaultGeometryPath(m_GeometryPathGetter)}
    {}

    void draw_content()
    {
        if (m_GeometryPathGetter() == nullptr)
        {
            // edge-case: the geometry path that this popup is editing no longer
            // exists (e.g. because a muscle was deleted or similar), so it should
            // announce the problem and close itself
            ui::draw_text("The GeometryPath no longer exists - closing this popup");
            request_close();
            return;
        }
        // else: the geometry path exists, but this UI should edit the cached
        // `m_EditedGeometryPath`, which is independent of the original data
        // and the target component (so that edits can be applied transactionally)

        ui::draw_text("Path Points:");
        ui::draw_separator();
        drawPathPointEditorTable();
        ui::draw_separator();
        drawAddPathPointButton();
        ui::start_new_line();
        drawBottomButtons();
    }

private:
    void drawPathPointEditorTable()
    {
        OpenSim::PathPointSet& pps = m_EditedGeometryPath.updPathPointSet();

        if (ui::begin_table("##GeometryPathEditorTable", 6))
        {
            ui::table_setup_column("Actions");
            ui::table_setup_column("Type");
            ui::table_setup_column("X");
            ui::table_setup_column("Y");
            ui::table_setup_column("Z");
            ui::table_setup_column("Frame");
            ui::table_setup_scroll_freeze(0, 1);
            ui::table_headers_row();

            for (ptrdiff_t i = 0; i < opyn::ssize(pps); ++i)
            {
                ui::push_id(i);
                drawIthPathPointTableRow(pps, i);
                ui::pop_id();
            }

            ui::end_table();
        }

        // perform any actions after rendering the table: in case the action would
        // in some way screw with rendering (e.g. deleting a point midway
        // through rendering a row is probably a bad idea)
        tryExecuteRequestedAction(pps);
    }

    void drawAddPathPointButton()
    {
        if (ui::draw_button(MSMICONS_PLUS_CIRCLE " Add Point"))
        {
            ActionAddNewPathPoint(m_EditedGeometryPath.updPathPointSet());
        }
    }

    void drawIthPathPointTableRow(OpenSim::PathPointSet& pps, ptrdiff_t i)
    {
        int column = 0;

        ui::table_next_row();

        ui::table_set_column_index(column++);
        drawIthPathPointActionsCell(pps, i);

        ui::table_set_column_index(column++);
        drawIthPathPointTypeCell(pps, i);

        tryDrawIthPathPointLocationEditorCells(pps, i, column);

        ui::table_set_column_index(column++);
        drawIthPathPointFrameCell(pps, i);
    }

    void drawIthPathPointActionsCell(OpenSim::PathPointSet& pps, ptrdiff_t i)
    {
        ui::push_style_var(ui::StyleVar::ItemSpacing, {2.0f, 0.0f});

        if (i <= 0) {
            ui::begin_disabled();
        }
        if (ui::draw_small_button(MSMICONS_ARROW_UP)) {
            m_RequestedAction = RequestedAction{RequestedAction::Type::MoveUp, i};
        }
        if (i <= 0) {
            ui::end_disabled();
        }

        ui::same_line();

        if (i+1 >= opyn::ssize(pps)) {
            ui::begin_disabled();
        }
        if (ui::draw_small_button(MSMICONS_ARROW_DOWN)) {
            m_RequestedAction = RequestedAction{RequestedAction::Type::MoveDown, i};
        }
        if (i+1 >= opyn::ssize(pps)) {
            ui::end_disabled();
        }

        ui::same_line();

        ui::push_style_color(ui::ColorVar::Text, Color{0.7f, 0.0f, 0.0f});
        if (ui::draw_small_button(MSMICONS_TRASH))
        {
            m_RequestedAction = RequestedAction{RequestedAction::Type::Delete, i};
        }
        ui::pop_style_color();

        ui::pop_style_var();
    }

    void drawIthPathPointTypeCell(const OpenSim::PathPointSet& pps, ptrdiff_t i)
    {
        ui::draw_text_disabled(opyn::At(pps, i).getConcreteClassName());
    }

    // try, because the path point type might not actually have a set location
    //
    // (e.g. `MovingPathPoint`s)
    void tryDrawIthPathPointLocationEditorCells(OpenSim::PathPointSet& pps, ptrdiff_t i, int& column)
    {
        OpenSim::AbstractPathPoint& app = opyn::At(pps, i);

        if (auto* const pp = dynamic_cast<OpenSim::PathPoint*>(&app))
        {
            const float inputWidth = ui::calc_text_size("0.00000").x();

            SimTK::Vec3& location = pp->upd_location();

            static_assert(c_LocationInputIDs.size() == 3);
            for (size_t dim = 0; dim < c_LocationInputIDs.size(); ++dim)
            {
                auto v = static_cast<float>(location[static_cast<int>(dim)]);

                ui::table_set_column_index(column++);
                ui::set_next_item_width(inputWidth);
                if (ui::draw_float_input(c_LocationInputIDs[dim], &v))
                {
                    location[static_cast<int>(dim)] = static_cast<double>(v);
                }
            }
        }
        else
        {
            // it's some other kind of path point, with no editable X, Y, or Z
            ui::table_set_column_index(column++);
            ui::table_set_column_index(column++);
            ui::table_set_column_index(column++);
        }
    }

    void drawIthPathPointFrameCell(OpenSim::PathPointSet& pps, ptrdiff_t i)
    {
        const float width = ui::calc_text_size("/bodyset/a_typical_body_name").x();

        const std::string& label = opyn::At(pps, i).getSocket("parent_frame").getConnecteePath();

        ui::set_next_item_width(width);
        if (ui::begin_combobox("##framesel", label))
        {
            for (const OpenSim::Frame& frame : m_TargetComponent->getComponent().getComponentList<OpenSim::Frame>())
            {
                const std::string absPath = frame.getAbsolutePathString();
                if (ui::draw_selectable(absPath))
                {
                    ActionSetPathPointFramePath(pps, i, absPath);
                }
            }
            ui::end_combobox();
        }
    }

    void drawBottomButtons()
    {
        if (ui::draw_button("cancel"))
        {
            request_close();
        }

        ui::same_line();

        if (ui::draw_button("save"))
        {
            m_OnLocalCopyEdited(m_EditedGeometryPath);
            request_close();
        }
    }

    void tryExecuteRequestedAction(OpenSim::PathPointSet& pps)
    {
        if (!(0 <= m_RequestedAction.pathPointIndex && m_RequestedAction.pathPointIndex < opyn::ssize(pps)))
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

    std::shared_ptr<const ComponentAccessor> m_TargetComponent;
    std::function<const OpenSim::GeometryPath*()> m_GeometryPathGetter;
    std::function<void(const OpenSim::GeometryPath&)> m_OnLocalCopyEdited;

    OpenSim::GeometryPath m_EditedGeometryPath;
    RequestedAction m_RequestedAction;
};


osc::GeometryPathEditorPopup::GeometryPathEditorPopup(
    Widget* parent_,
    std::string_view popupName_,
    std::shared_ptr<const ComponentAccessor> targetComponent_,
    std::function<const OpenSim::GeometryPath*()> geometryPathGetter_,
    std::function<void(const OpenSim::GeometryPath&)> onLocalCopyEdited_) :

    Popup{std::make_unique<Impl>(*this, parent_, popupName_, std::move(targetComponent_), std::move(geometryPathGetter_), std::move(onLocalCopyEdited_))}
{}
void osc::GeometryPathEditorPopup::impl_draw_content() { private_data().draw_content(); }
