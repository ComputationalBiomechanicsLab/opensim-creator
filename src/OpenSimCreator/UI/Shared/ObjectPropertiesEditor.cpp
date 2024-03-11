#include "ObjectPropertiesEditor.h"

#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/UI/IPopupAPI.h>
#include <OpenSimCreator/UI/Shared/GeometryPathEditorPopup.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>
#include <OpenSimCreator/Utils/SimTKHelpers.h>

#include <IconsFontAwesome5.h>
#include <OpenSim/Common/AbstractProperty.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/Object.h>
#include <OpenSim/Common/Property.h>
#include <OpenSim/Simulation/Model/AbstractGeometryPath.h>
#include <OpenSim/Simulation/Model/Appearance.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/GeometryPath.h>
#include <OpenSim/Simulation/Model/HuntCrossleyForce.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/Log.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/StringHelpers.h>
#include <oscar/Utils/Typelist.h>
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
#include <utility>

using namespace osc;

// constants
namespace
{
    inline constexpr float c_InitialStepSize = 0.001f;  // effectively, 1 mm or 0.001 rad
}

// helper functions
namespace
{
    // unpack a SimTK::Vec6 into an array
    std::array<float, 6> ToArray(SimTK::Vec6 const& v)
    {
        return
        {
            static_cast<float>(v[0]),
            static_cast<float>(v[1]),
            static_cast<float>(v[2]),
            static_cast<float>(v[3]),
            static_cast<float>(v[4]),
            static_cast<float>(v[5]),
        };
    }

    // returns an updater function that deletes an element from a list property
    template<typename T>
    std::function<void(OpenSim::AbstractProperty&)> MakePropElementDeleter(int idx)
    {
        return [idx](OpenSim::AbstractProperty& p)
        {
            auto* const ps = dynamic_cast<OpenSim::SimpleProperty<T>*>(&p);
            if (!ps)
            {
                return;  // types don't match: caller probably mismatched properties
            }

            auto copy = std::make_unique<OpenSim::SimpleProperty<T>>(ps->getName(), ps->isOneValueProperty());
            for (int i = 0; i < ps->size(); ++i)
            {
                if (i != idx)
                {
                    copy->appendValue(ps->getValue(i));
                }
            }

            ps->clear();
            ps->assign(*copy);
        };
    }

    // returns an updater function that sets the value of a property
    template<
        typename TValue,
        typename TProperty = std::remove_cvref_t<TValue>
    >
    std::function<void(OpenSim::AbstractProperty&)> MakePropValueSetter(int idx, TValue&& value)
    {
        return [idx, val = std::forward<TValue>(value)](OpenSim::AbstractProperty& p)
        {
            auto* const ps = dynamic_cast<OpenSim::Property<TProperty>*>(&p);
            if (!ps)
            {
                return;  // types don't match: caller probably mismatched properties
            }
            ps->setValue(idx, val);
        };
    }

    // draws the property name and (optionally) comment tooltip
    void DrawPropertyName(OpenSim::AbstractProperty const& prop)
    {
        ui::TextUnformatted(prop.getName());

        if (!prop.getComment().empty())
        {
            ui::SameLine();
            ui::DrawHelpMarker(prop.getComment());
        }
    }

    // wraps an object accessor with property information so that an individual
    // property accesssor with the same lifetime semantics as the object can exist
    std::function<OpenSim::AbstractProperty const*()> MakePropertyAccessor(
        std::function<OpenSim::Object const*()> const& objAccessor,
        std::string const& propertyName)
    {
        return [objAccessor, propertyName]() -> OpenSim::AbstractProperty const*
        {
            OpenSim::Object const* maybeObj = objAccessor();
            if (!maybeObj)
            {
                return nullptr;
            }
            OpenSim::Object const& obj = *maybeObj;

            if (!obj.hasProperty(propertyName))
            {
                return nullptr;
            }
            return &obj.getPropertyByName(propertyName);
        };
    }

    // returns a suitable color for the given dimension index (e.g. x == 0)
    Color IthDimensionColor(Vec3::size_type i)
    {
        Color color = {0.0f, 0.0f, 0.0f, 0.6f};
        color[i] = 1.0f;
        return color;
    }

    // draws a little vertical line, which is usually used to visually indicate
    // x/y/z to the user
    void DrawColoredDimensionHintVerticalLine(Color const& color)
    {
        ImDrawList* const l = ui::GetWindowDrawList();
        Vec2 const p = ui::GetCursorScreenPos();
        float const h = ui::GetTextLineHeight() + 2.0f*ui::GetStyle().FramePadding.y + 2.0f*ui::GetStyle().FrameBorderSize;
        Vec2 const dims = Vec2{4.0f, h};
        l->AddRectFilled(p, p + dims, ui::ToImU32(color));
        ui::SetCursorScreenPos({p.x + 4.0f, p.y});
    }

