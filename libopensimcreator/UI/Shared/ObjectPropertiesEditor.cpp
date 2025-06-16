#include "ObjectPropertiesEditor.h"

#include <libopensimcreator/Documents/Model/IModelStatePair.h>
#include <libopensimcreator/Documents/Model/IVersionedComponentAccessor.h>
#include <libopensimcreator/Platform/IconCodepoints.h>
#include <libopensimcreator/UI/Shared/FunctionCurveViewerPopup.h>
#include <libopensimcreator/UI/Shared/GeometryPathEditorPopup.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>
#include <libopensimcreator/Utils/SimTKConverters.h>

#include <liboscar/Graphics/Color.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/Maths/Vec3.h>
#include <liboscar/Maths/Vec4.h>
#include <liboscar/Platform/App.h>
#include <liboscar/Platform/Log.h>
#include <liboscar/Platform/Widget.h>
#include <liboscar/Platform/WidgetPrivate.h>
#include <liboscar/UI/Events/OpenPanelEvent.h>
#include <liboscar/UI/Events/OpenPopupEvent.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/Utils/Algorithms.h>
#include <liboscar/Utils/StringHelpers.h>
#include <liboscar/Utils/Typelist.h>
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
#include <ranges>
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
    inline constexpr float c_InitialStepSize = 0.001f;  // effectively, 1 mm or 0.001 rad
}

// helper functions
namespace
{
    // returns an updater function that deletes an element from a list property
    template<typename T>
    std::function<void(OpenSim::AbstractProperty&)> MakeSimplePropertyElementDeleter(int propertyIndex)
    {
        return [propertyIndex](OpenSim::AbstractProperty& p)
        {
            auto* const simpleProp = dynamic_cast<OpenSim::SimpleProperty<T>*>(&p);
            if (not simpleProp) {
                return;  // types don't match: caller probably mismatched properties
            }

            auto copy = std::make_unique<OpenSim::SimpleProperty<T>>(simpleProp->getName(), simpleProp->isOneValueProperty());
            for (int i = 0; i < simpleProp->size(); ++i) {
                if (i != propertyIndex) {
                    copy->appendValue(simpleProp->getValue(i));
                }
            }

            simpleProp->clear();
            simpleProp->assign(*copy);
        };
    }

    // returns an updater function that sets the value of a property
    template<
        typename TValue,
        typename TProperty = std::remove_cvref_t<TValue>
    >
    std::function<void(OpenSim::AbstractProperty&)> MakePropertyValueSetter(int propertyIndex, TValue&& value)
    {
        return [propertyIndex, val = std::forward<TValue>(value)](OpenSim::AbstractProperty& p)
        {
            auto* const concreteProp = dynamic_cast<OpenSim::Property<TProperty>*>(&p);
            if (not concreteProp) {
                return;  // types don't match: caller probably mismatched properties
            }
            concreteProp->setValue(propertyIndex, val);
        };
    }

    // draws the property name and (optionally) comment tooltip
    void DrawPropertyName(const OpenSim::AbstractProperty& property)
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
    std::function<const OpenSim::AbstractProperty*()> MakePropertyAccessor(
        const std::function<const OpenSim::Object*()>& objectAccessor,
        const std::string& propertyName)
    {
        return [objectAccessor, propertyName]() -> const OpenSim::AbstractProperty*
        {
            const OpenSim::Object* obj = objectAccessor();
            if (not obj) {
                return nullptr;
            }

            if (not obj->hasProperty(propertyName)) {
                return nullptr;
            }

            return &obj->getPropertyByName(propertyName);
        };
    }

    // draws a little vertical line, which is usually used to visually indicate
    // x/y/z to the user
    void DrawColoredDimensionHintVerticalLine(const Color& color)
    {
        ui::DrawListView l = ui::get_panel_draw_list();
        const Vec2 p = ui::get_cursor_ui_pos();
        const float h = ui::get_text_line_height_in_current_panel() + 2.0f*ui::get_style_frame_padding().y + 2.0f*ui::get_style_frame_border_size();
        const Vec2 dims = Vec2{4.0f, h};
        l.add_rect_filled({p, p + dims}, color);
        ui::set_cursor_ui_pos({p.x + 4.0f, p.y});
    }

