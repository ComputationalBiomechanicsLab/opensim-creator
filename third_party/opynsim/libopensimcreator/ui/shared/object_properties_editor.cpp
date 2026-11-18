#include "object_properties_editor.h"

#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/ui/shared/function_curve_viewer_popup.h>
#include <libopensimcreator/ui/shared/geometry_path_editor_popup.h>

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/documents/model/versioned_component_accessor.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <libopynsim/utilities/simbody_x_oscar.h>
#include <liboscar/graphics/color.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/widget.h>
#include <liboscar/platform/widget_private.h>
#include <liboscar/ui/events/open_panel_event.h>
#include <liboscar/ui/events/open_popup_event.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/utilities/algorithms.h>
#include <liboscar/utilities/string_helpers.h>
#include <liboscar/utilities/typelist.h>
#include <OpenSim/Actuators/ActiveForceLengthCurve.h>
#include <OpenSim/Common/AbstractProperty.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/Object.h>
#include <OpenSim/Common/Property.h>
#include <OpenSim/Simulation/Model/AbstractGeometryPath.h>
#include <OpenSim/Simulation/Model/Appearance.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/GeometryPath.h>
#include <OpenSim/Simulation/Model/HuntCrossleyForce.h>
#include <SimTKcommon/Constants.h>
#include <SimTKcommon/SmallMatrix.h>

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>

using namespace osc;
namespace rgs = std::ranges;

// constants
namespace
{
    inline constexpr float c_initial_step_size = 0.001f;  // effectively, 1 mm or 0.001 rad
}

// helper functions
namespace
{
    // returns an updater function that deletes an element from a list property
    template<typename T>
    std::function<void(OpenSim::AbstractProperty&)> make_simple_property_element_deleter(
        int property_index)
    {
        return [property_index](OpenSim::AbstractProperty& p) {
            auto* const simple_prop = dynamic_cast<OpenSim::SimpleProperty<T>*>(&p);
            if (not simple_prop) {
                return;  // types don't match: caller probably mismatched properties
            }

            auto copy = std::make_unique<OpenSim::SimpleProperty<T>>(
                simple_prop->getName(), simple_prop->isOneValueProperty());
            for (int i = 0; i < simple_prop->size(); ++i) {
                if (i != property_index) {
                    copy->appendValue(simple_prop->getValue(i));
                }
            }

            simple_prop->clear();
            simple_prop->assign(*copy);
        };
    }

    // returns an updater function that sets the value of a property
    template<typename TValue, typename TProperty = std::remove_cvref_t<TValue>>
    std::function<void(OpenSim::AbstractProperty&)> make_property_value_setter(
        int property_index, TValue&& value)
    {
        return [property_index, val = std::forward<TValue>(value)](OpenSim::AbstractProperty& p) {
            auto* const concrete_prop = dynamic_cast<OpenSim::Property<TProperty>*>(&p);
            if (not concrete_prop) {
                return;  // types don't match: caller probably mismatched properties
            }
            concrete_prop->setValue(property_index, val);
        };
    }

    // draws the property name and (optionally) comment tooltip
    void draw_property_name(const OpenSim::AbstractProperty& property)
    {
        ui::align_text_to_frame_padding();  // ensure it aligns with the editors in the next column
        ui::draw_text(property.getName());

        if (not property.getComment().empty()) {
            ui::same_line();
            ui::draw_help_marker(property.getComment());
        }
    }

    // wraps an object accessor with property information so that an individual
    // property accesssor with the same lifetime semantics as the object can exist
    std::function<const OpenSim::AbstractProperty*()> make_property_accessor(
        const std::function<const OpenSim::Object*()>& object_accessor,
        const std::string& property_name)
    {
        return [object_accessor, property_name] -> const OpenSim::AbstractProperty* {
            const OpenSim::Object* obj = object_accessor();
            if (not obj) {
                return nullptr;
            }

            if (not obj->hasProperty(property_name)) {
                return nullptr;
            }

            return &obj->getPropertyByName(property_name);
        };
    }

    // draws a little vertical line, which is usually used to visually indicate
    // x/y/z to the user
    void draw_colored_dimension_hint_vertical_line(const Color& color)
    {
        ui::DrawListView l = ui::get_panel_draw_list();
        const Vector2 p = ui::get_cursor_ui_position();
        const float h = ui::get_text_line_height_in_current_panel() +
                        (2.0f * ui::get_style_frame_padding().y()) +
                        (2.0f * ui::get_style_frame_border_size());
        const Vector2 dims = Vector2{4.0f, h};
        l.add_rect_filled(Rect::from_corners(p, p + dims), color);
        ui::set_cursor_ui_position({p.x() + 4.0f, p.y()});
    }

    // draws a context menu that the user can use to change the step interval of the +/- buttons
    void draw_step_size_editor(float& step_size)
    {
        if (ui::begin_popup_context_menu("##valuecontextmenu")) {
            ui::draw_text("Set Step Size");
            ui::same_line();
            ui::draw_help_marker(
                "Sets the decrement/increment of the + and - buttons. Can be handy for tweaking property values");
            ui::draw_vertical_spacer(0.1f);
            ui::draw_separator();
            ui::draw_vertical_spacer(0.2f);

            if (ui::begin_table("CommonChoicesTable", 2, ui::TableFlag::SizingStretchProp)) {
                ui::table_setup_column("Type");
                ui::table_setup_column("Options");

                ui::table_next_row();
                ui::table_set_column_index(0);
                ui::draw_text("Custom");
                ui::table_set_column_index(1);
                ui::draw_float_input("##stepsizeinput", &step_size, 0.0f, 0.0f, "%.6f");

                ui::table_next_row();
                ui::table_set_column_index(0);
                ui::draw_text("Lengths");
                ui::table_set_column_index(1);
                if (ui::draw_button("10 cm")) {
                    step_size = 0.1f;
                }
                ui::same_line();
                if (ui::draw_button("1 cm")) {
                    step_size = 0.01f;
                }
                ui::same_line();
                if (ui::draw_button("1 mm")) {
                    step_size = 0.001f;
                }
                ui::same_line();
                if (ui::draw_button("0.1 mm")) {
                    step_size = 0.0001f;
                }

                ui::table_next_row();
                ui::table_set_column_index(0);
                ui::draw_text("Angles (Degrees)");
                ui::table_set_column_index(1);
                if (ui::draw_button("180")) {
                    step_size = 180.0f;
                }
                ui::same_line();
                if (ui::draw_button("90")) {
                    step_size = 90.0f;
                }
                ui::same_line();
                if (ui::draw_button("45")) {
                    step_size = 45.0f;
                }
                ui::same_line();
                if (ui::draw_button("10")) {
                    step_size = 10.0f;
                }
                ui::same_line();
                if (ui::draw_button("1")) {
                    step_size = 1.0f;
                }

                ui::table_next_row();
                ui::table_set_column_index(0);
                ui::draw_text("Angles (Radians)");
                ui::table_set_column_index(1);
                if (ui::draw_button("1 pi")) {
                    step_size = pi_v<float>;
                }
                ui::same_line();
                if (ui::draw_button("1/2 pi")) {
                    step_size = pi_v<float> / 2.0f;
                }
                ui::same_line();
                if (ui::draw_button("1/4 pi")) {
                    step_size = pi_v<float> / 4.0f;
                }
                ui::same_line();
                if (ui::draw_button("10/180 pi")) {
                    step_size = (10.0f / 180.0f) * pi_v<float>;
                }
                ui::same_line();
                if (ui::draw_button("1/180 pi")) {
                    step_size = (1.0f / 180.0f) * pi_v<float>;
                }

                ui::table_next_row();
                ui::table_set_column_index(0);
                ui::draw_text("Masses");
                ui::table_set_column_index(1);
                if (ui::draw_button("1 kg")) {
                    step_size = 1.0f;
                }
                ui::same_line();
                if (ui::draw_button("100 g")) {
                    step_size = 0.1f;
                }
                ui::same_line();
                if (ui::draw_button("10 g")) {
                    step_size = 0.01f;
                }
                ui::same_line();
                if (ui::draw_button("1 g")) {
                    step_size = 0.001f;
                }
                ui::same_line();
                if (ui::draw_button("100 mg")) {
                    step_size = 0.0001f;
                }

                ui::end_table();
            }

            ui::end_popup();
        }
    }