    // draws a context menu that the user can use to change the step interval of the +/- buttons
    void DrawStepSizeEditor(float& stepSize)
    {
        if (ui::BeginPopupContextItem("##valuecontextmenu"))
        {
            ui::Text("Set Step Size");
            ui::SameLine();
            ui::DrawHelpMarker("Sets the decrement/increment of the + and - buttons. Can be handy for tweaking property values");
            ui::Dummy({0.0f, 0.1f*ui::GetTextLineHeight()});
            ui::Separator();
            ui::Dummy({0.0f, 0.2f*ui::GetTextLineHeight()});

            if (ui::BeginTable("CommonChoicesTable", 2, ImGuiTableFlags_SizingStretchProp))
            {
                ui::TableSetupColumn("Type");
                ui::TableSetupColumn("Options");

                ui::TableNextRow();
                ui::TableSetColumnIndex(0);
                ui::Text("Custom");
                ui::TableSetColumnIndex(1);
                ui::InputFloat("##stepsizeinput", &stepSize, 0.0f, 0.0f, "%.6f");

                ui::TableNextRow();
                ui::TableSetColumnIndex(0);
                ui::Text("Lengths");
                ui::TableSetColumnIndex(1);
                if (ui::Button("10 cm"))
                {
                    stepSize = 0.1f;
                }
                ui::SameLine();
                if (ui::Button("1 cm"))
                {
                    stepSize = 0.01f;
                }
                ui::SameLine();
                if (ui::Button("1 mm"))
                {
                    stepSize = 0.001f;
                }
                ui::SameLine();
                if (ui::Button("0.1 mm"))
                {
                    stepSize = 0.0001f;
                }

                ui::TableNextRow();
                ui::TableSetColumnIndex(0);
                ui::Text("Angles (Degrees)");
                ui::TableSetColumnIndex(1);
                if (ui::Button("180"))
                {
                    stepSize = 180.0f;
                }
                ui::SameLine();
                if (ui::Button("90"))
                {
                    stepSize = 90.0f;
                }
                ui::SameLine();
                if (ui::Button("45"))
                {
                    stepSize = 45.0f;
                }
                ui::SameLine();
                if (ui::Button("10"))
                {
                    stepSize = 10.0f;
                }
                ui::SameLine();
                if (ui::Button("1"))
                {
                    stepSize = 1.0f;
                }

                ui::TableNextRow();
                ui::TableSetColumnIndex(0);
                ui::Text("Angles (Radians)");
                ui::TableSetColumnIndex(1);
                if (ui::Button("1 pi"))
                {
                    stepSize = pi_v<float>;
                }
                ui::SameLine();
                if (ui::Button("1/2 pi"))
                {
                    stepSize = pi_v<float>/2.0f;
                }
                ui::SameLine();
                if (ui::Button("1/4 pi"))
                {
                    stepSize = pi_v<float>/4.0f;
                }
                ui::SameLine();
                if (ui::Button("10/180 pi"))
                {
                    stepSize = (10.0f/180.0f) * pi_v<float>;
                }
                ui::SameLine();
                if (ui::Button("1/180 pi"))
                {
                    stepSize = (1.0f/180.0f) * pi_v<float>;
                }

                ui::TableNextRow();
                ui::TableSetColumnIndex(0);
                ui::Text("Masses");
                ui::TableSetColumnIndex(1);
                if (ui::Button("1 kg"))
                {
                    stepSize = 1.0f;
                }
                ui::SameLine();
                if (ui::Button("100 g"))
                {
                    stepSize = 0.1f;
                }
                ui::SameLine();
                if (ui::Button("10 g"))
                {
                    stepSize = 0.01f;
                }
                ui::SameLine();
                if (ui::Button("1 g"))
                {
                    stepSize = 0.001f;
                }
                ui::SameLine();
                if (ui::Button("100 mg"))
                {
                    stepSize = 0.0001f;
                }

                ui::EndTable();
            }

            ui::EndPopup();
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

        ui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, {1.0f, 0.0f});
        if (ui::InputScalar(label, ImGuiDataType_Float, &value, &stepSize, nullptr, "%.6f"))
        {
            rv.wasEdited = true;
        }
        ui::PopStyleVar();
        rv.shouldSave = ui::ItemValueShouldBeSaved();
        App::upd().addFrameAnnotation(frameAnnotationLabel, ui::GetItemRect());
        ui::DrawTooltipIfItemHovered("Step Size", "You can right-click to adjust the step size of the buttons");
        DrawStepSizeEditor(stepSize);

        return rv;
    }