    // draws a context menu that the user can use to change the step interval of the +/- buttons
    void DrawStepSizeEditor(float& stepSize)
    {
        if (ui::begin_popup_context_menu("##valuecontextmenu")) {
            ui::draw_text("Set Step Size");
            ui::same_line();
            ui::draw_help_marker("Sets the decrement/increment of the + and - buttons. Can be handy for tweaking property values");
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
                ui::draw_float_input("##stepsizeinput", &stepSize, 0.0f, 0.0f, "%.6f");

                ui::table_next_row();
                ui::table_set_column_index(0);
                ui::draw_text("Lengths");
                ui::table_set_column_index(1);
                if (ui::draw_button("10 cm")) {
                    stepSize = 0.1f;
                }
                ui::same_line();
                if (ui::draw_button("1 cm")) {
                    stepSize = 0.01f;
                }
                ui::same_line();
                if (ui::draw_button("1 mm")) {
                    stepSize = 0.001f;
                }
                ui::same_line();
                if (ui::draw_button("0.1 mm")) {
                    stepSize = 0.0001f;
                }

                ui::table_next_row();
                ui::table_set_column_index(0);
                ui::draw_text("Angles (Degrees)");
                ui::table_set_column_index(1);
                if (ui::draw_button("180")) {
                    stepSize = 180.0f;
                }
                ui::same_line();
                if (ui::draw_button("90")) {
                    stepSize = 90.0f;
                }
                ui::same_line();
                if (ui::draw_button("45")) {
                    stepSize = 45.0f;
                }
                ui::same_line();
                if (ui::draw_button("10")) {
                    stepSize = 10.0f;
                }
                ui::same_line();
                if (ui::draw_button("1")) {
                    stepSize = 1.0f;
                }

                ui::table_next_row();
                ui::table_set_column_index(0);
                ui::draw_text("Angles (Radians)");
                ui::table_set_column_index(1);
                if (ui::draw_button("1 pi")) {
                    stepSize = pi_v<float>;
                }
                ui::same_line();
                if (ui::draw_button("1/2 pi")) {
                    stepSize = pi_v<float>/2.0f;
                }
                ui::same_line();
                if (ui::draw_button("1/4 pi")) {
                    stepSize = pi_v<float>/4.0f;
                }
                ui::same_line();
                if (ui::draw_button("10/180 pi")) {
                    stepSize = (10.0f/180.0f) * pi_v<float>;
                }
                ui::same_line();
                if (ui::draw_button("1/180 pi")) {
                    stepSize = (1.0f/180.0f) * pi_v<float>;
                }

                ui::table_next_row();
                ui::table_set_column_index(0);
                ui::draw_text("Masses");
                ui::table_set_column_index(1);
                if (ui::draw_button("1 kg")) {
                    stepSize = 1.0f;
                }
                ui::same_line();
                if (ui::draw_button("100 g")) {
                    stepSize = 0.1f;
                }
                ui::same_line();
                if (ui::draw_button("10 g")) {
                    stepSize = 0.01f;
                }
                ui::same_line();
                if (ui::draw_button("1 g")) {
                    stepSize = 0.001f;
                }
                ui::same_line();
                if (ui::draw_button("100 mg")) {
                    stepSize = 0.0001f;
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

    ScalarInputRv DrawCustomScalarInput(
        CStringView label,
        float& value,
        float& stepSize,
        std::string_view frameAnnotationLabel)
    {
        ScalarInputRv rv;

        ui::push_style_var(ui::StyleVar::ItemInnerSpacing, {1.0f, 0.0f});
        if (ui::draw_scalar_input(label, ui::DataType::Float, &value, &stepSize, nullptr, "%.6f")) {
            rv.wasEdited = true;
        }
        ui::pop_style_var();
        rv.shouldSave = ui::should_save_last_drawn_item_value();
        ui::add_screenshot_annotation_to_last_drawn_item(frameAnnotationLabel);
        ui::draw_tooltip_if_item_hovered("Step Size", "You can right-click to adjust the step size of the buttons");
        DrawStepSizeEditor(stepSize);

        return rv;
    }

    std::string GenerateVecFrameAnnotationLabel(
        const OpenSim::AbstractProperty& backingProperty,
        Vec3::size_type ithDimension)
    {
        std::stringstream ss;
        ss << "ObjectPropertiesEditor::Vec3/";
        ss << ithDimension;
        ss << '/';
        ss << backingProperty.getName();
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

        bool isCompatibleWith(const OpenSim::AbstractProperty& prop) const
        {
            return implIsCompatibleWith(prop);
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> onDraw()
        {
            return implOnDraw();
        }

    private:
        virtual bool implIsCompatibleWith(const OpenSim::AbstractProperty&) const = 0;
        virtual std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() = 0;
    };

    // construction-time arguments for the property editor
    struct PropertyEditorArgs final {
        Widget* parent = nullptr;
        std::shared_ptr<const IVersionedComponentAccessor> component;
        std::function<const OpenSim::Object*()> objectAccessor;
        std::function<const OpenSim::AbstractProperty*()> propertyAccessor;
    };

    template<std::derived_from<OpenSim::AbstractProperty> ConcreteProperty>
    struct PropertyTraits {
        static bool IsCompatibleWith(const OpenSim::AbstractProperty* prop)
        {
            return dynamic_cast<const ConcreteProperty*>(prop) != nullptr;
        }

        static const ConcreteProperty* TryGetDowncasted(const OpenSim::AbstractProperty* prop)
        {
            return dynamic_cast<const ConcreteProperty*>(prop);
        }
    };

    // partial implementation class for a specific property editor
    template<
        std::derived_from<OpenSim::AbstractProperty> ConcreteProperty,
        class Traits = PropertyTraits<ConcreteProperty>
    >
    class PropertyEditor : public IPropertyEditor {
    public:
        using property_type = ConcreteProperty;

        static bool IsCompatibleWith(const OpenSim::AbstractProperty& prop)
        {
            return Traits::IsCompatibleWith(&prop);
        }

        explicit PropertyEditor(PropertyEditorArgs args) :
            m_Args{std::move(args)}
        {}

    protected:
        const OpenSim::AbstractProperty* tryGetProperty() const
        {
            return m_Args.propertyAccessor();
        }

        const property_type* tryGetDowncastedProperty() const
        {
            return Traits::TryGetDowncasted(tryGetProperty());
        }

        std::function<const property_type*()> getDowncastedPropertyAccessor() const
        {
            return [inner = m_Args.propertyAccessor]()
            {
                return Traits::TryGetDowncasted(inner());
            };
        }

        const OpenSim::Component& getRootComponent() const
        {
            return m_Args.component->getComponent();
        }

        std::shared_ptr<const IVersionedComponentAccessor> tryGetComponentSharedPtr() const
        {
            return m_Args.component;
        }

        const SimTK::State* tryGetState() const
        {
            if (auto* msp = dynamic_cast<const IModelStatePair*>(m_Args.component.get())) {
                return &msp->getState();
            }
            else {
                return nullptr;
            }
        }

        const OpenSim::Object* tryGetObject() const
        {
            return m_Args.objectAccessor();
        }

        OpenSim::ComponentPath tryGetObjectAbsPath() const
        {
            auto* obj = tryGetObject();
            if (not obj) {
                return {};
            }
            auto* component = dynamic_cast<const OpenSim::Component*>(obj);
            if (not component) {
                return {};
            }
            return GetAbsolutePath(*component);
        }

        Widget* tryGetParentWidget()
        {
            return m_Args.parent;
        }

    private:
        bool implIsCompatibleWith(const OpenSim::AbstractProperty& prop) const final
        {
            return Traits::IsCompatibleWith(&prop);
        }

        PropertyEditorArgs m_Args;
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
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() final
        {
            // fetch property from object
            const property_type* prop = tryGetDowncastedProperty();
            if (not prop) {
                return std::nullopt;
            }

            // update any cached data
            if (*prop != m_OriginalProperty) {
                m_OriginalProperty = *prop;
                m_EditedProperty = *prop;
            }

            ui::draw_separator();

            // draw the name of the property in left-hand column
            DrawPropertyName(m_EditedProperty);
            ui::next_column();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < m_EditedProperty.size(); ++idx) {
                ui::push_id(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editorRv = drawIthEditor(idx);
                ui::pop_id();

                if (not rv) {
                    rv = std::move(editorRv);
                }
            }

            // draw "Add Entry" or "Populate" buttons
            if (m_EditedProperty.isListProperty()) {
                // it's a list property, so draw an "Add Entry" button
                //
                // users can use this to add a blank entry to this editor. The entry isn't
                // emitted to the rest of the UI until the user edits it.

                // disable the button if at the maximum list size or the last entry in the
                // list is a blank (probably from the last time the user clicked "Add Entry")
                const bool disabled =
                    (m_EditedProperty.size() >= m_EditedProperty.getMaxListSize()) or
                    (not m_EditedProperty.empty() and m_EditedProperty[m_EditedProperty.size() - 1].empty());

                if (disabled) {
                    ui::begin_disabled();
                }
                if (ui::draw_button(OSC_ICON_PLUS " Add Entry", { ui::get_content_region_available().x, ui::calc_button_size("").y })) {
                    m_EditedProperty.appendValue(std::string{});  // append blank entry (don't emit upstream until user edits it)
                }
                if (disabled) {
                    ui::end_disabled();
                }
            }
            else if (m_EditedProperty.isOptionalProperty() and m_EditedProperty.empty()) {
                // it's an optional property, so draw a "Populate" button if it's unoccupied
                if (ui::draw_button(OSC_ICON_PLUS " Populate", { ui::get_content_region_available().x, ui::calc_button_size("").y })) {
                    m_EditedProperty.appendValue(std::string{});  // append blank entry (don't emit upstream until user edits it)
                }
            }

            ui::next_column();

            return rv;
        }

        // draw an editor for one of the property's string values (might be a list)
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> drawIthEditor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // calculate space taken by deletion button at end of line (if necessary)
            const float deletionButtonWidth = m_EditedProperty.size() > m_EditedProperty.getMinListSize() ?
                ui::calc_button_size(OSC_ICON_TRASH).x :
                0.0f;

            // read stored value from edited property
            std::string value = m_EditedProperty.getValue(idx);

            ui::set_next_item_width(ui::get_content_region_available().x - deletionButtonWidth);
            if (ui::draw_string_input("##stringeditor", value)) {
                // update the edited property - don't rely on ImGui to remember edits
                m_EditedProperty.setValue(idx, value);
            }

            // globally annotate the editor rect, for downstream screenshot automation
            ui::add_screenshot_annotation_to_last_drawn_item("ObjectPropertiesEditor::StringEditor/" + m_EditedProperty.getName());

            if (ui::should_save_last_drawn_item_value()) {
                rv = MakePropertyValueSetter(idx, m_EditedProperty.getValue(idx));
            }

            // if applicable, add deletion button
            if (m_EditedProperty.size() > m_EditedProperty.getMinListSize()) {
                ui::same_line();
                if (ui::draw_button(OSC_ICON_TRASH)) {
                    rv = MakeSimplePropertyElementDeleter<std::string>(idx);
                }
            }

            return rv;
        }

        property_type m_OriginalProperty{"blank", true};
        property_type m_EditedProperty{"blank", true};
    };

    // concrete property editor for a simple double value
    class DoublePropertyEditor final : public PropertyEditor<OpenSim::SimpleProperty<double>> {
    public:
        using PropertyEditor::PropertyEditor;

    private:
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() final
        {
            const property_type* maybeProp = tryGetDowncastedProperty();
            if (not maybeProp) {
                return std::nullopt;
            }
            const property_type& prop = *maybeProp;

            // update any cached data
            if (prop != m_OriginalProperty) {
                m_OriginalProperty = prop;
                m_EditedProperty = prop;
            }

            ui::draw_separator();

            // draw name of the property in left-hand column
            DrawPropertyName(m_EditedProperty);
            ui::next_column();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < max(m_EditedProperty.size(), 1); ++idx) {
                ui::push_id(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editorRv = drawIthEditor(idx);
                ui::pop_id();

                if (not rv) {
                    rv = std::move(editorRv);
                }
            }
            ui::next_column();

            return rv;
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> drawIthEditor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // draw deletion button that can delete an element from the property's list
            if (m_EditedProperty.isListProperty()) {
                if (ui::draw_button(OSC_ICON_TRASH)) {
                    rv = MakeSimplePropertyElementDeleter<double>(idx);
                }
                ui::same_line();
            }

            // read stored value from edited property
            //
            // care: optional properties have size==0, so perform a range check
            auto value = static_cast<float>(idx < m_EditedProperty.size() ? m_EditedProperty.getValue(idx) : 0.0);
            ui::set_next_item_width(ui::get_content_region_available().x);
            auto frameAnnotationLabel = "ObjectPropertiesEditor::DoubleEditor/" + m_EditedProperty.getName();
            auto drawRV = DrawCustomScalarInput("##doubleeditor", value, m_StepSize, frameAnnotationLabel);

            if (drawRV.wasEdited) {
                // update the edited property - don't rely on ImGui to remember edits
                m_EditedProperty.setValue(idx, static_cast<double>(value));
            }
            if (drawRV.shouldSave) {
                rv = MakePropertyValueSetter(idx, m_EditedProperty.getValue(idx));
            }

            return rv;
        }

        property_type m_OriginalProperty{"blank", true};
        property_type m_EditedProperty{"blank", true};
        float m_StepSize = c_InitialStepSize;
    };

    // concrete property editor for a simple `bool` value
    class BoolPropertyEditor final : public PropertyEditor<OpenSim::SimpleProperty<bool>> {
    public:
        using PropertyEditor::PropertyEditor;

    private:
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() final
        {
            const property_type* maybeProp = tryGetDowncastedProperty();
            if (not maybeProp) {
                return std::nullopt;
            }
            const property_type& prop = *maybeProp;

            // update any cached data
            if (prop != m_OriginalProperty) {
                m_OriginalProperty = prop;
                m_EditedProperty = prop;
            }

            ui::draw_separator();

            // draw name of the property in left-hand column
            DrawPropertyName(m_EditedProperty);
            ui::next_column();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < max(m_EditedProperty.size(), 1); ++idx) {
                ui::push_id(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editorRv = drawIthEditor(idx);
                ui::pop_id();

                if (not rv) {
                    rv = std::move(editorRv);
                }
            }
            ui::next_column();

            return rv;
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> drawIthEditor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // draw deletion button that can delete an element from the property's list
            if (m_EditedProperty.isListProperty()) {
                if (ui::draw_button(OSC_ICON_TRASH)) {
                    rv = MakeSimplePropertyElementDeleter<bool>(idx);
                }
                ui::same_line();
            }

            // read stored value from edited property
            //
            // care: optional properties have size==0, so perform a range check
            bool value = idx < m_EditedProperty.size() ? m_EditedProperty.getValue(idx) : false;
            bool edited = false;

            ui::set_next_item_width(ui::get_content_region_available().x);
            if (ui::draw_checkbox("##booleditor", &value)) {
                // update the edited property - don't rely on ImGui to remember edits
                m_EditedProperty.setValue(idx, value);
                edited = true;
            }

            // globally annotate the editor rect, for downstream screenshot automation
            ui::add_screenshot_annotation_to_last_drawn_item("ObjectPropertiesEditor::BoolEditor/" + m_EditedProperty.getName());

            if (edited or ui::should_save_last_drawn_item_value()) {
                rv = MakePropertyValueSetter(idx, m_EditedProperty.getValue(idx));
            }

            return rv;
        }

        property_type m_OriginalProperty{"blank", true};
        property_type m_EditedProperty{"blank", true};
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
                float propertyToEditedValueScaler_,
                SimTK::Transform propertyToEditedTransform_) :

                m_PropertyToEditedValueScaler{propertyToEditedValueScaler_},
                m_PropertyToEditedTransform{std::move(propertyToEditedTransform_)}
            {}

            Vec3 propertyValueToEditedValue(const Vec3& propertyValue) const
            {
                return to<Vec3>(static_cast<double>(m_PropertyToEditedValueScaler) * (m_PropertyToEditedTransform * to<SimTK::Vec3>(propertyValue)));
            }

            Vec3 editedValueToPropertyValue(const Vec3& editedValue) const
            {
                return to<Vec3>(m_PropertyToEditedTransform.invert() * to<SimTK::Vec3>(editedValue/m_PropertyToEditedValueScaler));
            }
        private:
            float m_PropertyToEditedValueScaler;
            SimTK::Transform m_PropertyToEditedTransform;
        };

        // returns `true` if the Vec3 property is edited in radians
        bool isPropertyEditedInRadians() const
        {
            return is_equal_case_insensitive(m_EditedProperty.getName(), "orientation");
        }

        // If the `Vec3` property has a parent frame, returns a pointer to the frame; otherwise,
        // returns a `nullptr`.
        const OpenSim::PhysicalFrame* tryGetParentFrame() const
        {
            const OpenSim::Object* const obj = tryGetObject();
            if (not obj) {
                return nullptr;  // cannot find the property's parent object?
            }

            const auto* const component = dynamic_cast<const OpenSim::Component*>(obj);
            if (not component) {
                return nullptr;  // the object isn't an OpenSim component
            }

            if (&component->getRoot() != &getRootComponent()) {
                return nullptr;  // the object is not within the tree of the root component (#800)
            }

            const auto positionPropName = TryGetPositionalPropertyName(*component);
            if (not positionPropName) {
                return nullptr;  // the component doesn't have a logical positional property that can be edited with the transform
            }

            const OpenSim::Property<SimTK::Vec3>* const prop = tryGetDowncastedProperty();
            if (not prop) {
                return nullptr;  // can't access the property this editor is ultimately editing
            }

            if (prop->getName() != *positionPropName) {
                return nullptr;  // the property this editor is editing isn't a logically positional one
            }

            return TryGetParentToGroundFrame(*component);
        }

        // if the Vec3 property has a parent frame, returns a transform that maps the Vec3
        // property's value to ground
        std::optional<SimTK::Transform> getParentToGroundTransform() const
        {
            const SimTK::State* state = tryGetState();
            if (not state) {
                return std::nullopt;
            }

            const OpenSim::PhysicalFrame* frame = tryGetParentFrame();
            if (not frame) {
                return std::nullopt;
            }

            return frame->getTransformInGround(*state);
        }

        // if the user has selected a different frame in which to edit 3D quantities, then
        // returns a transform that maps Vec3 properties expressed in ground to the other
        // frame
        std::optional<SimTK::Transform> getGroundToUserSelectedFrameTransform() const
        {
            if (not m_MaybeUserSelectedFrameAbsPath) {
                return std::nullopt;
            }

            const SimTK::State* state = tryGetState();
            if (not state) {
                return std::nullopt;
            }

            const auto* frame = FindComponent<OpenSim::Frame>(getRootComponent(), *m_MaybeUserSelectedFrameAbsPath);
            if (not frame) {
                return std::nullopt;
            }

            return frame->getTransformInGround(*state).invert();
        }

        ValueConverter getValueConverter() const
        {
            float conversionCoefficient = 1.0f;
            if (isPropertyEditedInRadians() and not m_OrientationValsAreInRadians) {
                conversionCoefficient = static_cast<float>(SimTK_RADIAN_TO_DEGREE);
            }

            const std::optional<SimTK::Transform> parent2ground = getParentToGroundTransform();
            const std::optional<SimTK::Transform> ground2frame = getGroundToUserSelectedFrameTransform();
            const SimTK::Transform transform = parent2ground and ground2frame ?
                (*ground2frame) * (*parent2ground) :
                SimTK::Transform{};

            return ValueConverter{conversionCoefficient, transform};
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() final
        {
            const property_type* maybeProp = tryGetDowncastedProperty();
            if (not maybeProp)
            {
                return std::nullopt;
            }
            const property_type& prop = *maybeProp;

            // update any cached data
            if (prop != m_OriginalProperty) {
                m_OriginalProperty = prop;
                m_EditedProperty = prop;
            }

            // compute value converter (applies to all values)
            const ValueConverter valueConverter = getValueConverter();


            // draw UI


            ui::draw_separator();

            // draw name of the property in left-hand column
            DrawPropertyName(m_EditedProperty);
            ui::next_column();

            // top line of right column shows "reexpress in" editor (if applicable)
            drawReexpressionEditorIfApplicable();

            // draw radians/degrees conversion toggle button (if applicable)
            drawDegreesToRadiansConversionToggle();

            // draw `[0, 1]` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < max(m_EditedProperty.size(), 1); ++idx) {
                ui::push_id(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editorRv = drawIthEditor(valueConverter, idx);
                ui::pop_id();

                if (not rv) {
                    rv = std::move(editorRv);
                }
            }
            ui::next_column();

            return rv;
        }

        void drawReexpressionEditorIfApplicable()
        {
            const OpenSim::PhysicalFrame* parentFrame = tryGetParentFrame();
            if (not parentFrame) {
                return;
            }

            const auto& defaultedLabel = parentFrame->getName();
            const std::string preview = m_MaybeUserSelectedFrameAbsPath ?
                m_MaybeUserSelectedFrameAbsPath->getComponentName() :
                std::string{defaultedLabel};

            ui::set_next_item_width(ui::get_content_region_available().x - ui::calc_text_size("(?)").x);
            if (ui::begin_combobox("##reexpressioneditor", preview)) {
                int imguiID = 0;

                // draw "default" (reset) option
                {
                    ui::draw_separator();
                    ui::push_id(imguiID++);
                    bool selected = !m_MaybeUserSelectedFrameAbsPath.has_value();
                    if (ui::draw_selectable(defaultedLabel, &selected)) {
                        m_MaybeUserSelectedFrameAbsPath.reset();
                    }
                    ui::pop_id();
                    ui::draw_separator();
                }

                // draw selectable for each frame in the component tree
                for (const OpenSim::Frame& frame : getRootComponent().getComponentList<OpenSim::Frame>()) {
                    const OpenSim::ComponentPath frameAbsPath = GetAbsolutePath(frame);

                    ui::push_id(imguiID++);
                    bool selected = frameAbsPath == m_MaybeUserSelectedFrameAbsPath;
                    if (ui::draw_selectable(frame.getName(), &selected)) {
                        m_MaybeUserSelectedFrameAbsPath = frameAbsPath;
                    }
                    ui::pop_id();
                }

                ui::end_combobox();
            }
            ui::same_line();

            ui::draw_help_marker("Expression Frame", "The coordinate frame in which this quantity is edited.\n\nNote: Changing this only affects the coordinate space the the value is edited in. It does not change the frame that the component is attached to. You can change the frame attachment by using the component's context menu: Socket > $FRAME > (edit button) > (select new frame)");
        }

        // draws an editor for the `ith` Vec3 element of the given (potentially, list) property
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> drawIthEditor(
            const ValueConverter& valueConverter,
            int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // draw deletion button that can delete an element from the property's list
            if (m_EditedProperty.isListProperty()) {
                if (ui::draw_button(OSC_ICON_TRASH)) {
                    rv = MakeSimplePropertyElementDeleter<SimTK::Vec3>(idx);
                }
                ui::same_line();
            }

            // read stored value from edited property
            //
            // care: optional properties have size==0, so perform a range check
            const Vec3 rawValue = to<Vec3>(idx < m_EditedProperty.size() ? m_EditedProperty.getValue(idx) : SimTK::Vec3{0.0});
            const Vec3 editedValue = valueConverter.propertyValueToEditedValue(rawValue);

            // draw an editor for each component of the Vec3
            bool shouldSave = false;
            for (Vec3::size_type i = 0; i < 3; ++i) {
                const ComponentEditorReturn componentEditorRv = drawVec3ComponentEditor(idx, i, editedValue, valueConverter);
                shouldSave = shouldSave or componentEditorRv == ComponentEditorReturn::ShouldSave;
            }

            // if any component editor indicated that it should be saved then propagate that upwards
            if (shouldSave) {
                rv = MakePropertyValueSetter(idx, m_EditedProperty.getValue(idx));
            }

            return rv;
        }

        enum class ComponentEditorReturn { None, ShouldSave };

        // draws float input for a single component of the Vec3 (e.g. vec.x)
        ComponentEditorReturn drawVec3ComponentEditor(
            int idx,
            Vec3::size_type i,
            Vec3 editedValue,
            const ValueConverter& valueConverter)
        {
            ui::push_id(i);
            // draw dimension hint (color bar next to the input)
            DrawColoredDimensionHintVerticalLine(Color(0.0f, 0.6f).with_element(i, 1.0f));

            // draw the input editor
            ui::set_next_item_width(ui::get_content_region_available().x);
            auto frameAnnotation = GenerateVecFrameAnnotationLabel(m_EditedProperty, i);
            auto drawRV = DrawCustomScalarInput("##valueinput", editedValue[i], m_StepSize, frameAnnotation);

            if (drawRV.wasEdited) {
                // un-convert the value on save
                const Vec3 savedValue = valueConverter.editedValueToPropertyValue(editedValue);
                m_EditedProperty.setValue(idx, to<SimTK::Vec3>(savedValue));
            }

            ui::pop_id();

            return drawRV.shouldSave ? ComponentEditorReturn::ShouldSave : ComponentEditorReturn::None;
        }

        // draws button that lets the user toggle between inputting radians vs. degrees
        void drawDegreesToRadiansConversionToggle()
        {
            if (not isPropertyEditedInRadians()) {
                return;
            }

            if (m_OrientationValsAreInRadians) {
                if (ui::draw_button("radians")) {
                    m_OrientationValsAreInRadians = !m_OrientationValsAreInRadians;
                }
                ui::add_screenshot_annotation_to_last_drawn_item("ObjectPropertiesEditor::OrientationToggle/" + m_EditedProperty.getName());
                ui::draw_tooltip_body_only_if_item_hovered("This quantity is edited in radians (click to switch to degrees)");
            }
            else {
                if (ui::draw_button("degrees")) {
                    m_OrientationValsAreInRadians = !m_OrientationValsAreInRadians;
                }
                ui::add_screenshot_annotation_to_last_drawn_item("ObjectPropertiesEditor::OrientationToggle/" + m_EditedProperty.getName());
                ui::draw_tooltip_body_only_if_item_hovered("This quantity is edited in degrees (click to switch to radians)");
            }
        }

        property_type m_OriginalProperty{"blank", true};
        property_type m_EditedProperty{"blank", true};
        std::optional<OpenSim::ComponentPath> m_MaybeUserSelectedFrameAbsPath;
        float m_StepSize = c_InitialStepSize;
        bool m_OrientationValsAreInRadians = false;
    };

    // concrete property editor for a simple `Vec6` value
    class Vec6PropertyEditor final : public PropertyEditor<OpenSim::SimpleProperty<SimTK::Vec6>> {
    public:
        using PropertyEditor::PropertyEditor;

    private:
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() final
        {
            const property_type* maybeProp = tryGetDowncastedProperty();
            if (not maybeProp) {
                return std::nullopt;
            }
            const property_type& prop = *maybeProp;

            // update any cached data
            if (prop != m_OriginalProperty) {
                m_OriginalProperty = prop;
                m_EditedProperty = prop;
            }

            ui::draw_separator();

            // draw name of the property in left-hand column
            DrawPropertyName(m_EditedProperty);
            ui::next_column();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < max(m_EditedProperty.size(), 1); ++idx) {
                ui::push_id(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editorRv = drawIthEditor(idx);
                ui::pop_id();

                if (not rv) {
                    rv = std::move(editorRv);
                }
            }
            ui::next_column();

            return rv;
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> drawIthEditor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // draw deletion button that can delete an element from the property's list
            if (m_EditedProperty.isListProperty()) {
                if (ui::draw_button(OSC_ICON_TRASH)) {
                    rv = MakeSimplePropertyElementDeleter<SimTK::Vec6>(idx);
                }
            }

            // read latest raw value as-stored in edited property
            //
            // care: `getValue` can return `nullptr` if the property is optional (size == 0)
            std::array<float, 6> rawValue = idx < m_EditedProperty.size() ?
                to<std::array<float, 6>>(m_EditedProperty.getValue(idx)) :
                std::array<float, 6>{};

            bool shouldSave = false;
            for (int i = 0; i < 2; ++i) {
                ui::push_id(i);

                ui::set_next_item_width(ui::get_content_region_available().x);
                if (ui::draw_float3_input("##vec6editor", rawValue.data() + static_cast<ptrdiff_t>(3*i), "%.6f")) {
                    m_EditedProperty.updValue(idx)[3*i + 0] = static_cast<double>(rawValue[3*i + 0]);
                    m_EditedProperty.updValue(idx)[3*i + 1] = static_cast<double>(rawValue[3*i + 1]);
                    m_EditedProperty.updValue(idx)[3*i + 2] = static_cast<double>(rawValue[3*i + 2]);
                }
                shouldSave = shouldSave or ui::should_save_last_drawn_item_value();
                ui::add_screenshot_annotation_to_last_drawn_item("ObjectPropertiesEditor::Vec6Editor/" + m_EditedProperty.getName());

                ui::pop_id();
            }

            if (shouldSave) {
                rv = MakePropertyValueSetter(idx, m_EditedProperty.getValue(idx));
            }

            return rv;
        }

        property_type m_OriginalProperty{"blank", true};
        property_type m_EditedProperty{"blank", true};
    };

    class IntPropertyEditor final : public PropertyEditor<OpenSim::SimpleProperty<int>> {
    public:
        using PropertyEditor::PropertyEditor;

    private:
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() final
        {
            const property_type* maybeProp = tryGetDowncastedProperty();
            if (not maybeProp) {
                return std::nullopt;
            }
            const property_type& prop = *maybeProp;

            // update any cached data
            if (prop != m_OriginalProperty) {
                m_OriginalProperty = prop;
                m_EditedProperty = prop;
            }

            ui::draw_separator();

            // draw name of the property in left-hand column
            DrawPropertyName(m_EditedProperty);
            ui::next_column();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < max(m_EditedProperty.size(), 1); ++idx) {
                ui::push_id(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editorRv = drawIthEditor(idx);
                ui::pop_id();

                if (not rv) {
                    rv = std::move(editorRv);
                }
            }
            ui::next_column();

            return rv;
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> drawIthEditor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // draw deletion button that can delete an element from the property's list
            if (m_EditedProperty.isListProperty()) {
                if (ui::draw_button(OSC_ICON_TRASH)) {
                    rv = MakeSimplePropertyElementDeleter<int>(idx);
                }
                ui::same_line();
            }

            // read stored value from edited property
            //
            // care: optional properties have size==0, so perform a range check
            int value = idx < m_EditedProperty.size() ? m_EditedProperty.getValue(idx) : 0;
            bool edited = false;

            ui::set_next_item_width(ui::get_content_region_available().x);
            if (ui::draw_int_input("##inteditor", &value)) {
                // update the edited property - don't rely on ImGui to remember edits
                m_EditedProperty.setValue(idx, value);
                edited = true;
            }

            // globally annotate the editor rect, for downstream screenshot automation
            ui::add_screenshot_annotation_to_last_drawn_item("ObjectPropertiesEditor::IntEditor/");

            if (edited or ui::should_save_last_drawn_item_value()) {
                rv = MakePropertyValueSetter(idx, m_EditedProperty.getValue(idx));
            }

            return rv;
        }

        property_type m_OriginalProperty{"blank", true};
        property_type m_EditedProperty{"blank", true};
    };
}

// concrete property editors for object types
namespace
{
    // concrete property editor for an OpenSim::Appearance
    class AppearancePropertyEditor final : public PropertyEditor<OpenSim::ObjectProperty<OpenSim::Appearance>> {
    public:
        using PropertyEditor::PropertyEditor;

    private:
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() final
        {
            const property_type* maybeProp = tryGetDowncastedProperty();
            if (not maybeProp) {
                return std::nullopt;
            }
            const property_type& prop = *maybeProp;

            // update any cached data
            if (prop != m_OriginalProperty) {
                m_OriginalProperty = prop;
                m_EditedProperty = prop;
            }

            ui::draw_separator();

            // draw name of the property in left-hand column
            DrawPropertyName(m_EditedProperty);
            ui::next_column();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < max(m_EditedProperty.size(), 1); ++idx) {
                ui::push_id(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editorRv = drawIthEditor(idx);
                ui::pop_id();

                if (not rv) {
                    rv = std::move(editorRv);
                }
            }
            ui::next_column();

            return rv;
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> drawIthEditor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            if (m_EditedProperty.isListProperty()) {
                return rv;  // HACK: ignore list props for now
            }

            if (m_EditedProperty.empty()) {
                return rv;  // HACK: ignore optional props for now
            }

            bool shouldSave = false;

            Color color = to_color(m_EditedProperty.getValue());
            ui::set_next_item_width(ui::get_content_region_available().x);

            if (ui::draw_rgba_color_editor("##coloreditor", color)) {
                SimTK::Vec3 newColor;
                newColor[0] = static_cast<double>(color[0]);
                newColor[1] = static_cast<double>(color[1]);
                newColor[2] = static_cast<double>(color[2]);

                m_EditedProperty.updValue().set_color(newColor);
                m_EditedProperty.updValue().set_opacity(static_cast<double>(color[3]));
            }
            shouldSave = shouldSave or ui::should_save_last_drawn_item_value();

            bool isVisible = m_EditedProperty.getValue().get_visible();
            if (ui::draw_checkbox("is visible", &isVisible)) {
                m_EditedProperty.updValue().set_visible(isVisible);
            }
            shouldSave = shouldSave or ui::should_save_last_drawn_item_value();

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
                size_t index = clamp(static_cast<size_t>(m_EditedProperty.getValue().get_representation())+1, 0uz, options.size());
                ui::set_next_item_width(ui::get_content_region_available().x);
                if (ui::draw_combobox("##DisplayPref", &index, options)) {
                    m_EditedProperty.updValue().set_representation(static_cast<OpenSim::VisualRepresentation>(static_cast<int>(index)-1));
                    shouldSave = true;
                }
            }

            if (shouldSave) {
                rv = MakePropertyValueSetter(idx, m_EditedProperty.getValue(idx));
            }

            return rv;
        }

        property_type m_OriginalProperty{"blank", true};
        property_type m_EditedProperty{"blank", true};
    };

    // concrete property editor for an `OpenSim::HuntCrossleyForce::ContactParametersSet`
    class ContactParameterSetEditor final : public PropertyEditor<OpenSim::ObjectProperty<OpenSim::HuntCrossleyForce::ContactParametersSet>> {
    public:
        using PropertyEditor::PropertyEditor;

    private:
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() final
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            const property_type* maybeProp = tryGetDowncastedProperty();
            if (not maybeProp) {
                return std::nullopt;  // cannot find property
            }
            const property_type& prop = *maybeProp;

            if (empty(prop.getValue())) {
                return std::nullopt;  // no editable contact set on the property
            }

            const OpenSim::HuntCrossleyForce::ContactParameters& params = prop.getValue()[0];

            // update cached editors, if necessary
            if (not m_MaybeNestedEditor) {
                if (const auto componentPtr = tryGetComponentSharedPtr()) {
                    m_MaybeNestedEditor.emplace(tryGetParentWidget(), componentPtr, [&params]() { return &params; });
                }
            }
            ObjectPropertiesEditor& nestedEditor = *m_MaybeNestedEditor;

            ui::set_num_columns();
            auto resp = nestedEditor.onDraw();
            ui::set_num_columns(2);

            if (resp) {
                // careful here: the response has a correct updater but doesn't know the full
                // path to the housing component, so we have to wrap the updater with
                // appropriate lookups etc

                rv = [=](OpenSim::AbstractProperty& p) mutable
                {
                    auto* downcasted = dynamic_cast<OpenSim::Property<OpenSim::HuntCrossleyForce::ContactParametersSet>*>(&p);
                    if (downcasted and not empty(downcasted->getValue())) {
                        OpenSim::HuntCrossleyForce::ContactParameters& contactParams = At(downcasted->updValue(), 0);
                        if (params.hasProperty(resp->getPropertyName())) {
                            OpenSim::AbstractProperty& childP = contactParams.updPropertyByName(resp->getPropertyName());
                            resp->apply(childP);
                        }
                    }
                };
            }

            return rv;
        }

        std::optional<ObjectPropertiesEditor> m_MaybeNestedEditor;
    };