    struct ScalarInputRv final {
        bool wasEdited = false;
        bool shouldSave = false;
    };

    ScalarInputRv draw_custom_scalar_input(
        CStringView label, float& value, float& step_size, std::string_view frame_annotation_label)
    {
        ScalarInputRv rv;

        ui::push_style_var(ui::StyleVar::ItemInnerSpacing, {1.0f, 0.0f});
        if (ui::draw_scalar_input(
                label, ui::DataType::Float, &value, &step_size, nullptr, "%.6f")) {
            rv.wasEdited = true;
        }
        ui::pop_style_var();
        rv.shouldSave = ui::should_save_last_drawn_item_value();
        ui::add_screenshot_annotation_to_last_drawn_item(frame_annotation_label);
        ui::draw_tooltip_if_item_hovered(
            "Step Size", "You can right-click to adjust the step size of the buttons");
        draw_step_size_editor(step_size);

        return rv;
    }

    std::string generate_vec_frame_annotation_label(
        const OpenSim::AbstractProperty& backing_property, Vector3::size_type ith_dimension)
    {
        std::stringstream ss;
        ss << "ObjectPropertiesEditor::Vec3/";
        ss << ith_dimension;
        ss << '/';
        ss << backing_property.getName();
        return std::move(ss).str();
    }
}

// property editor base class etc.
namespace
{
    // type-erased property editor
    class IPropertyEditor {
    protected:
        IPropertyEditor() = default;
        IPropertyEditor(const IPropertyEditor&) = default;
        IPropertyEditor(IPropertyEditor&&) noexcept = default;
        IPropertyEditor& operator=(const IPropertyEditor&) = default;
        IPropertyEditor& operator=(IPropertyEditor&&) noexcept = default;

    public:
        virtual ~IPropertyEditor() noexcept = default;

        bool is_compatible_with(const OpenSim::AbstractProperty& prop) const
        {
            return impl_is_compatible_with(prop);
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> on_draw()
        {
            return impl_on_draw();
        }

    private:
        virtual bool impl_is_compatible_with(const OpenSim::AbstractProperty&) const = 0;
        virtual std::optional<std::function<void(OpenSim::AbstractProperty&)>> impl_on_draw() = 0;
    };

    // construction-time arguments for the property editor
    struct PropertyEditorArgs final {
        Widget* parent = nullptr;
        std::shared_ptr<const opyn::VersionedComponentAccessor> component;
        std::function<const OpenSim::Object*()> objectAccessor;
        std::function<const OpenSim::AbstractProperty*()> propertyAccessor;
    };

    template<std::derived_from<OpenSim::AbstractProperty> ConcreteProperty>
    struct PropertyTraits {
        static bool is_compatible_with(const OpenSim::AbstractProperty* prop)
        {
            return dynamic_cast<const ConcreteProperty*>(prop) != nullptr;
        }

        static const ConcreteProperty* try_get_downcasted(const OpenSim::AbstractProperty* prop)
        {
            return dynamic_cast<const ConcreteProperty*>(prop);
        }
    };

    // partial implementation class for a specific property editor
    template<
        std::derived_from<OpenSim::AbstractProperty> ConcreteProperty,
        class Traits = PropertyTraits<ConcreteProperty>>
    class PropertyEditor : public IPropertyEditor {
    public:
        using property_type = ConcreteProperty;

        static bool is_compatible_with(const OpenSim::AbstractProperty& prop)
        {
            return Traits::is_compatible_with(&prop);
        }

        explicit PropertyEditor(PropertyEditorArgs args) :
            args_{std::move(args)}
        {}

    protected:
        const OpenSim::AbstractProperty* try_get_property() const
        {
            return args_.propertyAccessor();
        }

        const property_type* try_get_downcasted_property() const
        {
            return Traits::try_get_downcasted(try_get_property());
        }

        std::function<const property_type*()> get_downcasted_property_accessor() const
        {
            return [inner = args_.propertyAccessor] {
                return Traits::try_get_downcasted(inner());
            };
        }

        const OpenSim::Component& get_root_component() const
        {
            return args_.component->get_component();
        }

        std::shared_ptr<const opyn::VersionedComponentAccessor> try_get_component_shared_ptr() const
        {
            return args_.component;
        }

        const SimTK::State* try_get_state() const
        {
            if (auto* msp = dynamic_cast<const opyn::ModelStatePair*>(args_.component.get())) {
                return &msp->get_state();
            } else {
                return nullptr;
            }
        }

        const OpenSim::Object* try_get_object() const { return args_.objectAccessor(); }

        OpenSim::ComponentPath try_get_object_abs_path() const
        {
            auto* obj = try_get_object();
            if (not obj) {
                return {};
            }
            auto* component = dynamic_cast<const OpenSim::Component*>(obj);
            if (not component) {
                return {};
            }
            return opyn::get_absolute_path(*component);
        }

        Widget* try_get_parent_widget() { return args_.parent; }

    private:
        bool impl_is_compatible_with(const OpenSim::AbstractProperty& prop) const override
        {
            return Traits::is_compatible_with(&prop);
        }

        PropertyEditorArgs args_;
    };
}

// concrete property editors for simple (e.g. bool, double) types
namespace
{
    // concrete property editor for a simple `std::string` value
    class StringPropertyEditor final : public PropertyEditor<OpenSim::SimpleProperty<std::string>> {
    public:
        using PropertyEditor::PropertyEditor;