    std::string GenerateVecFrameAnnotationLabel(
        OpenSim::AbstractProperty const& backingProperty,
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
        IPropertyEditor(IPropertyEditor const&) = default;
        IPropertyEditor(IPropertyEditor&&) noexcept = default;
        IPropertyEditor& operator=(IPropertyEditor const&) = default;
        IPropertyEditor& operator=(IPropertyEditor&&) noexcept = default;
    public:
        virtual ~IPropertyEditor() noexcept = default;

        bool isCompatibleWith(OpenSim::AbstractProperty const& prop) const
        {
            return implIsCompatibleWith(prop);
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> onDraw()
        {
            return implOnDraw();
        }

    private:
        virtual bool implIsCompatibleWith(OpenSim::AbstractProperty const&) const = 0;
        virtual std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() = 0;
    };

    // construction-time arguments for the property editor
    struct PropertyEditorArgs final {
        IPopupAPI* api;
        std::shared_ptr<UndoableModelStatePair const> model;
        std::function<OpenSim::Object const*()> objectAccessor;
        std::function<OpenSim::AbstractProperty const*()> propertyAccessor;
    };

    template<std::derived_from<OpenSim::AbstractProperty> ConcreteProperty>
    struct PropertyEditorTraits {
        static bool IsCompatibleWith(OpenSim::AbstractProperty const& prop)
        {
            return dynamic_cast<ConcreteProperty const*>(&prop) != nullptr;
        }
    };

    // partial implementation class for a specific property editor
    template<
        std::derived_from<OpenSim::AbstractProperty> ConcreteProperty,
        class Traits = PropertyEditorTraits<ConcreteProperty>
    >
    class PropertyEditor : public IPropertyEditor {
    public:
        using property_type = ConcreteProperty;

        static bool IsCompatibleWith(OpenSim::AbstractProperty const& prop)
        {
            return Traits::IsCompatibleWith(prop);
        }

        explicit PropertyEditor(PropertyEditorArgs args) :
            m_Args{std::move(args)}
        {
        }

    protected:
        property_type const* tryGetProperty() const
        {
            return dynamic_cast<property_type const*>(m_Args.propertyAccessor());
        }

        std::function<property_type const*()> getPropertyAccessor() const
        {
            return [accessor = this->m_Args.propertyAccessor]()
            {
                return dynamic_cast<property_type const*>(accessor());
            };
        }

        OpenSim::Model const& getModel() const
        {
            return m_Args.model->getModel();
        }

        std::shared_ptr<UndoableModelStatePair const> getModelPtr() const
        {
            return m_Args.model;
        }

        SimTK::State const& getState() const
        {
            return m_Args.model->getState();
        }

        OpenSim::Object const* tryGetObject() const
        {
            return m_Args.objectAccessor();
        }

        IPopupAPI* getPopupAPIPtr() const
        {
            return m_Args.api;
        }

        void pushPopup(std::unique_ptr<IPopup> p)
        {
            if (auto api = getPopupAPIPtr())
            {
                api->pushPopup(std::move(p));
            }
        }

    private:
        bool implIsCompatibleWith(OpenSim::AbstractProperty const& prop) const final
        {
            return Traits::IsCompatibleWith(prop);
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
            property_type const* maybeProp = tryGetProperty();
            if (!maybeProp)
            {
                return std::nullopt;
            }
            property_type const& prop = *maybeProp;

            // update any cached data
            if (!prop.equals(m_OriginalProperty))
            {
                m_OriginalProperty = prop;
                m_EditedProperty = prop;
            }

            ui::Separator();

            // draw name of the property in left-hand column
            DrawPropertyName(m_EditedProperty);
            ui::NextColumn();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < max(m_EditedProperty.size(), 1); ++idx)
            {
                ui::PushID(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editorRv = drawIthEditor(idx);
                ui::PopID();

                if (!rv)
                {
                    rv = std::move(editorRv);
                }
            }
            ui::NextColumn();

            return rv;
        }

        // draw an editor for one of the property's string values (might be a list)
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> drawIthEditor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // draw trash can that can delete an element from the property's list
            if (m_EditedProperty.isListProperty())
            {
                if (ui::Button(ICON_FA_TRASH))
                {
                    rv = MakePropElementDeleter<std::string>(idx);
                }
                ui::SameLine();
            }

            // read stored value from edited property
            //
            // care: optional properties have size==0, so perform a range check
            std::string value = idx < m_EditedProperty.size() ? m_EditedProperty.getValue(idx) : std::string{};

            ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
            if (ui::InputString("##stringeditor", value))
            {
                // update the edited property - don't rely on ImGui to remember edits
                m_EditedProperty.setValue(idx, value);
            }

            // globally annotate the editor rect, for downstream screenshot automation
            App::upd().addFrameAnnotation("ObjectPropertiesEditor::StringEditor/" + m_EditedProperty.getName(), ui::GetItemRect());

            if (ui::ItemValueShouldBeSaved())
            {
                rv = MakePropValueSetter(idx, m_EditedProperty.getValue(idx));
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
            property_type const* maybeProp = tryGetProperty();
            if (!maybeProp)
            {
                return std::nullopt;
            }
            property_type const& prop = *maybeProp;

            // update any cached data
            if (!prop.equals(m_OriginalProperty))
            {
                m_OriginalProperty = prop;
                m_EditedProperty = prop;
            }

            ui::Separator();

            // draw name of the property in left-hand column
            DrawPropertyName(m_EditedProperty);
            ui::NextColumn();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < max(m_EditedProperty.size(), 1); ++idx)
            {
                ui::PushID(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editorRv = drawIthEditor(idx);
                ui::PopID();

                if (!rv)
                {
                    rv = std::move(editorRv);
                }
            }
            ui::NextColumn();

            return rv;
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> drawIthEditor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // draw trash can that can delete an element from the property's list
            if (m_EditedProperty.isListProperty())
            {
                if (ui::Button(ICON_FA_TRASH))
                {
                    rv = MakePropElementDeleter<double>(idx);
                }
                ui::SameLine();
            }

            ui::SetNextItemWidth(ui::GetContentRegionAvail().x);

            // draw an invisible vertical line, so that `double` properties are properly
            // aligned with `Vec3` properties (that have a non-invisible R/G/B line)
            DrawColoredDimensionHintVerticalLine(Color::clear());

            // read stored value from edited property
            //
            // care: optional properties have size==0, so perform a range check
            auto value = static_cast<float>(idx < m_EditedProperty.size() ? m_EditedProperty.getValue(idx) : 0.0);
            auto frameAnnotationLabel = "ObjectPropertiesEditor::DoubleEditor/" + m_EditedProperty.getName();

            auto drawRV = DrawCustomScalarInput("##doubleeditor", value, m_StepSize, frameAnnotationLabel);

            if (drawRV.wasEdited)
            {
                // update the edited property - don't rely on ImGui to remember edits
                m_EditedProperty.setValue(idx, static_cast<double>(value));
            }
            if (drawRV.shouldSave)
            {
                rv = MakePropValueSetter(idx, m_EditedProperty.getValue(idx));
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
            property_type const* maybeProp = tryGetProperty();
            if (!maybeProp)
            {
                return std::nullopt;
            }
            property_type const& prop = *maybeProp;

            // update any cached data
            if (!prop.equals(m_OriginalProperty))
            {
                m_OriginalProperty = prop;
                m_EditedProperty = prop;
            }

            ui::Separator();

            // draw name of the property in left-hand column
            DrawPropertyName(m_EditedProperty);
            ui::NextColumn();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < max(m_EditedProperty.size(), 1); ++idx)
            {
                ui::PushID(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editorRv = drawIthEditor(idx);
                ui::PopID();

                if (!rv)
                {
                    rv = std::move(editorRv);
                }
            }
            ui::NextColumn();

            return rv;
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> drawIthEditor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // draw trash can that can delete an element from the property's list
            if (m_EditedProperty.isListProperty())
            {
                if (ui::Button(ICON_FA_TRASH))
                {
                    rv = MakePropElementDeleter<bool>(idx);
                }
                ui::SameLine();
            }

            // read stored value from edited property
            //
            // care: optional properties have size==0, so perform a range check
            bool value = idx < m_EditedProperty.size() ? m_EditedProperty.getValue(idx) : false;
            bool edited = false;

            ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
            if (ui::Checkbox("##booleditor", &value))
            {
                // update the edited property - don't rely on ImGui to remember edits
                m_EditedProperty.setValue(idx, value);
                edited = true;
            }

            // globally annotate the editor rect, for downstream screenshot automation
            App::upd().addFrameAnnotation("ObjectPropertiesEditor::BoolEditor/" + m_EditedProperty.getName(), ui::GetItemRect());

            if (edited || ui::ItemValueShouldBeSaved())
            {
                rv = MakePropValueSetter(idx, m_EditedProperty.getValue(idx));
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
                float modelToEditedValueScaler_,
                SimTK::Transform  modelToEditedTransform_) :

                m_ModelToEditedValueScaler{modelToEditedValueScaler_},
                m_ModelToEditedTransform{std::move(modelToEditedTransform_)}
            {
            }

            Vec3 modelValueToEditedValue(Vec3 const& modelValue) const
            {
                return ToVec3(static_cast<double>(m_ModelToEditedValueScaler) * (m_ModelToEditedTransform * ToSimTKVec3(modelValue)));
            }

            Vec3 editedValueToModelValue(Vec3 const& editedValue) const
            {
                return ToVec3(m_ModelToEditedTransform.invert() * ToSimTKVec3(editedValue/m_ModelToEditedValueScaler));
            }
        private:
            float m_ModelToEditedValueScaler;
            SimTK::Transform m_ModelToEditedTransform;
        };

        // returns `true` if the Vec3 property is expressed w.r.t. a frame
        bool isPropertyExpressedWithinAParentFrame() const
        {
            return getParentToGroundTransform() != std::nullopt;
        }

        // returns `true` if the Vec3 property is edited in radians
        bool isPropertyEditedInRadians() const
        {
            return IsEqualCaseInsensitive(m_EditedProperty.getName(), "orientation");
        }

        // if the Vec3 property has a parent frame, returns a transform that maps the Vec3
        // property's value to ground
        std::optional<SimTK::Transform> getParentToGroundTransform() const
        {
            OpenSim::Object const* const obj = tryGetObject();
            if (!obj)
            {
                return std::nullopt;  // cannot find the property's parent object?
            }

            auto const* const component = dynamic_cast<OpenSim::Component const*>(obj);
            if (!component)
            {
                return std::nullopt;  // the object isn't an OpenSim component
            }

            if (&component->getRoot() != &getModel())
            {
                return std::nullopt;  // the object is not within the tree of the model (#800)
            }

            auto const positionPropName = TryGetPositionalPropertyName(*component);
            if (!positionPropName)
            {
                return std::nullopt;  // the component doesn't have a logical positional property that can be edited with the transform
            }

            OpenSim::Property<SimTK::Vec3> const* const prop = tryGetProperty();
            if (!prop)
            {
                return std::nullopt;  // can't access the property this editor is ultimately editing
            }

            if (prop->getName() != *positionPropName)
            {
                return std::nullopt;  // the property this editor is editing isn't a logically positional one
            }

            std::optional<SimTK::Transform> transform = TryGetParentToGroundTransform(*component, getState());
            if (!transform)
            {
                return std::nullopt;  // the component doesn't have a logical position-to-ground transform
            }

            return transform;
        }

        // if the user has selected a different frame in which to edit 3D quantities, then
        // returns a transform that maps Vec3 properties expressed in ground to the other
        // frame
        std::optional<SimTK::Transform> getGroundToUserSelectedFrameTransform() const
        {
            if (!m_MaybeUserSelectedFrameAbsPath)
            {
                return std::nullopt;
            }

            OpenSim::Model const& model = getModel();
            auto const* frame = FindComponent<OpenSim::Frame>(model, *m_MaybeUserSelectedFrameAbsPath);
            return frame->getTransformInGround(getState()).invert();
        }

        ValueConverter getValueConverter() const
        {
            float conversionCoefficient = 1.0f;
            if (isPropertyEditedInRadians() && !m_OrientationValsAreInRadians)
            {
                conversionCoefficient = static_cast<float>(SimTK_RADIAN_TO_DEGREE);
            }

            std::optional<SimTK::Transform> const parent2ground = getParentToGroundTransform();
            std::optional<SimTK::Transform> const ground2frame = getGroundToUserSelectedFrameTransform();
            SimTK::Transform const transform = parent2ground && ground2frame ?
                (*ground2frame) * (*parent2ground) :
                SimTK::Transform{};

            return ValueConverter{conversionCoefficient, transform};
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() final
        {
            property_type const* maybeProp = tryGetProperty();
            if (!maybeProp)
            {
                return std::nullopt;
            }
            property_type const& prop = *maybeProp;

            // update any cached data
            if (!prop.equals(m_OriginalProperty))
            {
                m_OriginalProperty = prop;
                m_EditedProperty = prop;
            }

            // compute value converter (applies to all values)
            ValueConverter const valueConverter = getValueConverter();


            // draw UI


            ui::Separator();

            // draw name of the property in left-hand column
            DrawPropertyName(m_EditedProperty);
            ui::NextColumn();

            // top line of right column shows "reexpress in" editor (if applicable)
            drawReexpressionEditorIfApplicable();

            // draw radians/degrees conversion toggle button (if applicable)
            drawDegreesToRadiansConversionToggle();

            // draw `[0, 1]` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < max(m_EditedProperty.size(), 1); ++idx)
            {
                ui::PushID(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editorRv = drawIthEditor(valueConverter, idx);
                ui::PopID();

                if (!rv)
                {
                    rv = std::move(editorRv);
                }
            }
            ui::NextColumn();

            return rv;
        }

        void drawReexpressionEditorIfApplicable()
        {
            if (!isPropertyExpressedWithinAParentFrame())
            {
                return;
            }

            CStringView const defaultedLabel = "(parent frame)";
            std::string const preview = m_MaybeUserSelectedFrameAbsPath ?
                m_MaybeUserSelectedFrameAbsPath->getComponentName() :
                std::string{defaultedLabel};

            ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
            if (ui::BeginCombo("##reexpressioneditor", preview))
            {
                ui::TextDisabled("Frame (editing)");
                ui::SameLine();
                ui::DrawHelpMarker("Note: this only affects the values that the quantities are edited in. It does not change the frame that the component is attached to. You can change the frame attachment by using the component's context menu: Socket > $FRAME > (edit button) > (select new frame)");
                ui::Dummy({0.0f, 0.25f*ui::GetTextLineHeight()});

                int imguiID = 0;

                // draw "default" (reset) option
                {
                    ui::Separator();
                    ui::PushID(imguiID++);
                    bool selected = !m_MaybeUserSelectedFrameAbsPath.has_value();
                    if (ui::Selectable(defaultedLabel, &selected))
                    {
                        m_MaybeUserSelectedFrameAbsPath.reset();
                    }
                    ui::PopID();
                    ui::Separator();
                }

                // draw selectable for each frame in the model
                for (OpenSim::Frame const& frame : getModel().getComponentList<OpenSim::Frame>())
                {
                    OpenSim::ComponentPath const frameAbsPath = GetAbsolutePath(frame);

                    ui::PushID(imguiID++);
                    bool selected = frameAbsPath == m_MaybeUserSelectedFrameAbsPath;
                    if (ui::Selectable(frame.getName(), &selected))
                    {
                        m_MaybeUserSelectedFrameAbsPath = frameAbsPath;
                    }
                    ui::PopID();
                }

                ui::EndCombo();
            }
        }

        // draws an editor for the `ith` Vec3 element of the given (potentially, list) property
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> drawIthEditor(
            ValueConverter const& valueConverter,
            int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // draw trash can that can delete an element from the property's list
            if (m_EditedProperty.isListProperty())
            {
                if (ui::Button(ICON_FA_TRASH))
                {
                    rv = MakePropElementDeleter<SimTK::Vec3>(idx);
                }
                ui::SameLine();
            }

            // read stored value from edited property
            //
            // care: optional properties have size==0, so perform a range check
            Vec3 const rawValue = ToVec3(idx < m_EditedProperty.size() ? m_EditedProperty.getValue(idx) : SimTK::Vec3{0.0});
            Vec3 const editedValue = valueConverter.modelValueToEditedValue(rawValue);

            // draw an editor for each component of the Vec3
            bool shouldSave = false;
            for (Vec3::size_type i = 0; i < 3; ++i)
            {
                ComponentEditorReturn const componentEditorRv = drawVec3ComponentEditor(idx, i, editedValue, valueConverter);
                shouldSave = shouldSave || componentEditorRv == ComponentEditorReturn::ShouldSave;
            }

            // if any component editor indicated that it should be saved then propagate that upwards
            if (shouldSave)
            {
                rv = MakePropValueSetter(idx, m_EditedProperty.getValue(idx));
            }

            return rv;
        }

        enum class ComponentEditorReturn { None, ShouldSave };

        // draws float input for a single component of the Vec3 (e.g. vec.x)
        ComponentEditorReturn drawVec3ComponentEditor(
            int idx,
            Vec3::size_type i,
            Vec3 editedValue,
            ValueConverter const& valueConverter)
        {
            ui::PushID(i);
            ui::SetNextItemWidth(ui::GetContentRegionAvail().x);

            // draw dimension hint (color bar next to the input)
            DrawColoredDimensionHintVerticalLine(IthDimensionColor(i));

            // draw the input editor
            auto frameAnnotation = GenerateVecFrameAnnotationLabel(m_EditedProperty, i);
            auto drawRV = DrawCustomScalarInput("##valueinput", editedValue[i], m_StepSize, frameAnnotation);

            if (drawRV.wasEdited)
            {
                // un-convert the value on save
                Vec3 const savedValue = valueConverter.editedValueToModelValue(editedValue);
                m_EditedProperty.setValue(idx, ToSimTKVec3(savedValue));
            }

            ui::PopID();

            return drawRV.shouldSave ? ComponentEditorReturn::ShouldSave : ComponentEditorReturn::None;
        }

        // draws button that lets the user toggle between inputting radians vs. degrees
        void drawDegreesToRadiansConversionToggle()
        {
            if (!isPropertyEditedInRadians())
            {
                return;
            }

            if (m_OrientationValsAreInRadians)
            {
                if (ui::Button("radians"))
                {
                    m_OrientationValsAreInRadians = !m_OrientationValsAreInRadians;
                }
                App::upd().addFrameAnnotation("ObjectPropertiesEditor::OrientationToggle/" + m_EditedProperty.getName(), ui::GetItemRect());
                ui::DrawTooltipBodyOnlyIfItemHovered("This quantity is edited in radians (click to switch to degrees)");
            }
            else
            {
                if (ui::Button("degrees"))
                {
                    m_OrientationValsAreInRadians = !m_OrientationValsAreInRadians;
                }
                App::upd().addFrameAnnotation("ObjectPropertiesEditor::OrientationToggle/" + m_EditedProperty.getName(), ui::GetItemRect());
                ui::DrawTooltipBodyOnlyIfItemHovered("This quantity is edited in degrees (click to switch to radians)");
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
            property_type const* maybeProp = tryGetProperty();
            if (!maybeProp)
            {
                return std::nullopt;
            }
            property_type const& prop = *maybeProp;

            // update any cached data
            if (!prop.equals(m_OriginalProperty))
            {
                m_OriginalProperty = prop;
                m_EditedProperty = prop;
            }

            ui::Separator();

            // draw name of the property in left-hand column
            DrawPropertyName(m_EditedProperty);
            ui::NextColumn();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < max(m_EditedProperty.size(), 1); ++idx)
            {
                ui::PushID(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editorRv = drawIthEditor(idx);
                ui::PopID();

                if (!rv)
                {
                    rv = std::move(editorRv);
                }
            }
            ui::NextColumn();

            return rv;
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> drawIthEditor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // draw trash can that can delete an element from the property's list
            if (m_EditedProperty.isListProperty())
            {
                if (ui::Button(ICON_FA_TRASH))
                {
                    rv = MakePropElementDeleter<SimTK::Vec6>(idx);
                }
            }

            // read latest raw value as-stored in edited property
            //
            // care: `getValue` can return `nullptr` if the property is optional (size == 0)
            std::array<float, 6> rawValue = idx < m_EditedProperty.size() ?
                ToArray(m_EditedProperty.getValue(idx)) :
                std::array<float, 6>{};

            bool shouldSave = false;
            for (int i = 0; i < 2; ++i)
            {
                ui::PushID(i);

                ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
                if (ui::InputFloat3("##vec6editor", rawValue.data() + static_cast<ptrdiff_t>(3*i), "%.6f"))
                {
                    m_EditedProperty.updValue(idx)[3*i + 0] = static_cast<double>(rawValue[3*i + 0]);
                    m_EditedProperty.updValue(idx)[3*i + 1] = static_cast<double>(rawValue[3*i + 1]);
                    m_EditedProperty.updValue(idx)[3*i + 2] = static_cast<double>(rawValue[3*i + 2]);
                }
                shouldSave = shouldSave || ui::ItemValueShouldBeSaved();
                App::upd().addFrameAnnotation("ObjectPropertiesEditor::Vec6Editor/" + m_EditedProperty.getName(), ui::GetItemRect());

                ui::PopID();
            }

            if (shouldSave)
            {
                rv = MakePropValueSetter(idx, m_EditedProperty.getValue(idx));
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
            property_type const* maybeProp = tryGetProperty();
            if (!maybeProp)
            {
                return std::nullopt;
            }
            property_type const& prop = *maybeProp;

            // update any cached data
            if (!prop.equals(m_OriginalProperty))
            {
                m_OriginalProperty = prop;
                m_EditedProperty = prop;
            }

            ui::Separator();

            // draw name of the property in left-hand column
            DrawPropertyName(m_EditedProperty);
            ui::NextColumn();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < max(m_EditedProperty.size(), 1); ++idx)
            {
                ui::PushID(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editorRv = drawIthEditor(idx);
                ui::PopID();

                if (!rv)
                {
                    rv = std::move(editorRv);
                }
            }
            ui::NextColumn();

            return rv;
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> drawIthEditor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // draw trash can that can delete an element from the property's list
            if (m_EditedProperty.isListProperty())
            {
                if (ui::Button(ICON_FA_TRASH))
                {
                    rv = MakePropElementDeleter<int>(idx);
                }
                ui::SameLine();
            }

            // read stored value from edited property
            //
            // care: optional properties have size==0, so perform a range check
            int value = idx < m_EditedProperty.size() ? m_EditedProperty.getValue(idx) : 0;
            bool edited = false;

            ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
            if (ui::InputInt("##inteditor", &value))
            {
                // update the edited property - don't rely on ImGui to remember edits
                m_EditedProperty.setValue(idx, value);
                edited = true;
            }

            // globally annotate the editor rect, for downstream screenshot automation
            App::upd().addFrameAnnotation("ObjectPropertiesEditor::IntEditor/" + m_EditedProperty.getName(), ui::GetItemRect());

            if (edited || ui::ItemValueShouldBeSaved())
            {
                rv = MakePropValueSetter(idx, m_EditedProperty.getValue(idx));
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
            property_type const* maybeProp = tryGetProperty();
            if (!maybeProp)
            {
                return std::nullopt;
            }
            property_type const& prop = *maybeProp;

            // update any cached data
            if (!prop.equals(m_OriginalProperty))
            {
                m_OriginalProperty = prop;
                m_EditedProperty = prop;
            }

            ui::Separator();

            // draw name of the property in left-hand column
            DrawPropertyName(m_EditedProperty);
            ui::NextColumn();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < max(m_EditedProperty.size(), 1); ++idx)
            {
                ui::PushID(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editorRv = drawIthEditor(idx);
                ui::PopID();

                if (!rv)
                {
                    rv = std::move(editorRv);
                }
            }
            ui::NextColumn();

            return rv;
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> drawIthEditor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            if (m_EditedProperty.isListProperty())
            {
                return rv;  // HACK: ignore list props for now
            }

            if (m_EditedProperty.empty())
            {
                return rv;  // HACK: ignore optional props for now
            }

            bool shouldSave = false;

            Color color = ToColor(m_EditedProperty.getValue());
            ui::SetNextItemWidth(ui::GetContentRegionAvail().x);

            if (ui::ColorEditRGBA("##coloreditor", color))
            {
                SimTK::Vec3 newColor;
                newColor[0] = static_cast<double>(color[0]);
                newColor[1] = static_cast<double>(color[1]);
                newColor[2] = static_cast<double>(color[2]);

                m_EditedProperty.updValue().set_color(newColor);
                m_EditedProperty.updValue().set_opacity(static_cast<double>(color[3]));
            }
            shouldSave = shouldSave || ui::ItemValueShouldBeSaved();

            bool isVisible = m_EditedProperty.getValue().get_visible();
            if (ui::Checkbox("is visible", &isVisible))
            {
                m_EditedProperty.updValue().set_visible(isVisible);
            }
            shouldSave = shouldSave || ui::ItemValueShouldBeSaved();

            // DisplayPreference
            {
                static_assert(OpenSim::VisualRepresentation::DrawDefault == -1);
                static_assert(OpenSim::VisualRepresentation::Hide == 0);
                static_assert(OpenSim::VisualRepresentation::DrawPoints == 1);
                static_assert(OpenSim::VisualRepresentation::DrawWireframe == 2);
                static_assert(OpenSim::VisualRepresentation::DrawSurface == 3);
                auto const options = std::to_array<CStringView>({
                    "Default",
                    "Hide",
                    "Points",
                    "Wireframe",
                    "Surface",
                });
                size_t index = clamp(static_cast<size_t>(m_EditedProperty.getValue().get_representation())+1, static_cast<size_t>(0), options.size());
                if (ui::Combo("##DisplayPref", &index, options)) {
                    m_EditedProperty.updValue().set_representation(static_cast<OpenSim::VisualRepresentation>(static_cast<int>(index)-1));
                    shouldSave = true;
                }
            }

            if (shouldSave)
            {
                rv = MakePropValueSetter(idx, m_EditedProperty.getValue(idx));
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

            property_type const* maybeProp = tryGetProperty();
            if (!maybeProp)
            {
                return std::nullopt;  // cannot find property
            }
            property_type const& prop = *maybeProp;

            if (empty(prop.getValue()))
            {
                return std::nullopt;  // no editable contact set on the property
            }

            OpenSim::HuntCrossleyForce::ContactParameters const& params = prop.getValue()[0];

            // update cached editors, if necessary
            if (!m_MaybeNestedEditor)
            {
                m_MaybeNestedEditor.emplace(getPopupAPIPtr(), getModelPtr(), [&params]() { return &params; });
            }
            ObjectPropertiesEditor& nestedEditor = *m_MaybeNestedEditor;

            ui::Columns();
            auto resp = nestedEditor.onDraw();
            ui::Columns(2);

            if (resp)
            {
                // careful here: the response has a correct updater but doesn't know the full
                // path to the housing component, so we have to wrap the updater with
                // appropriate lookups etc

                rv = [=](OpenSim::AbstractProperty& p) mutable
                {
                    auto* downcasted = dynamic_cast<OpenSim::Property<OpenSim::HuntCrossleyForce::ContactParametersSet>*>(&p);
                    if (downcasted && !empty(downcasted->getValue()))
                    {
                        OpenSim::HuntCrossleyForce::ContactParameters& contactParams = At(downcasted->updValue(), 0);
                        if (params.hasProperty(resp->getPropertyName()))
                        {
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
            property_type const* maybeProp = tryGetProperty();
            if (!maybeProp)
            {
                return std::nullopt;
            }
            property_type const& prop = *maybeProp;

            ui::Separator();
            DrawPropertyName(prop);
            ui::NextColumn();
            if (ui::Button(ICON_FA_EDIT))
            {
                pushPopup(createGeometryPathEditorPopup());
            }
            ui::NextColumn();


            if (*m_ReturnValueHolder)
            {
                std::optional<ObjectPropertyEdit> edit;
                std::swap(*m_ReturnValueHolder, edit);
                return edit->getUpdater();
            }
            else
            {
                return std::nullopt;
            }
        }

        std::unique_ptr<IPopup> createGeometryPathEditorPopup()
        {
            auto accessor = getPropertyAccessor();
            return std::make_unique<GeometryPathEditorPopup>(
                "Edit Geometry Path",
                getModelPtr(),
                [accessor]() -> OpenSim::GeometryPath const*
                {
                    property_type const* p = accessor();
                    if (!p || p->isListProperty())
                    {
                        return nullptr;
                    }
                    return dynamic_cast<OpenSim::GeometryPath const*>(&p->getValueAsObject());
                },
                [shared = m_ReturnValueHolder, accessor](OpenSim::GeometryPath const& gp) mutable
                {
                    if (property_type const* prop = accessor())
                    {
                        *shared = ObjectPropertyEdit{*prop, MakePropValueSetter<OpenSim::GeometryPath const&, OpenSim::AbstractGeometryPath>(0, gp)};
                    }
                }
            );
        }

        // shared between this property editor and a popup it may have spawned
        std::shared_ptr<std::optional<ObjectPropertyEdit>> m_ReturnValueHolder = std::make_shared<std::optional<ObjectPropertyEdit>>();
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
        AbstractGeometryPathPropertyEditor
    >;

    // a type-erased entry in the runtime registry LUT
    class PropertyEditorRegistryEntry final {
    public:
        // a function that tests whether a property editor in the registry is suitable
        // for the given abstract property
        using PropertyEditorTester = bool(*)(OpenSim::AbstractProperty const&);

        // a function that can be used to construct a pointer to a virtual property editor
        using PropertyEditorCtor = std::unique_ptr<IPropertyEditor>(*)(PropertyEditorArgs);

        // create a type-erased entry from a known, concrete, editor
        template<std::derived_from<IPropertyEditor> ConcretePropertyEditor>
        constexpr static PropertyEditorRegistryEntry make_entry()
        {
            auto const testerFn = ConcretePropertyEditor::IsCompatibleWith;
            auto const typeErasedCtorFn = [](PropertyEditorArgs args) -> std::unique_ptr<IPropertyEditor>
            {
                return std::make_unique<ConcretePropertyEditor>(std::move(args));
            };

            return PropertyEditorRegistryEntry{testerFn, typeErasedCtorFn};
        }

        bool isCompatibleWith(OpenSim::AbstractProperty const& abstractProp) const
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
            OpenSim::AbstractProperty const* prop = args.propertyAccessor();
            if (!prop)
            {
                return nullptr;  // cannot access the property
            }

            auto const it = find_if(
                m_Entries,
                [&prop](auto const& entry) { return entry.isCompatibleWith(*prop); }
            );
            if (it == m_Entries.end())
            {
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
class osc::ObjectPropertiesEditor::Impl final {
public:
    Impl(
        IPopupAPI* api_,
        std::shared_ptr<UndoableModelStatePair const> targetModel_,
        std::function<OpenSim::Object const*()> objectGetter_) :

        m_API{api_},
        m_TargetModel{std::move(targetModel_)},
        m_ObjectGetter{std::move(objectGetter_)}
    {
    }

    std::optional<ObjectPropertyEdit> onDraw()
    {
        if (OpenSim::Object const* maybeObj = m_ObjectGetter())
        {
            // object accessible: draw property editors
            return drawPropertyEditors(*maybeObj);
        }
        else
        {
            // object inaccessible: draw nothing
            return std::nullopt;
        }
    }
private:

    // draws all property editors for the given object
    std::optional<ObjectPropertyEdit> drawPropertyEditors(
        OpenSim::Object const& obj)
    {
        if (m_PreviousObject != &obj)
        {
            // the object has changed since the last draw call, so
            // reset all property editor state
            m_PropertyEditorsByName.clear();
            m_PreviousObject = &obj;
        }

        // draw each editor and return the last property edit (or std::nullopt)
        std::optional<ObjectPropertyEdit> rv;

        ui::Columns(2);
        for (int i = 0; i < obj.getNumProperties(); ++i)
        {
            ui::PushID(i);
            std::optional<ObjectPropertyEdit> maybeEdit = tryDrawPropertyEditor(obj, obj.getPropertyByIndex(i));
            ui::PopID();

            if (maybeEdit)
            {
                rv = std::move(maybeEdit);
            }
        }
        ui::Columns();

        return rv;
    }

    // tries to draw one property editor for one property of an object
    std::optional<ObjectPropertyEdit> tryDrawPropertyEditor(
        OpenSim::Object const& obj,
        OpenSim::AbstractProperty const& prop)
    {
        if (prop.getName().starts_with("socket_"))
        {
            // #542: ignore properties that begin with `socket_`, because
            // they are proxy properties to the object's sockets and should
            // be manipulated via socket, rather than property, editors
            return std::nullopt;
        }
        else if (IPropertyEditor* maybeEditor = tryGetPropertyEditor(prop))
        {
            return drawPropertyEditor(obj, prop, *maybeEditor);
        }
        else
        {
            drawNonEditablePropertyDetails(prop);
            return std::nullopt;
        }
    }

    // draws a property editor for the given object+property
    std::optional<ObjectPropertyEdit> drawPropertyEditor(
        OpenSim::Object const& obj,
        OpenSim::AbstractProperty const& prop,
        IPropertyEditor& editor)
    {
        ui::PushID(prop.getName());
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> maybeUpdater = editor.onDraw();
        ui::PopID();

        if (maybeUpdater)
        {
            return ObjectPropertyEdit{obj, prop, std::move(maybeUpdater).value()};
        }
        else
        {
            return std::nullopt;
        }
    }

    // draws a non-editable representation of a property
    void drawNonEditablePropertyDetails(
        OpenSim::AbstractProperty const& prop)
    {
        ui::Separator();
        DrawPropertyName(prop);
        ui::NextColumn();
        ui::TextUnformatted(prop.toString());
        ui::NextColumn();
    }

    // try get/construct a property editor for the given property
    IPropertyEditor* tryGetPropertyEditor(OpenSim::AbstractProperty const& prop)
    {
        auto const [it, inserted] = m_PropertyEditorsByName.try_emplace(prop.getName(), nullptr);

        if (inserted || (it->second && !it->second->isCompatibleWith(prop)))
        {
            // need to create a new editor because either it hasn't been made yet or the existing
            // editor is for a different type
            it->second = c_Registry.tryCreateEditor({
                .api = m_API,
                .model = m_TargetModel,
                .objectAccessor = m_ObjectGetter,
                .propertyAccessor = MakePropertyAccessor(m_ObjectGetter, prop.getName()),
            });
        }

        return it->second.get();
    }

    IPopupAPI* m_API;
    std::shared_ptr<UndoableModelStatePair const> m_TargetModel;
    std::function<OpenSim::Object const*()> m_ObjectGetter;
    OpenSim::Object const* m_PreviousObject = nullptr;
    std::unordered_map<std::string, std::unique_ptr<IPropertyEditor>> m_PropertyEditorsByName;
};


// public API (ObjectPropertiesEditor)

osc::ObjectPropertiesEditor::ObjectPropertiesEditor(
    IPopupAPI* api_,
    std::shared_ptr<UndoableModelStatePair const> targetModel_,
    std::function<OpenSim::Object const*()> objectGetter_) :

    m_Impl{std::make_unique<Impl>(api_, std::move(targetModel_), std::move(objectGetter_))}
{
}

osc::ObjectPropertiesEditor::ObjectPropertiesEditor(ObjectPropertiesEditor&&) noexcept = default;
osc::ObjectPropertiesEditor& osc::ObjectPropertiesEditor::operator=(ObjectPropertiesEditor&&) noexcept = default;
osc::ObjectPropertiesEditor::~ObjectPropertiesEditor() noexcept = default;

std::optional<ObjectPropertyEdit> osc::ObjectPropertiesEditor::onDraw()
{
    return m_Impl->onDraw();
}