    // concrete property editor for an OpenSim::GeometryPath
    class AbstractGeometryPathPropertyEditor final :
        public PropertyEditor<OpenSim::ObjectProperty<OpenSim::AbstractGeometryPath>> {
    public:
        using PropertyEditor::PropertyEditor;

    private:
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() final
        {
            const property_type* maybeProp = tryGetDowncastedProperty();
            if (not maybeProp) {
                return std::nullopt;
            }
            const property_type& prop = *maybeProp;

            ui::draw_separator();
            DrawPropertyName(prop);
            ui::next_column();
            {
                Widget* parentWidget = tryGetParentWidget();
                const auto componentPtr = tryGetComponentSharedPtr();
                if (parentWidget and componentPtr) {
                    if (ui::draw_button(OSC_ICON_EDIT " edit ")) {
                        App::post_event<OpenPopupEvent>(*parentWidget, createGeometryPathEditorPopup(componentPtr));
                    }
                }
                else {
                    ui::align_text_to_frame_padding();  // ensure it aligns with the property name in the previous column
                    ui::draw_text(prop.toString());
                }
            }
            ui::next_column();

            if (*m_ReturnValueHolder) {
                std::optional<ObjectPropertyEdit> edit;
                std::swap(*m_ReturnValueHolder, edit);
                return edit->getUpdater();
            }
            else {
                return std::nullopt;
            }
        }

        std::unique_ptr<Popup> createGeometryPathEditorPopup(const std::shared_ptr<const IComponentAccessor>& componentPtr)
        {
            const auto accessor = getDowncastedPropertyAccessor();
            return std::make_unique<GeometryPathEditorPopup>(
                tryGetParentWidget(),
                "Edit Geometry Path",
                componentPtr,
                [accessor]() -> const OpenSim::GeometryPath*
                {
                    const property_type* p = accessor();
                    if (not p or p->isListProperty()) {
                        return nullptr;
                    }
                    return dynamic_cast<const OpenSim::GeometryPath*>(&p->getValueAsObject());
                },
                [shared = m_ReturnValueHolder, accessor](const OpenSim::GeometryPath& gp) mutable
                {
                    if (const property_type* prop = accessor()) {
                        *shared = ObjectPropertyEdit{*prop, MakePropertyValueSetter<const OpenSim::GeometryPath&, OpenSim::AbstractGeometryPath>(0, gp)};
                    }
                }
            );
        }

        // shared between this property editor and a popup it may have spawned
        std::shared_ptr<std::optional<ObjectPropertyEdit>> m_ReturnValueHolder = std::make_shared<std::optional<ObjectPropertyEdit>>();
    };