    private:
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> impl_on_draw() override
        {
            // fetch property from object
            const property_type* prop = try_get_downcasted_property();
            if (not prop) {
                return std::nullopt;
            }

            // update any cached data
            if (*prop != original_property_) {
                original_property_ = *prop;
                edited_property_ = *prop;
            }

            ui::draw_separator();

            // draw the name of the property in left-hand column
            draw_property_name(edited_property_);
            ui::next_column();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < edited_property_.size(); ++idx) {
                ui::push_id(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editor_rv =
                    draw_ith_editor(idx);
                ui::pop_id();

                if (not rv) {
                    rv = std::move(editor_rv);
                }
            }

            // draw "Add Entry" or "Populate" buttons
            if (edited_property_.isListProperty()) {
                // it's a list property, so draw an "Add Entry" button
                //
                // users can use this to add a blank entry to this editor. The entry isn't
                // emitted to the rest of the UI until the user edits it.

                // disable the button if at the maximum list size or the last entry in the
                // list is a blank (probably from the last time the user clicked "Add Entry")
                const bool disabled =
                    (edited_property_.size() >= edited_property_.getMaxListSize()) or
                    (not edited_property_.empty() and
                     edited_property_[edited_property_.size() - 1].empty());

                if (disabled) {
                    ui::begin_disabled();
                }
                if (ui::draw_button(
                        MSMICONS_PLUS " Add Entry",
                        {ui::get_content_region_available().x(), ui::calc_button_size("").y()})) {
                    // append blank entry (don't emit upstream until user edits it)
                    edited_property_.appendValue(std::string{});
                }
                if (disabled) {
                    ui::end_disabled();
                }
            } else if (edited_property_.isOptionalProperty() and edited_property_.empty()) {
                // it's an optional property, so draw a "Populate" button if it's unoccupied
                if (ui::draw_button(
                        MSMICONS_PLUS " Populate",
                        {ui::get_content_region_available().x(), ui::calc_button_size("").y()})) {
                    // append blank entry (don't emit upstream until user edits it)
                    edited_property_.appendValue(std::string{});
                }
            }

            ui::next_column();

            return rv;
        }

        // draw an editor for one of the property's string values (might be a list)
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> draw_ith_editor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // calculate space taken by deletion button at end of line (if necessary)
            const float deletion_button_width =
                edited_property_.size() > edited_property_.getMinListSize()
                    ? ui::calc_button_size(MSMICONS_TRASH).x()
                    : 0.0f;

            // read stored value from edited property
            std::string value = edited_property_.getValue(idx);

            ui::set_next_item_width(ui::get_content_region_available().x() - deletion_button_width);
            if (ui::draw_string_input("##stringeditor", value)) {
                // update the edited property - don't rely on ImGui to remember edits
                edited_property_.setValue(idx, value);
            }

            // globally annotate the editor rect, for downstream screenshot automation
            ui::add_screenshot_annotation_to_last_drawn_item(
                "ObjectPropertiesEditor::StringEditor/" + edited_property_.getName());

            if (ui::should_save_last_drawn_item_value()) {
                rv = make_property_value_setter(idx, edited_property_.getValue(idx));
            }

            // if applicable, add deletion button
            if (edited_property_.size() > edited_property_.getMinListSize()) {
                ui::same_line();
                if (ui::draw_button(MSMICONS_TRASH)) {
                    rv = make_simple_property_element_deleter<std::string>(idx);
                }
            }

            return rv;
        }

        property_type original_property_{"blank", true};
        property_type edited_property_{"blank", true};
    };

    // concrete property editor for a simple double value
    class DoublePropertyEditor final : public PropertyEditor<OpenSim::SimpleProperty<double>> {
    public:
        using PropertyEditor::PropertyEditor;

    private:
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> impl_on_draw() override
        {
            const property_type* maybe_prop = try_get_downcasted_property();
            if (not maybe_prop) {
                return std::nullopt;
            }
            const property_type& prop = *maybe_prop;

            // update any cached data
            if (prop != original_property_) {
                original_property_ = prop;
                edited_property_ = prop;
            }

            ui::draw_separator();

            // draw name of the property in left-hand column
            draw_property_name(edited_property_);
            ui::next_column();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < max(edited_property_.size(), 1); ++idx) {
                ui::push_id(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editor_rv =
                    draw_ith_editor(idx);
                ui::pop_id();

                if (not rv) {
                    rv = std::move(editor_rv);
                }
            }
            ui::next_column();

            return rv;
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> draw_ith_editor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // draw deletion button that can delete an element from the property's list
            if (edited_property_.isListProperty()) {
                if (ui::draw_button(MSMICONS_TRASH)) {
                    rv = make_simple_property_element_deleter<double>(idx);
                }
                ui::same_line();
            }

            // read stored value from edited property
            //
            // care: optional properties have size==0, so perform a range check
            auto value = static_cast<float>(
                idx < edited_property_.size() ? edited_property_.getValue(idx) : 0.0);
            ui::set_next_item_width(ui::get_content_region_available().x());
            auto frame_annotation_label =
                "ObjectPropertiesEditor::DoubleEditor/" + edited_property_.getName();
            auto draw_rv = draw_custom_scalar_input(
                "##doubleeditor", value, step_size_, frame_annotation_label);

            if (draw_rv.wasEdited) {
                // update the edited property - don't rely on ImGui to remember edits
                edited_property_.setValue(idx, static_cast<double>(value));
            }
            if (draw_rv.shouldSave) {
                rv = make_property_value_setter(idx, edited_property_.getValue(idx));
            }

            return rv;
        }

        property_type original_property_{"blank", true};
        property_type edited_property_{"blank", true};
        float step_size_ = c_initial_step_size;
    };

    // concrete property editor for a simple `bool` value
    class BoolPropertyEditor final : public PropertyEditor<OpenSim::SimpleProperty<bool>> {
    public:
        using PropertyEditor::PropertyEditor;

    private:
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> impl_on_draw() override
        {
            const property_type* maybe_prop = try_get_downcasted_property();
            if (not maybe_prop) {
                return std::nullopt;
            }
            const property_type& prop = *maybe_prop;

            // update any cached data
            if (prop != original_property_) {
                original_property_ = prop;
                edited_property_ = prop;
            }

            ui::draw_separator();

            // draw name of the property in left-hand column
            draw_property_name(edited_property_);
            ui::next_column();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < max(edited_property_.size(), 1); ++idx) {
                ui::push_id(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editor_rv =
                    draw_ith_editor(idx);
                ui::pop_id();

                if (not rv) {
                    rv = std::move(editor_rv);
                }
            }
            ui::next_column();

            return rv;
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> draw_ith_editor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // draw deletion button that can delete an element from the property's list
            if (edited_property_.isListProperty()) {
                if (ui::draw_button(MSMICONS_TRASH)) {
                    rv = make_simple_property_element_deleter<bool>(idx);
                }
                ui::same_line();
            }

            // read stored value from edited property
            //
            // care: optional properties have size==0, so perform a range check
            bool value = idx < edited_property_.size() ? edited_property_.getValue(idx) : false;
            bool edited = false;

            ui::set_next_item_width(ui::get_content_region_available().x());
            if (ui::draw_checkbox("##booleditor", &value)) {
                // update the edited property - don't rely on ImGui to remember edits
                edited_property_.setValue(idx, value);
                edited = true;
            }

            // globally annotate the editor rect, for downstream screenshot automation
            ui::add_screenshot_annotation_to_last_drawn_item(
                "ObjectPropertiesEditor::BoolEditor/" + edited_property_.getName());

            if (edited or ui::should_save_last_drawn_item_value()) {
                rv = make_property_value_setter(idx, edited_property_.getValue(idx));
            }

            return rv;
        }

        property_type original_property_{"blank", true};
        property_type edited_property_{"blank", true};
    };

    // concrete property editor for a simple `Vec3` value
    class Vec3PropertyEditor final : public PropertyEditor<OpenSim::SimpleProperty<SimTK::Vec3>> {
    public:
        using PropertyEditor::PropertyEditor;

    private:
        // converter class that changes based on whether the user wants the value in
        // different units, different frame, etc.
        class ValueConverter final {
        public:
            ValueConverter(
                float property_to_edited_value_scaler,
                SimTK::Transform property_to_edited_transform) :

                property_to_edited_value_scaler_{property_to_edited_value_scaler},
                property_to_edited_transform_{std::move(property_to_edited_transform)}
            {}

            Vector3 property_value_to_edited_value(const Vector3& property_value) const
            {
                return to<Vector3>(
                    static_cast<double>(property_to_edited_value_scaler_) *
                    (property_to_edited_transform_ * to<SimTK::Vec3>(property_value)));
            }

            Vector3 edited_value_to_property_value(const Vector3& edited_value) const
            {
                return to<Vector3>(
                    property_to_edited_transform_.invert() *
                    to<SimTK::Vec3>(edited_value / property_to_edited_value_scaler_));
            }

        private:
            float property_to_edited_value_scaler_;
            SimTK::Transform property_to_edited_transform_;
        };

        // returns `true` if the Vec3 property is edited in radians
        bool is_property_edited_in_radians() const
        {
            return is_equal_case_insensitive(edited_property_.getName(), "orientation");
        }

        // If the `Vec3` property has a parent frame, returns a pointer to the frame; otherwise,
        // returns a `nullptr`.
        const OpenSim::PhysicalFrame* try_get_parent_frame() const
        {
            const OpenSim::Object* const obj = try_get_object();
            if (not obj) {
                return nullptr;  // cannot find the property's parent object?
            }

            const auto* const component = dynamic_cast<const OpenSim::Component*>(obj);
            if (not component) {
                return nullptr;  // the object isn't an OpenSim component
            }

            if (&component->getRoot() != &get_root_component()) {
                return nullptr;  // the object is not within the tree of the root component (#800)
            }

            const auto position_prop_name = opyn::try_get_positional_property_name(*component);
            if (not position_prop_name) {
                // the component doesn't have a logical positional property that can be edited with
                // the transform
                return nullptr;
            }

            const OpenSim::Property<SimTK::Vec3>* const prop = try_get_downcasted_property();
            if (not prop) {
                return nullptr;  // can't access the property this editor is ultimately editing
            }

            if (prop->getName() != *position_prop_name) {
                // the property this editor is editing isn't a logically positional one
                return nullptr;
            }

            return opyn::try_get_parent_to_ground_frame(*component);
        }

        // if the Vec3 property has a parent frame, returns a transform that maps the Vec3
        // property's value to ground
        std::optional<SimTK::Transform> get_parent_to_ground_transform() const
        {
            const SimTK::State* state = try_get_state();
            if (not state) {
                return std::nullopt;
            }

            const OpenSim::PhysicalFrame* frame = try_get_parent_frame();
            if (not frame) {
                return std::nullopt;
            }

            return frame->getTransformInGround(*state);
        }

        // if the user has selected a different frame in which to edit 3D quantities, then
        // returns a transform that maps Vec3 properties expressed in ground to the other
        // frame
        std::optional<SimTK::Transform> get_ground_to_user_selected_frame_transform() const
        {
            if (not maybe_user_selected_frame_abs_path_) {
                return std::nullopt;
            }

            const SimTK::State* state = try_get_state();
            if (not state) {
                return std::nullopt;
            }

            const auto* frame = opyn::find_component<OpenSim::Frame>(
                get_root_component(), *maybe_user_selected_frame_abs_path_);
            if (not frame) {
                return std::nullopt;
            }

            return frame->getTransformInGround(*state).invert();
        }

        ValueConverter get_value_converter() const
        {
            float conversion_coefficient = 1.0f;
            if (is_property_edited_in_radians() and not orientation_vals_are_in_radians_) {
                conversion_coefficient = static_cast<float>(SimTK_RADIAN_TO_DEGREE);
            }

            const std::optional<SimTK::Transform> parent2ground = get_parent_to_ground_transform();
            const std::optional<SimTK::Transform> ground2frame =
                get_ground_to_user_selected_frame_transform();
            const SimTK::Transform transform = parent2ground and ground2frame
                                                   ? (*ground2frame) * (*parent2ground)
                                                   : SimTK::Transform{};

            return ValueConverter{conversion_coefficient, transform};
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> impl_on_draw() override
        {
            const property_type* maybe_prop = try_get_downcasted_property();
            if (not maybe_prop) {
                return std::nullopt;
            }
            const property_type& prop = *maybe_prop;

            // update any cached data
            if (prop != original_property_) {
                original_property_ = prop;
                edited_property_ = prop;
            }

            // compute value converter (applies to all values)
            const ValueConverter value_converter = get_value_converter();

            // draw UI

            ui::draw_separator();

            // draw name of the property in left-hand column
            draw_property_name(edited_property_);
            ui::next_column();

            // top line of right column shows "reexpress in" editor (if applicable)
            draw_reexpression_editor_if_applicable();

            // draw radians/degrees conversion toggle button (if applicable)
            draw_degrees_to_radians_conversion_toggle();

            // draw `[0, 1]` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < max(edited_property_.size(), 1); ++idx) {
                ui::push_id(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editor_rv =
                    draw_ith_editor(value_converter, idx);
                ui::pop_id();

                if (not rv) {
                    rv = std::move(editor_rv);
                }
            }
            ui::next_column();

            return rv;
        }

        void draw_reexpression_editor_if_applicable()
        {
            const OpenSim::PhysicalFrame* parent_frame = try_get_parent_frame();
            if (not parent_frame) {
                return;
            }

            const auto& defaulted_label = parent_frame->getName();
            const std::string preview =
                maybe_user_selected_frame_abs_path_
                    ? maybe_user_selected_frame_abs_path_->getComponentName()
                    : std::string{defaulted_label};

            ui::set_next_item_width(
                ui::get_content_region_available().x() - ui::calc_text_size("(?)").x());
            if (ui::begin_combobox("##reexpressioneditor", preview)) {
                int imgui_id = 0;

                // draw "default" (reset) option
                {
                    ui::draw_separator();
                    ui::push_id(imgui_id++);
                    bool selected = !maybe_user_selected_frame_abs_path_.has_value();
                    if (ui::draw_selectable(defaulted_label, &selected)) {
                        maybe_user_selected_frame_abs_path_.reset();
                    }
                    ui::pop_id();
                    ui::draw_separator();
                }

                // draw selectable for each frame in the component tree
                for (const OpenSim::Frame& frame :
                     get_root_component().getComponentList<OpenSim::Frame>()) {
                    const OpenSim::ComponentPath frame_abs_path = opyn::get_absolute_path(frame);

                    ui::push_id(imgui_id++);
                    bool selected = frame_abs_path == maybe_user_selected_frame_abs_path_;
                    if (ui::draw_selectable(frame.getName(), &selected)) {
                        maybe_user_selected_frame_abs_path_ = frame_abs_path;
                    }
                    ui::pop_id();
                }

                ui::end_combobox();
            }
            ui::same_line();

            ui::draw_help_marker(
                "Expression Frame",
                "The coordinate frame in which this quantity is edited.\n\nNote: Changing this only affects the coordinate space the the value is edited in. It does not change the frame that the component is attached to. You can change the frame attachment by using the component's context menu: Socket > $FRAME > (edit button) > (select new frame)");
        }

        // draws an editor for the `ith` Vec3 element of the given (potentially, list) property
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> draw_ith_editor(
            const ValueConverter& value_converter, int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // draw deletion button that can delete an element from the property's list
            if (edited_property_.isListProperty()) {
                if (ui::draw_button(MSMICONS_TRASH)) {
                    rv = make_simple_property_element_deleter<SimTK::Vec3>(idx);
                }
                ui::same_line();
            }

            // read stored value from edited property
            //
            // care: optional properties have size==0, so perform a range check
            const auto raw_value = to<Vector3>(
                idx < edited_property_.size() ? edited_property_.getValue(idx) : SimTK::Vec3{0.0});
            const Vector3 edited_value = value_converter.property_value_to_edited_value(raw_value);

            // draw an editor for each component of the Vec3
            bool should_save = false;
            for (Vector3::size_type i = 0; i < 3; ++i) {
                const ComponentEditorReturn component_editor_rv =
                    draw_vec3_component_editor(idx, i, edited_value, value_converter);
                should_save =
                    should_save or component_editor_rv == ComponentEditorReturn::ShouldSave;
            }

            // if any component editor indicated that it should be saved then propagate that upwards
            if (should_save) {
                rv = make_property_value_setter(idx, edited_property_.getValue(idx));
            }

            return rv;
        }

        enum class ComponentEditorReturn {
            None,
            ShouldSave,
        };

        // draws float input for a single component of the Vec3 (e.g. vec.x)
        ComponentEditorReturn draw_vec3_component_editor(
            int idx,
            Vector3::size_type i,
            Vector3 edited_value,
            const ValueConverter& value_converter)
        {
            ui::push_id(i);
            // draw dimension hint (color bar next to the input)
            draw_colored_dimension_hint_vertical_line(Color(0.0f, 0.6f).with_element(i, 1.0f));

            // draw the input editor
            ui::set_next_item_width(ui::get_content_region_available().x());
            auto frame_annotation = generate_vec_frame_annotation_label(edited_property_, i);
            auto draw_rv = draw_custom_scalar_input(
                "##valueinput", edited_value[i], step_size_, frame_annotation);

            if (draw_rv.wasEdited) {
                // un-convert the value on save
                const Vector3 saved_value =
                    value_converter.edited_value_to_property_value(edited_value);
                edited_property_.setValue(idx, to<SimTK::Vec3>(saved_value));
            }

            ui::pop_id();

            return draw_rv.shouldSave ? ComponentEditorReturn::ShouldSave
                                      : ComponentEditorReturn::None;
        }

        // draws button that lets the user toggle between inputting radians vs. degrees
        void draw_degrees_to_radians_conversion_toggle()
        {
            if (not is_property_edited_in_radians()) {
                return;
            }

            if (orientation_vals_are_in_radians_) {
                if (ui::draw_button("radians")) {
                    orientation_vals_are_in_radians_ = !orientation_vals_are_in_radians_;
                }
                ui::add_screenshot_annotation_to_last_drawn_item(
                    "ObjectPropertiesEditor::OrientationToggle/" + edited_property_.getName());
                ui::draw_tooltip_body_only_if_item_hovered(
                    "This quantity is edited in radians (click to switch to degrees)");
            } else {
                if (ui::draw_button("degrees")) {
                    orientation_vals_are_in_radians_ = !orientation_vals_are_in_radians_;
                }
                ui::add_screenshot_annotation_to_last_drawn_item(
                    "ObjectPropertiesEditor::OrientationToggle/" + edited_property_.getName());
                ui::draw_tooltip_body_only_if_item_hovered(
                    "This quantity is edited in degrees (click to switch to radians)");
            }
        }

        property_type original_property_{"blank", true};
        property_type edited_property_{"blank", true};
        std::optional<OpenSim::ComponentPath> maybe_user_selected_frame_abs_path_;
        float step_size_ = c_initial_step_size;
        bool orientation_vals_are_in_radians_ = false;
    };