    struct FunctionPropertyEditorTraits {
        static bool IsCompatibleWith(const OpenSim::AbstractProperty* prop)
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

        static const OpenSim::ObjectProperty<OpenSim::Function>* TryGetDowncasted(const OpenSim::AbstractProperty*)
        {
            // there's no safe way to upcast an `OpenSim::Property<SpecializedFunction>` --> `OpenSim::Property<Function>`
            //
            // this trait implementation's job is just to ensure the `PropertyEditor` for a function is selected
            // so that we can read data out of it generically (rather than actually edit it)
            return nullptr;
        }
    };

    class FunctionPropertyEditor final :
        public PropertyEditor<OpenSim::ObjectProperty<OpenSim::Function>, FunctionPropertyEditorTraits> {
    public:
        using PropertyEditor::PropertyEditor;

    private:
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() final
        {
            const OpenSim::AbstractProperty* prop = tryGetProperty();
            if (not prop) {
                return std::nullopt;
            }

            ui::draw_separator();
            DrawPropertyName(*prop);

            ui::next_column();
            {
                Widget* parentWidget = tryGetParentWidget();
                const auto componentPtr = tryGetComponentSharedPtr();
                if (parentWidget and componentPtr) {
                    if (ui::draw_button(OSC_ICON_EYE " view ")) {

                        // care: the accessor here differs from the default because the user's selection
                        // can change the accessor's behavior. This is a panel, so it should stick to
                        // whatever was selected when the panel was spawned.
                        auto panel = std::make_unique<FunctionCurveViewerPanel>(
                            parentWidget,
                            generatePopupName(*prop),
                            componentPtr,
                            [component = componentPtr, parentPath = tryGetObjectAbsPath(), propname = prop->getName()]() -> const OpenSim::Function*
                            {
                                auto* parentComponent = FindComponent(*component, parentPath);
                                if (not parentComponent) {
                                    return nullptr;
                                }
                                if (not parentComponent->hasProperty(propname)) {
                                    return nullptr;
                                }
                                auto& prop = parentComponent->getPropertyByName(propname);
                                if (prop.empty()) {
                                    return nullptr;
                                }
                                if (not prop.isObjectProperty()) {
                                    return nullptr;
                                }
                                if (prop.empty()) {
                                    return nullptr;
                                }
                                return dynamic_cast<const OpenSim::Function*>(&prop.getValueAsObject(0));
                            }
                        );
                        App::post_event<OpenPanelEvent>(*parentWidget, std::move(panel));
                    }
                }
                else {
                    ui::align_text_to_frame_padding();  // ensure it aligns with the property name in the previous column
                    ui::draw_text(prop->toString());
                }
            }

            ui::draw_tooltip_if_item_hovered("View Function", OSC_ICON_MAGIC " Experimental Feature " OSC_ICON_MAGIC ": currently, plots the `OpenSim::Function`, but it doesn't know what the X or Y axes are, or what values might be reasonable for either. It also doesn't spawn a non-modal panel, which would be handy if you wanted to view multiple functions at the same time - I should work on that ;)");
            ui::same_line();
            ui::align_text_to_frame_padding();  // ensure it aligns with the property name in the previous column
            ui::draw_text(prop->getTypeName());
            ui::next_column();

            return std::nullopt;
        }

        std::string generatePopupName(const OpenSim::AbstractProperty& prop) const
        {
            std::stringstream ss;
            ss << "View ";
            if (const OpenSim::Object* obj = tryGetObject()) {
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
        FunctionPropertyEditor
    >;

    // a type-erased entry in the runtime registry LUT
    class PropertyEditorRegistryEntry final {
    public:
        // a function that tests whether a property editor in the registry is suitable
        // for the given abstract property
        using PropertyEditorTester = bool(*)(const OpenSim::AbstractProperty&);

        // a function that can be used to construct a pointer to a virtual property editor
        using PropertyEditorCtor = std::unique_ptr<IPropertyEditor>(*)(PropertyEditorArgs);

        // create a type-erased entry from a known, concrete, editor
        template<std::derived_from<IPropertyEditor> ConcretePropertyEditor>
        static constexpr PropertyEditorRegistryEntry make_entry()
        {
            const auto testerFn = ConcretePropertyEditor::IsCompatibleWith;
            const auto typeErasedCtorFn = [](PropertyEditorArgs args) -> std::unique_ptr<IPropertyEditor>
            {
                return std::make_unique<ConcretePropertyEditor>(std::move(args));
            };

            return PropertyEditorRegistryEntry{testerFn, typeErasedCtorFn};
        }

        bool isCompatibleWith(const OpenSim::AbstractProperty& abstractProp) const
        {
            return m_Tester(abstractProp);
        }

        std::unique_ptr<IPropertyEditor> construct(PropertyEditorArgs args) const
        {
            return m_Ctor(std::move(args));
        }
    private:
        constexpr PropertyEditorRegistryEntry(
            PropertyEditorTester tester_,
            PropertyEditorCtor ctor_) :

            m_Tester{tester_},
            m_Ctor{ctor_}
        {}

        PropertyEditorTester m_Tester;
        PropertyEditorCtor m_Ctor;
    };

    // runtime type-erased registry for all property editors
    class PropertyEditorRegistry final {
    public:
        std::unique_ptr<IPropertyEditor> tryCreateEditor(PropertyEditorArgs args) const
        {
            const OpenSim::AbstractProperty* prop = args.propertyAccessor();
            if (not prop) {
                return nullptr;  // cannot access the property
            }

            const auto it = rgs::find_if(
                m_Entries,
                [&prop](const auto& entry) { return entry.isCompatibleWith(*prop); }
            );
            if (it == m_Entries.end()) {
                return nullptr;  // no property editor registered for the given typeid
            }

            return it->construct(std::move(args));
        }
    private:
        using storage_type = std::array<PropertyEditorRegistryEntry, TypelistSizeV<PropertyEditors>>;

        storage_type m_Entries = []<class... ConcretePropertyEditors>(Typelist<ConcretePropertyEditors...>) {
            return std::to_array({PropertyEditorRegistryEntry::make_entry<ConcretePropertyEditors>()...});
        }(PropertyEditors{});
    };

    constexpr PropertyEditorRegistry c_Registry;
}

// top-level implementation of the properties editor
class osc::ObjectPropertiesEditor::Impl final : public WidgetPrivate {
public:
    explicit Impl(
        Widget& owner,
        Widget* parent,
        std::shared_ptr<const IVersionedComponentAccessor> targetComponent,
        std::function<const OpenSim::Object*()> objectGetter) :

        WidgetPrivate{owner, parent},
        m_TargetComponent{std::move(targetComponent)},
        m_ObjectGetter{std::move(objectGetter)}
    {}

    std::optional<ObjectPropertyEdit> onDraw()
    {
        const bool disabled = m_TargetComponent->isReadonly();
        if (disabled) {
            ui::begin_disabled();
        }

        std::optional<ObjectPropertyEdit> rv;
        if (const OpenSim::Object* maybeObj = m_ObjectGetter()) {
            rv = drawPropertyEditors(*maybeObj);  // object accessible: draw property editors
        }

        if (disabled) {
            ui::end_disabled();
        }

        return rv;
    }

    void insertInBlacklist(std::string_view propertyName)
    {
        m_Blacklist.insert(std::string{propertyName});
    }
private:

    // draws all property editors for the given object
    std::optional<ObjectPropertyEdit> drawPropertyEditors(
        const OpenSim::Object& obj)
    {
        if (m_PreviousObject != &obj) {
            // the object has changed since the last draw call, so
            // reset all property editor state
            m_PropertyEditorsByName.clear();
            m_PreviousObject = &obj;
        }

        // draw each editor and return the last property edit (or std::nullopt)
        std::optional<ObjectPropertyEdit> rv;

        ui::set_num_columns(2);
        for (int i = 0; i < obj.getNumProperties(); ++i) {
            ui::push_id(i);
            std::optional<ObjectPropertyEdit> maybeEdit = tryDrawPropertyEditor(obj, obj.getPropertyByIndex(i));
            ui::pop_id();

            if (maybeEdit) {
                rv = std::move(maybeEdit);
            }
        }
        ui::set_num_columns();

        return rv;
    }

    // tries to draw one property editor for one property of an object
    std::optional<ObjectPropertyEdit> tryDrawPropertyEditor(
        const OpenSim::Object& obj,
        const OpenSim::AbstractProperty& prop)
    {
        if (prop.getName().starts_with("socket_")) {
            // #542: ignore properties that begin with `socket_`, because
            // they are proxy properties to the object's sockets and should
            // be manipulated via socket, rather than property, editors
            return std::nullopt;
        }
        if (m_Blacklist.contains(prop.getName())) {
            return std::nullopt;
        }
        else if (IPropertyEditor* maybeEditor = tryGetPropertyEditor(prop)) {
            return drawPropertyEditor(obj, prop, *maybeEditor);
        }
        else {
            drawNonEditablePropertyDetails(prop);
            return std::nullopt;
        }
    }

    // draws a property editor for the given object+property
    std::optional<ObjectPropertyEdit> drawPropertyEditor(
        const OpenSim::Object& obj,
        const OpenSim::AbstractProperty& prop,
        IPropertyEditor& editor)
    {
        ui::push_id(prop.getName());
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> maybeUpdater = editor.onDraw();
        ui::pop_id();

        if (maybeUpdater) {
            return ObjectPropertyEdit{obj, prop, std::move(maybeUpdater).value()};
        }
        else {
            return std::nullopt;
        }
    }

    // draws a non-editable representation of a property
    void drawNonEditablePropertyDetails(
        const OpenSim::AbstractProperty& prop)
    {
        ui::draw_separator();
        DrawPropertyName(prop);
        ui::next_column();
        ui::align_text_to_frame_padding();  // ensure it aligns with the property name in the previous column
        ui::draw_text(prop.toString());
        ui::next_column();
    }

    // try get/construct a property editor for the given property
    IPropertyEditor* tryGetPropertyEditor(const OpenSim::AbstractProperty& prop)
    {
        const auto [it, inserted] = m_PropertyEditorsByName.try_emplace(prop.getName(), nullptr);

        if (inserted or (it->second and not it->second->isCompatibleWith(prop))) {
            // need to create a new editor because either it hasn't been made yet or the existing
            // editor is for a different type
            auto accessor = MakePropertyAccessor(m_ObjectGetter, prop.getName());
            it->second = c_Registry.tryCreateEditor({
                .parent = parent(),
                .component = m_TargetComponent,
                .objectAccessor = m_ObjectGetter,
                .propertyAccessor = std::move(accessor),
            });
        }

        return it->second.get();
    }

    std::shared_ptr<const IVersionedComponentAccessor> m_TargetComponent;
    std::function<const OpenSim::Object*()> m_ObjectGetter;
    std::unordered_set<std::string> m_Blacklist;
    const OpenSim::Object* m_PreviousObject = nullptr;
    std::unordered_map<std::string, std::unique_ptr<IPropertyEditor>> m_PropertyEditorsByName;
};


osc::ObjectPropertiesEditor::ObjectPropertiesEditor(
    Widget* parent,
    std::shared_ptr<const IVersionedComponentAccessor> targetComponent,
    std::function<const OpenSim::Object*()> objectGetter) :

    Widget{std::make_unique<Impl>(*this, parent, std::move(targetComponent), std::move(objectGetter))}
{}
void osc::ObjectPropertiesEditor::insertInBlacklist(std::string_view propertyName) { private_data().insertInBlacklist(propertyName); }
std::optional<ObjectPropertyEdit> osc::ObjectPropertiesEditor::onDraw() { return private_data().onDraw(); }
void osc::ObjectPropertiesEditor::impl_on_draw() { private_data().onDraw(); }