    // concrete property editor for a simple `Vec6` value
    class Vec6PropertyEditor final : public PropertyEditor<OpenSim::SimpleProperty<SimTK::Vec6>> {
    public:
        using PropertyEditor::PropertyEditor;

    private:
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> impl_on_draw() override
        {
            const property_type* maybe_prop = try_get_downcasted_property();
            if (not maybe_prop) {
                return std::nullopt;
            }
            const property_type& prop = *maybe_prop;

            // update any cached data
            if (prop != original_property_) {
                original_property_ = prop;
                edited_property_ = prop;
            }

            ui::draw_separator();

            // draw name of the property in left-hand column
            draw_property_name(edited_property_);
            ui::next_column();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < max(edited_property_.size(), 1); ++idx) {
                ui::push_id(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editor_rv =
                    draw_ith_editor(idx);
                ui::pop_id();

                if (not rv) {
                    rv = std::move(editor_rv);
                }
            }
            ui::next_column();

            return rv;
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> draw_ith_editor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // draw deletion button that can delete an element from the property's list
            if (edited_property_.isListProperty()) {
                if (ui::draw_button(MSMICONS_TRASH)) {
                    rv = make_simple_property_element_deleter<SimTK::Vec6>(idx);
                }
            }

            // read latest raw value as-stored in edited property
            //
            // care: `getValue` can return `nullptr` if the property is optional (size == 0)
            std::array<float, 6> raw_value =
                idx < edited_property_.size()
                    ? to<std::array<float, 6>>(edited_property_.getValue(idx))
                    : std::array<float, 6>{};

            bool should_save = false;
            for (int i = 0; i < 2; ++i) {
                ui::push_id(i);

                ui::set_next_item_width(ui::get_content_region_available().x());
                if (ui::draw_float3_input(
                        "##vec6editor", raw_value.data() + static_cast<ptrdiff_t>(3 * i), "%.6f")) {
                    edited_property_.updValue(idx)[3 * i + 0] =
                        static_cast<double>(raw_value[3 * i + 0]);
                    edited_property_.updValue(idx)[3 * i + 1] =
                        static_cast<double>(raw_value[3 * i + 1]);
                    edited_property_.updValue(idx)[3 * i + 2] =
                        static_cast<double>(raw_value[3 * i + 2]);
                }
                should_save = should_save or ui::should_save_last_drawn_item_value();
                ui::add_screenshot_annotation_to_last_drawn_item(
                    "ObjectPropertiesEditor::Vec6Editor/" + edited_property_.getName());

                ui::pop_id();
            }

            if (should_save) {
                rv = make_property_value_setter(idx, edited_property_.getValue(idx));
            }

            return rv;
        }

        property_type original_property_{"blank", true};
        property_type edited_property_{"blank", true};
    };

    class IntPropertyEditor final : public PropertyEditor<OpenSim::SimpleProperty<int>> {
    public:
        using PropertyEditor::PropertyEditor;

    private:
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> impl_on_draw() override
        {
            const property_type* maybe_prop = try_get_downcasted_property();
            if (not maybe_prop) {
                return std::nullopt;
            }
            const property_type& prop = *maybe_prop;

            // update any cached data
            if (prop != original_property_) {
                original_property_ = prop;
                edited_property_ = prop;
            }

            ui::draw_separator();

            // draw name of the property in left-hand column
            draw_property_name(edited_property_);
            ui::next_column();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < max(edited_property_.size(), 1); ++idx) {
                ui::push_id(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editor_rv =
                    draw_ith_editor(idx);
                ui::pop_id();

                if (not rv) {
                    rv = std::move(editor_rv);
                }
            }
            ui::next_column();

            return rv;
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> draw_ith_editor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // draw deletion button that can delete an element from the property's list
            if (edited_property_.isListProperty()) {
                if (ui::draw_button(MSMICONS_TRASH)) {
                    rv = make_simple_property_element_deleter<int>(idx);
                }
                ui::same_line();
            }

            // read stored value from edited property
            //
            // care: optional properties have size==0, so perform a range check
            int value = idx < edited_property_.size() ? edited_property_.getValue(idx) : 0;
            bool edited = false;

            ui::set_next_item_width(ui::get_content_region_available().x());
            if (ui::draw_int_input("##inteditor", &value)) {
                // update the edited property - don't rely on ImGui to remember edits
                edited_property_.setValue(idx, value);
                edited = true;
            }

            // globally annotate the editor rect, for downstream screenshot automation
            ui::add_screenshot_annotation_to_last_drawn_item("ObjectPropertiesEditor::IntEditor/");

            if (edited or ui::should_save_last_drawn_item_value()) {
                rv = make_property_value_setter(idx, edited_property_.getValue(idx));
            }

            return rv;
        }

        property_type original_property_{"blank", true};
        property_type edited_property_{"blank", true};
    };
}

// concrete property editors for object types
namespace
{
    // concrete property editor for an OpenSim::Appearance
    class AppearancePropertyEditor final
        : public PropertyEditor<OpenSim::ObjectProperty<OpenSim::Appearance>> {
    public:
        using PropertyEditor::PropertyEditor;

    private:
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> impl_on_draw() override
        {
            const property_type* maybe_prop = try_get_downcasted_property();
            if (not maybe_prop) {
                return std::nullopt;
            }
            const property_type& prop = *maybe_prop;

            // update any cached data
            if (prop != original_property_) {
                original_property_ = prop;
                edited_property_ = prop;
            }

            ui::draw_separator();

            // draw name of the property in left-hand column
            draw_property_name(edited_property_);
            ui::next_column();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < max(edited_property_.size(), 1); ++idx) {
                ui::push_id(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editor_rv =
                    draw_ith_editor(idx);
                ui::pop_id();

                if (not rv) {
                    rv = std::move(editor_rv);
                }
            }
            ui::next_column();

            return rv;
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> draw_ith_editor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            if (edited_property_.isListProperty()) {
                return rv;  // HACK: ignore list props for now
            }

            if (edited_property_.empty()) {
                return rv;  // HACK: ignore optional props for now
            }

            bool should_save = false;

            Color color = opyn::to_color(edited_property_.getValue());
            ui::set_next_item_width(ui::get_content_region_available().x());

            if (ui::draw_rgba_color_editor("##coloreditor", color)) {
                SimTK::Vec3 new_color;
                new_color[0] = static_cast<double>(color[0]);
                new_color[1] = static_cast<double>(color[1]);
                new_color[2] = static_cast<double>(color[2]);

                edited_property_.updValue().set_color(new_color);
                edited_property_.updValue().set_opacity(static_cast<double>(color[3]));
            }
            should_save = should_save or ui::should_save_last_drawn_item_value();

            bool is_visible = edited_property_.getValue().get_visible();
            if (ui::draw_checkbox("is visible", &is_visible)) {
                edited_property_.updValue().set_visible(is_visible);
            }
            should_save = should_save or ui::should_save_last_drawn_item_value();

            // DisplayPreference
            {
                static_assert(OpenSim::VisualRepresentation::DrawDefault == -1);
                static_assert(OpenSim::VisualRepresentation::Hide == 0);
                static_assert(OpenSim::VisualRepresentation::DrawPoints == 1);
                static_assert(OpenSim::VisualRepresentation::DrawWireframe == 2);
                static_assert(OpenSim::VisualRepresentation::DrawSurface == 3);
                const auto options = std::to_array<CStringView>({
                    "Default",
                    "Hide",
                    "Points",
                    "Wireframe",
                    "Surface",
                });
                size_t index = clamp(
                    static_cast<size_t>(edited_property_.getValue().get_representation()) + 1,
                    0uz,
                    options.size());
                ui::set_next_item_width(ui::get_content_region_available().x());
                if (ui::draw_combobox("##DisplayPref", &index, options)) {
                    edited_property_.updValue().set_representation(
                        static_cast<OpenSim::VisualRepresentation>(static_cast<int>(index) - 1));
                    should_save = true;
                }
            }

            if (should_save) {
                rv = make_property_value_setter(idx, edited_property_.getValue(idx));
            }

            return rv;
        }

        property_type original_property_{"blank", true};
        property_type edited_property_{"blank", true};
    };

    // concrete property editor for an `OpenSim::HuntCrossleyForce::ContactParametersSet`
    class ContactParameterSetEditor final
        : public PropertyEditor<
              OpenSim::ObjectProperty<OpenSim::HuntCrossleyForce::ContactParametersSet>> {
    public:
        using PropertyEditor::PropertyEditor;

    private:
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> impl_on_draw() override
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            const property_type* maybe_prop = try_get_downcasted_property();
            if (not maybe_prop) {
                return std::nullopt;  // cannot find property
            }
            const property_type& prop = *maybe_prop;

            if (opyn::empty(prop.getValue())) {
                return std::nullopt;  // no editable contact set on the property
            }

            const OpenSim::HuntCrossleyForce::ContactParameters& params = prop.getValue()[0];

            // update cached editors, if necessary
            if (not maybe_nested_editor_) {
                if (const auto component_ptr = try_get_component_shared_ptr()) {
                    maybe_nested_editor_.emplace(
                        try_get_parent_widget(), component_ptr, [&params]() { return &params; });
                }
            }
            ObjectPropertiesEditor& nested_editor = *maybe_nested_editor_;

            ui::set_num_columns();
            auto resp = nested_editor.on_draw();
            ui::set_num_columns(2);

            if (resp) {
                // careful here: the response has a correct updater but doesn't know the full
                // path to the housing component, so we have to wrap the updater with
                // appropriate lookups etc

                rv = [=](OpenSim::AbstractProperty& p) mutable {
                    auto* downcasted = dynamic_cast<
                        OpenSim::Property<OpenSim::HuntCrossleyForce::ContactParametersSet>*>(&p);
                    if (downcasted and not opyn::empty(downcasted->getValue())) {
                        OpenSim::HuntCrossleyForce::ContactParameters& contact_params =
                            opyn::at(downcasted->updValue(), 0);
                        if (params.hasProperty(resp->get_property_name())) {
                            OpenSim::AbstractProperty& child_p =
                                contact_params.updPropertyByName(resp->get_property_name());
                            resp->apply(child_p);
                        }
                    }
                };
            }

            return rv;
        }

        std::optional<ObjectPropertiesEditor> maybe_nested_editor_;
    };

    // concrete property editor for an OpenSim::GeometryPath
    class AbstractGeometryPathPropertyEditor final
        : public PropertyEditor<OpenSim::ObjectProperty<OpenSim::AbstractGeometryPath>> {
    public:
        using PropertyEditor::PropertyEditor;

    private:
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> impl_on_draw() override
        {
            const property_type* maybe_prop = try_get_downcasted_property();
            if (not maybe_prop) {
                return std::nullopt;
            }
            const property_type& prop = *maybe_prop;

            ui::draw_separator();
            draw_property_name(prop);
            ui::next_column();
            {
                Widget* parent_widget = try_get_parent_widget();
                const auto component_ptr = try_get_component_shared_ptr();
                if (parent_widget and component_ptr) {
                    if (ui::draw_button(MSMICONS_EDIT " edit ")) {
                        App::post_event<OpenPopupEvent>(
                            *parent_widget, create_geometry_path_editor_popup(component_ptr));
                    }
                } else {
                    // ensure it aligns with the property name in the previous column
                    ui::align_text_to_frame_padding();
                    ui::draw_text(prop.toString());
                }
            }
            ui::next_column();

            if (auto rv = std::exchange(*return_value_holder_, std::nullopt)) {
                return rv->get_updater();
            } else {
                return std::nullopt;
            }
        }

        std::unique_ptr<Popup> create_geometry_path_editor_popup(
            const std::shared_ptr<const opyn::ComponentAccessor>& component_ptr)
        {
            const auto accessor = get_downcasted_property_accessor();
            return std::make_unique<GeometryPathEditorPopup>(
                try_get_parent_widget(),
                "Edit Geometry Path",
                component_ptr,
                [accessor] -> const OpenSim::GeometryPath* {
                    const property_type* p = accessor();
                    if (not p or p->isListProperty()) {
                        return nullptr;
                    }
                    return dynamic_cast<const OpenSim::GeometryPath*>(&p->getValueAsObject());
                },
                [shared = return_value_holder_, accessor](const OpenSim::GeometryPath& gp) mutable {
                    if (const property_type* prop = accessor()) {
                        *shared = ObjectPropertyEdit{
                            *prop,
                            make_property_value_setter<
                                const OpenSim::GeometryPath&,
                                OpenSim::AbstractGeometryPath>(0, gp)};
                    }
                });
        }

        // shared between this property editor and a popup it may have spawned
        std::shared_ptr<std::optional<ObjectPropertyEdit>> return_value_holder_ =
            std::make_shared<std::optional<ObjectPropertyEdit>>();
    };

    struct FunctionPropertyEditorTraits {
        static bool is_compatible_with(const OpenSim::AbstractProperty* prop)
        {
            if (not prop) {
                return false;
            }
            if (not prop->isObjectProperty()) {
                return false;
            }
            if (prop->empty()) {
                return false;
            }

            return dynamic_cast<const OpenSim::Function*>(&prop->getValueAsObject(0)) != nullptr;
        }

        static const OpenSim::ObjectProperty<OpenSim::Function>* try_get_downcasted(
            const OpenSim::AbstractProperty*)
        {
            // there's no safe way to upcast an `OpenSim::Property<SpecializedFunction>` -->
            // `OpenSim::Property<Function>`
            //
            // this trait implementation's job is just to ensure the `PropertyEditor` for a function
            // is selected so that we can read data out of it generically (rather than actually edit
            // it)
            return nullptr;
        }
    };

    class FunctionPropertyEditor final : public PropertyEditor<
                                             OpenSim::ObjectProperty<OpenSim::Function>,
                                             FunctionPropertyEditorTraits> {
    public:
        using PropertyEditor::PropertyEditor;

    private:
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> impl_on_draw() override
        {
            const OpenSim::AbstractProperty* prop = try_get_property();
            if (not prop) {
                return std::nullopt;
            }

            ui::draw_separator();
            draw_property_name(*prop);

            ui::next_column();
            {
                Widget* parent_widget = try_get_parent_widget();
                const auto component_ptr = try_get_component_shared_ptr();
                if (parent_widget and component_ptr) {
                    if (ui::draw_button(MSMICONS_EYE " view ")) {

                        // care: the accessor here differs from the default because the user's
                        // selection can change the accessor's behavior. This is a panel, so it
                        // should stick to whatever was selected when the panel was spawned.
                        auto panel = std::make_unique<FunctionCurveViewerPanel>(
                            parent_widget,
                            generate_popup_name(*prop),
                            component_ptr,
                            [component = component_ptr,
                             parent_path = try_get_object_abs_path(),
                             propname = prop->getName()] -> const OpenSim::Function* {
                                auto* parent_component =
                                    opyn::find_component(*component, parent_path);
                                if (not parent_component) {
                                    return nullptr;
                                }
                                if (not parent_component->hasProperty(propname)) {
                                    return nullptr;
                                }
                                auto& prop = parent_component->getPropertyByName(propname);
                                if (prop.empty()) {
                                    return nullptr;
                                }
                                if (not prop.isObjectProperty()) {
                                    return nullptr;
                                }
                                if (prop.empty()) {
                                    return nullptr;
                                }
                                return dynamic_cast<const OpenSim::Function*>(
                                    &prop.getValueAsObject(0));
                            });
                        App::post_event<OpenPanelEvent>(*parent_widget, std::move(panel));
                    }
                } else {
                    // ensure it aligns with the property name in the previous column
                    ui::align_text_to_frame_padding();
                    ui::draw_text(prop->toString());
                }
            }

            ui::draw_tooltip_if_item_hovered(
                "View Function",
                MSMICONS_MAGIC
                " Experimental Feature " MSMICONS_MAGIC
                ": currently, plots the `OpenSim::Function`, but it doesn't know what the X or Y axes are, or what values might be reasonable for either. It also doesn't spawn a non-modal panel, which would be handy if you wanted to view multiple functions at the same time - I should work on that ;)");
            ui::same_line();
            // ensure it aligns with the property name in the previous column
            ui::align_text_to_frame_padding();
            ui::draw_text(prop->getTypeName());
            ui::next_column();

            return std::nullopt;
        }

        std::string generate_popup_name(const OpenSim::AbstractProperty& prop) const
        {
            std::stringstream ss;
            ss << "View ";
            if (const OpenSim::Object* obj = try_get_object()) {
                ss << obj->getName() << '/';
            }
            ss << prop.getName() << " (" << prop.getTypeName() << ')';
            return std::move(ss).str();
        }
    };
}

namespace
{
    // compile-time list of all property editor types
    //
    // can be used with fold expressions etc. to generate runtime types
    using PropertyEditors = Typelist<
        StringPropertyEditor,
        DoublePropertyEditor,
        BoolPropertyEditor,
        Vec3PropertyEditor,
        Vec6PropertyEditor,
        IntPropertyEditor,
        AppearancePropertyEditor,
        ContactParameterSetEditor,
        AbstractGeometryPathPropertyEditor,
        FunctionPropertyEditor>;

    // a type-erased entry in the runtime registry LUT
    class PropertyEditorRegistryEntry final {
    public:
        // a function that tests whether a property editor in the registry is suitable
        // for the given abstract property
        using PropertyEditorTester = bool (*)(const OpenSim::AbstractProperty&);

        // a function that can be used to construct a pointer to a virtual property editor
        using PropertyEditorCtor = std::unique_ptr<IPropertyEditor> (*)(PropertyEditorArgs);

        // create a type-erased entry from a known, concrete, editor
        template<std::derived_from<IPropertyEditor> ConcretePropertyEditor>
        static constexpr PropertyEditorRegistryEntry make_entry()
        {
            const auto tester_fn = ConcretePropertyEditor::is_compatible_with;
            const auto type_erased_ctor_fn =
                [](PropertyEditorArgs args) -> std::unique_ptr<IPropertyEditor> {
                return std::make_unique<ConcretePropertyEditor>(std::move(args));
            };

            return PropertyEditorRegistryEntry{tester_fn, type_erased_ctor_fn};
        }

        bool is_compatible_with(const OpenSim::AbstractProperty& abstract_prop) const
        {
            return tester_(abstract_prop);
        }

        std::unique_ptr<IPropertyEditor> construct(PropertyEditorArgs args) const
        {
            return ctor_(std::move(args));
        }

    private:
        constexpr PropertyEditorRegistryEntry(
            PropertyEditorTester tester, PropertyEditorCtor ctor) :

            tester_{tester},
            ctor_{ctor}
        {}

        PropertyEditorTester tester_;
        PropertyEditorCtor ctor_;
    };

    // runtime type-erased registry for all property editors
    class PropertyEditorRegistry final {
    public:
        std::unique_ptr<IPropertyEditor> try_create_editor(PropertyEditorArgs args) const
        {
            const OpenSim::AbstractProperty* prop = args.propertyAccessor();
            if (not prop) {
                return nullptr;  // cannot access the property
            }

            const auto it = rgs::find_if(
                entries_, [&prop](const auto& entry) { return entry.is_compatible_with(*prop); });
            if (it == entries_.end()) {
                return nullptr;  // no property editor registered for the given typeid
            }

            return it->construct(std::move(args));
        }

    private:
        using storage_type =
            std::array<PropertyEditorRegistryEntry, typelist_size_v<PropertyEditors>>;

        storage_type entries_ =
            []<class... ConcretePropertyEditors>(Typelist<ConcretePropertyEditors...>) {
                return std::to_array(
                    {PropertyEditorRegistryEntry::make_entry<ConcretePropertyEditors>()...});
            }(PropertyEditors{});
    };

    constexpr PropertyEditorRegistry c_registry;
}

// top-level implementation of the properties editor
class osc::ObjectPropertiesEditor::Impl final : public WidgetPrivate {
public:
    explicit Impl(
        Widget& owner,
        Widget* parent,
        std::shared_ptr<const opyn::VersionedComponentAccessor> target_component,
        std::function<const OpenSim::Object*()> object_getter) :

        WidgetPrivate{owner, parent},
        target_component_{std::move(target_component)},
        object_getter_{std::move(object_getter)}
    {}

    std::optional<ObjectPropertyEdit> on_draw()
    {
        const bool disabled = target_component_->is_readonly();
        if (disabled) {
            ui::begin_disabled();
        }

        std::optional<ObjectPropertyEdit> rv;
        if (const OpenSim::Object* maybe_obj = object_getter_()) {
            rv = draw_property_editors(*maybe_obj);  // object accessible: draw property editors
        }

        if (disabled) {
            ui::end_disabled();
        }

        return rv;
    }

    void insert_in_blacklist(std::string_view property_name)
    {
        blacklist_.insert(std::string{property_name});
    }

private:
    // draws all property editors for the given object
    std::optional<ObjectPropertyEdit> draw_property_editors(const OpenSim::Object& obj)
    {
        if (previous_object_ != &obj) {
            // the object has changed since the last draw call, so
            // reset all property editor state
            property_editors_by_name_.clear();
            previous_object_ = &obj;
        }

        // draw each editor and return the last property edit (or std::nullopt)
        std::optional<ObjectPropertyEdit> rv;

        ui::set_num_columns(2);
        for (int i = 0; i < obj.getNumProperties(); ++i) {
            ui::push_id(i);
            std::optional<ObjectPropertyEdit> maybe_edit =
                try_draw_property_editor(obj, obj.getPropertyByIndex(i));
            ui::pop_id();

            if (maybe_edit) {
                rv = std::move(maybe_edit);
            }
        }
        ui::set_num_columns();

        return rv;
    }

    // tries to draw one property editor for one property of an object
    std::optional<ObjectPropertyEdit> try_draw_property_editor(
        const OpenSim::Object& obj, const OpenSim::AbstractProperty& prop)
    {
        if (prop.getName().starts_with("socket_")) {
            // #542: ignore properties that begin with `socket_`, because
            // they are proxy properties to the object's sockets and should
            // be manipulated via socket, rather than property, editors
            return std::nullopt;
        }
        if (blacklist_.contains(prop.getName())) {
            return std::nullopt;
        } else if (IPropertyEditor* maybe_editor = try_get_property_editor(prop)) {
            return draw_property_editor(obj, prop, *maybe_editor);
        } else {
            draw_non_editable_property_details(prop);
            return std::nullopt;
        }
    }

    // draws a property editor for the given object+property
    std::optional<ObjectPropertyEdit> draw_property_editor(
        const OpenSim::Object& obj, const OpenSim::AbstractProperty& prop, IPropertyEditor& editor)
    {
        ui::push_id(prop.getName());
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> maybe_updater =
            editor.on_draw();
        ui::pop_id();

        if (maybe_updater) {
            return ObjectPropertyEdit{obj, prop, std::move(maybe_updater).value()};
        } else {
            return std::nullopt;
        }
    }

    // draws a non-editable representation of a property
    void draw_non_editable_property_details(const OpenSim::AbstractProperty& prop)
    {
        ui::draw_separator();
        draw_property_name(prop);
        ui::next_column();
        // ensure it aligns with the property name in the previous column
        ui::align_text_to_frame_padding();
        ui::draw_text(prop.toString());
        ui::next_column();
    }

    // try get/construct a property editor for the given property
    IPropertyEditor* try_get_property_editor(const OpenSim::AbstractProperty& prop)
    {
        const auto [it, inserted] = property_editors_by_name_.try_emplace(prop.getName(), nullptr);

        if (inserted or (it->second and not it->second->is_compatible_with(prop))) {
            // need to create a new editor because either it hasn't been made yet or the existing
            // editor is for a different type
            auto accessor = make_property_accessor(object_getter_, prop.getName());
            it->second = c_registry.try_create_editor({
                .parent = parent(),
                .component = target_component_,
                .objectAccessor = object_getter_,
                .propertyAccessor = std::move(accessor),
            });
        }

        return it->second.get();
    }

    std::shared_ptr<const opyn::VersionedComponentAccessor> target_component_;
    std::function<const OpenSim::Object*()> object_getter_;
    std::unordered_set<std::string> blacklist_;
    const OpenSim::Object* previous_object_ = nullptr;
    std::unordered_map<std::string, std::unique_ptr<IPropertyEditor>> property_editors_by_name_;
};

osc::ObjectPropertiesEditor::ObjectPropertiesEditor(
    Widget* parent,
    std::shared_ptr<const opyn::VersionedComponentAccessor> target_component,
    std::function<const OpenSim::Object*()> object_getter) :

    Widget{std::make_unique<Impl>(
        *this, parent, std::move(target_component), std::move(object_getter))}
{}
void osc::ObjectPropertiesEditor::insert_in_blacklist(std::string_view property_name)
{
    private_data().insert_in_blacklist(property_name);
}
std::optional<ObjectPropertyEdit> osc::ObjectPropertiesEditor::on_draw()
{
    return private_data().on_draw();
}
void osc::ObjectPropertiesEditor::impl_on_draw()
{
    private_data().on_draw();
}
