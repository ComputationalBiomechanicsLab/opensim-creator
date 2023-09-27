#include "ObjectPropertiesEditor.hpp"

#include <OpenSimCreator/Bindings/SimTKHelpers.hpp>
#include <OpenSimCreator/Model/UndoableModelStatePair.hpp>
#include <OpenSimCreator/UI/Middleware/EditorAPI.hpp>
#include <OpenSimCreator/UI/Widgets/GeometryPathPropertyEditorPopup.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <nonstd/span.hpp>
#include <OpenSim/Common/AbstractProperty.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/Object.h>
#include <OpenSim/Common/Property.h>
#include <OpenSim/Simulation/Model/Appearance.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/GeometryPath.h>
#include <OpenSim/Simulation/Model/HuntCrossleyForce.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Maths/Constants.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/StringHelpers.hpp>
#include <OscarConfiguration.hpp>
#include <SimTKcommon/Constants.h>
#include <SimTKcommon/SmallMatrix.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <utility>

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

    // returns an `osc::Color` extracted from the given `OpenSim::Appearance`
    osc::Color ToColor(OpenSim::Appearance const& appearance)
    {
        SimTK::Vec3 const& rgb = appearance.get_color();
        double const a = appearance.get_opacity();

        return
        {
            static_cast<float>(rgb[0]),
            static_cast<float>(rgb[1]),
            static_cast<float>(rgb[2]),
            static_cast<float>(a),
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
    template<typename T>
    std::function<void(OpenSim::AbstractProperty&)> MakePropValueSetter(int idx, T value)
    {
        return [idx, value](OpenSim::AbstractProperty& p)
        {
            auto* const ps = dynamic_cast<OpenSim::Property<T>*>(&p);
            if (!ps)
            {
                return;  // types don't match: caller probably mismatched properties
            }
            ps->setValue(idx, value);
        };
    }

    // draws the property name and (optionally) comment tooltip
    void DrawPropertyName(OpenSim::AbstractProperty const& prop)
    {
        ImGui::TextUnformatted(prop.getName().c_str());

        if (!prop.getComment().empty())
        {
            ImGui::SameLine();
            osc::DrawHelpMarker(prop.getComment());
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
    osc::Color IthDimensionColor(glm::vec3::length_type i)
    {
        osc::Color color = {0.0f, 0.0f, 0.0f, 0.6f};
        color[i] = 1.0f;
        return color;
    }

    // draws a little vertical line, which is usually used to visually indicate
    // x/y/z to the user
    void DrawColoredDimensionHintVerticalLine(osc::Color const& color)
    {
        ImDrawList* const l = ImGui::GetWindowDrawList();
        glm::vec2 const p = ImGui::GetCursorScreenPos();
        float const h = ImGui::GetTextLineHeight() + 2.0f*ImGui::GetStyle().FramePadding.y + 2.0f*ImGui::GetStyle().FrameBorderSize;
        glm::vec2 const dims = glm::vec2{4.0f, h};
        l->AddRectFilled(p, p + dims, ImGui::ColorConvertFloat4ToU32(glm::vec4{color}));
        ImGui::SetCursorScreenPos({p.x + 4.0f, p.y});
    }

    // draws a context menu that the user can use to change the step interval of the +/- buttons
    void DrawStepSizeEditor(float& stepSize)
    {
        if (ImGui::BeginPopupContextItem("##valuecontextmenu"))
        {
            ImGui::Text("Set Step Size");
            ImGui::SameLine();
            osc::DrawHelpMarker("Sets the decrement/increment of the + and - buttons. Can be handy for tweaking property values");
            ImGui::Dummy({0.0f, 0.1f*ImGui::GetTextLineHeight()});
            ImGui::Separator();
            ImGui::Dummy({0.0f, 0.2f*ImGui::GetTextLineHeight()});

            if (ImGui::BeginTable("CommonChoicesTable", 2, ImGuiTableFlags_SizingStretchProp))
            {
                ImGui::TableSetupColumn("Type");
                ImGui::TableSetupColumn("Options");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Custom");
                ImGui::TableSetColumnIndex(1);
                ImGui::InputFloat("##stepsizeinput", &stepSize, 0.0f, 0.0f, "%.6f");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Lengths");
                ImGui::TableSetColumnIndex(1);
                if (ImGui::Button("10 cm"))
                {
                    stepSize = 0.1f;
                }
                ImGui::SameLine();
                if (ImGui::Button("1 cm"))
                {
                    stepSize = 0.01f;
                }
                ImGui::SameLine();
                if (ImGui::Button("1 mm"))
                {
                    stepSize = 0.001f;
                }
                ImGui::SameLine();
                if (ImGui::Button("0.1 mm"))
                {
                    stepSize = 0.0001f;
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Angles (Degrees)");
                ImGui::TableSetColumnIndex(1);
                if (ImGui::Button("180"))
                {
                    stepSize = 180.0f;
                }
                ImGui::SameLine();
                if (ImGui::Button("90"))
                {
                    stepSize = 90.0f;
                }
                ImGui::SameLine();
                if (ImGui::Button("45"))
                {
                    stepSize = 45.0f;
                }
                ImGui::SameLine();
                if (ImGui::Button("10"))
                {
                    stepSize = 10.0f;
                }
                ImGui::SameLine();
                if (ImGui::Button("1"))
                {
                    stepSize = 1.0f;
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Angles (Radians)");
                ImGui::TableSetColumnIndex(1);
                if (ImGui::Button("1 pi"))
                {
                    stepSize = osc::fpi;
                }
                ImGui::SameLine();
                if (ImGui::Button("1/2 pi"))
                {
                    stepSize = osc::fpi2;
                }
                ImGui::SameLine();
                if (ImGui::Button("1/4 pi"))
                {
                    stepSize = osc::fpi4;
                }
                ImGui::SameLine();
                if (ImGui::Button("10/180 pi"))
                {
                    stepSize = (10.0f/180.0f) * osc::fpi;
                }
                ImGui::SameLine();
                if (ImGui::Button("1/180 pi"))
                {
                    stepSize = (1.0f/180.0f) * osc::fpi;
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Masses");
                ImGui::TableSetColumnIndex(1);
                if (ImGui::Button("1 kg"))
                {
                    stepSize = 1.0f;
                }
                ImGui::SameLine();
                if (ImGui::Button("100 g"))
                {
                    stepSize = 0.1f;
                }
                ImGui::SameLine();
                if (ImGui::Button("10 g"))
                {
                    stepSize = 0.01f;
                }
                ImGui::SameLine();
                if (ImGui::Button("1 g"))
                {
                    stepSize = 0.001f;
                }
                ImGui::SameLine();
                if (ImGui::Button("100 mg"))
                {
                    stepSize = 0.0001f;
                }

                ImGui::EndTable();
            }

            ImGui::EndPopup();
        }
    }

    struct ScalarInputRv final {
        bool wasEdited = false;
        bool shouldSave = false;
    };

    ScalarInputRv DrawCustomScalarInput(
        osc::CStringView label,
        float& value,
        float& stepSize,
        std::string_view frameAnnotationLabel)
    {
        ScalarInputRv rv;

        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, {1.0f, 0.0f});
        if (ImGui::InputScalar(label.c_str(), ImGuiDataType_Float, &value, &stepSize, nullptr, "%.6f"))
        {
            rv.wasEdited = true;
        }
        ImGui::PopStyleVar();
        rv.shouldSave = osc::ItemValueShouldBeSaved();
        osc::App::upd().addFrameAnnotation(frameAnnotationLabel, osc::GetItemRect());
        osc::DrawTooltipIfItemHovered("Step Size", "You can right-click to adjust the step size of the buttons");
        DrawStepSizeEditor(stepSize);

        return rv;
    }

    std::string GenerateVecFrameAnnotationLabel(
        OpenSim::AbstractProperty const& backingProperty,
        glm::vec3::length_type ithDimension)
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
    class VirtualPropertyEditor {
    protected:
        VirtualPropertyEditor() = default;
        VirtualPropertyEditor(VirtualPropertyEditor const&) = default;
        VirtualPropertyEditor(VirtualPropertyEditor&&) noexcept = default;
        VirtualPropertyEditor& operator=(VirtualPropertyEditor const&) = default;
        VirtualPropertyEditor& operator=(VirtualPropertyEditor&&) noexcept = default;
    public:
        virtual ~VirtualPropertyEditor() noexcept = default;

        bool isCompatibleWith(OpenSim::AbstractProperty const& prop) const
        {
            return typeid(prop) == implGetTypeInfo();
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> onDraw()
        {
            return implOnDraw();
        }

    private:
        virtual std::type_info const& implGetTypeInfo() const = 0;
        virtual std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() = 0;
    };
}

// concrete property editors for simple (e.g. bool, double) types
namespace
{
    // concrete property editor for a simple `std::string` value
    class StringPropertyEditor final : public VirtualPropertyEditor {
    public:
        using property_type = OpenSim::SimpleProperty<std::string>;

        StringPropertyEditor(
            osc::EditorAPI*,
            std::shared_ptr<osc::UndoableModelStatePair const> const&,
            std::function<OpenSim::Object const*()> const&,
            std::function<property_type const*()> const& accessor_) :  // NOLINT(modernize-pass-by-value)

            m_Accessor{accessor_}
        {
        }
    private:
        std::type_info const& implGetTypeInfo() const final
        {
            return typeid(property_type);
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() final
        {
            property_type const* maybeProp = m_Accessor();
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

            ImGui::Separator();

            // draw name of the property in left-hand column
            DrawPropertyName(m_EditedProperty);
            ImGui::NextColumn();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < std::max(m_EditedProperty.size(), 1); ++idx)
            {
                ImGui::PushID(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editorRv = drawIthEditor(idx);
                ImGui::PopID();

                if (!rv)
                {
                    rv = std::move(editorRv);
                }
            }
            ImGui::NextColumn();

            return rv;
        }

        // draw an editor for one of the property's string values (might be a list)
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> drawIthEditor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // draw trash can that can delete an element from the property's list
            if (m_EditedProperty.isListProperty())
            {
                if (ImGui::Button(ICON_FA_TRASH))
                {
                    rv = MakePropElementDeleter<std::string>(idx);
                }
                ImGui::SameLine();
            }

            // read stored value from edited property
            //
            // care: optional properties have size==0, so perform a range check
            std::string value = idx < m_EditedProperty.size() ? m_EditedProperty.getValue(idx) : std::string{};

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (osc::InputString("##stringeditor", value))
            {
                // update the edited property - don't rely on ImGui to remember edits
                m_EditedProperty.setValue(idx, value);
            }

            // globally annotate the editor rect, for downstream screenshot automation
            osc::App::upd().addFrameAnnotation("ObjectPropertiesEditor::StringEditor/" + m_EditedProperty.getName(), osc::GetItemRect());

            if (osc::ItemValueShouldBeSaved())
            {
                rv = MakePropValueSetter<std::string>(idx, m_EditedProperty.getValue(idx));
            }

            return rv;
        }

        std::function<property_type const*()> m_Accessor;
        property_type m_OriginalProperty{"blank", true};
        property_type m_EditedProperty{"blank", true};
    };

    // concrete property editor for a simple double value
    class DoublePropertyEditor final : public VirtualPropertyEditor {
    public:
        using property_type = OpenSim::SimpleProperty<double>;

        DoublePropertyEditor(
            osc::EditorAPI*,
            std::shared_ptr<osc::UndoableModelStatePair const> const&,
            std::function<OpenSim::Object const*()> const&,
            std::function<property_type const*()> const& accessor_) :  // NOLINT(modernize-pass-by-value)

            m_Accessor{accessor_}
        {
        }
    private:
        std::type_info const& implGetTypeInfo() const final
        {
            return typeid(property_type);
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() final
        {
            property_type const* maybeProp = m_Accessor();
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

            ImGui::Separator();

            // draw name of the property in left-hand column
            DrawPropertyName(m_EditedProperty);
            ImGui::NextColumn();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < std::max(m_EditedProperty.size(), 1); ++idx)
            {
                ImGui::PushID(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editorRv = drawIthEditor(idx);
                ImGui::PopID();

                if (!rv)
                {
                    rv = std::move(editorRv);
                }
            }
            ImGui::NextColumn();

            return rv;
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> drawIthEditor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // draw trash can that can delete an element from the property's list
            if (m_EditedProperty.isListProperty())
            {
                if (ImGui::Button(ICON_FA_TRASH))
                {
                    rv = MakePropElementDeleter<double>(idx);
                }
                ImGui::SameLine();
            }

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

            // draw an invisible vertical line, so that `double` properties are properly
            // aligned with `Vec3` properties (that have a non-invisible R/G/B line)
            DrawColoredDimensionHintVerticalLine(osc::Color::clear());

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
                rv = MakePropValueSetter<double>(idx, m_EditedProperty.getValue(idx));
            }

            return rv;
        }

        std::function<property_type const*()> m_Accessor;
        property_type m_OriginalProperty{"blank", true};
        property_type m_EditedProperty{"blank", true};
        float m_StepSize = c_InitialStepSize;
    };

    // concrete property editor for a simple `bool` value
    class BoolPropertyEditor final : public VirtualPropertyEditor {
    public:
        using property_type = OpenSim::SimpleProperty<bool>;

        BoolPropertyEditor(
            osc::EditorAPI*,
            std::shared_ptr<osc::UndoableModelStatePair const> const&,
            std::function<OpenSim::Object const*()> const&,
            std::function<property_type const*()> const& accessor_) :  // NOLINT(modernize-pass-by-value)

            m_Accessor{accessor_}
        {
        }
    private:
        std::type_info const& implGetTypeInfo() const final
        {
            return typeid(property_type);
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() final
        {
            property_type const* maybeProp = m_Accessor();
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

            ImGui::Separator();

            // draw name of the property in left-hand column
            DrawPropertyName(m_EditedProperty);
            ImGui::NextColumn();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < std::max(m_EditedProperty.size(), 1); ++idx)
            {
                ImGui::PushID(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editorRv = drawIthEditor(idx);
                ImGui::PopID();

                if (!rv)
                {
                    rv = std::move(editorRv);
                }
            }
            ImGui::NextColumn();

            return rv;
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> drawIthEditor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // draw trash can that can delete an element from the property's list
            if (m_EditedProperty.isListProperty())
            {
                if (ImGui::Button(ICON_FA_TRASH))
                {
                    rv = MakePropElementDeleter<bool>(idx);
                }
                ImGui::SameLine();
            }

            // read stored value from edited property
            //
            // care: optional properties have size==0, so perform a range check
            bool value = idx < m_EditedProperty.size() ? m_EditedProperty.getValue(idx) : false;
            bool edited = false;

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::Checkbox("##booleditor", &value))
            {
                // update the edited property - don't rely on ImGui to remember edits
                m_EditedProperty.setValue(idx, value);
                edited = true;
            }

            // globally annotate the editor rect, for downstream screenshot automation
            osc::App::upd().addFrameAnnotation("ObjectPropertiesEditor::BoolEditor/" + m_EditedProperty.getName(), osc::GetItemRect());

            if (edited || osc::ItemValueShouldBeSaved())
            {
                rv = MakePropValueSetter<bool>(idx, m_EditedProperty.getValue(idx));
            }

            return rv;
        }

        std::function<property_type const*()> m_Accessor;
        property_type m_OriginalProperty{"blank", true};
        property_type m_EditedProperty{"blank", true};
    };

    // concrete property editor for a simple `Vec3` value
    class Vec3PropertyEditor final : public VirtualPropertyEditor {
    public:
        using property_type = OpenSim::SimpleProperty<SimTK::Vec3>;

        Vec3PropertyEditor(
            osc::EditorAPI*,
            std::shared_ptr<osc::UndoableModelStatePair const> const& model_,  // NOLINT(modernize-pass-by-value)
            std::function<OpenSim::Object const*()> const& objectAccessor_,  // NOLINT(modernize-pass-by-value)
            std::function<property_type const*()> const& accessor_) :  // NOLINT(modernize-pass-by-value)

            m_Model{model_},
            m_ObjectAccessor{objectAccessor_},
            m_Accessor{accessor_}
        {
        }
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

            glm::vec3 modelValueToEditedValue(glm::vec3 const& modelValue) const
            {
                return osc::ToVec3(static_cast<double>(m_ModelToEditedValueScaler) * (m_ModelToEditedTransform * osc::ToSimTKVec3(modelValue)));
            }

            glm::vec3 editedValueToModelValue(glm::vec3 const& editedValue) const
            {
                return osc::ToVec3(m_ModelToEditedTransform.invert() * osc::ToSimTKVec3(editedValue/m_ModelToEditedValueScaler));
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
            return osc::IsEqualCaseInsensitive(m_EditedProperty.getName(), "orientation");
        }

        // if the Vec3 property has a parent frame, returns a transform that maps the Vec3
        // property's value to ground
        std::optional<SimTK::Transform> getParentToGroundTransform() const
        {
            OpenSim::Object const* const obj = m_ObjectAccessor();
            if (!obj)
            {
                return std::nullopt;  // cannot find the property's parent object?
            }

            auto const* const component = dynamic_cast<OpenSim::Component const*>(obj);
            if (!component)
            {
                return std::nullopt;  // the object isn't an OpenSim component
            }

            auto const positionPropName = osc::TryGetPositionalPropertyName(*component);
            if (!positionPropName)
            {
                return std::nullopt;  // the component doesn't have a logical positional property that can be edited with the transform
            }

            OpenSim::Property<SimTK::Vec3> const* const prop = m_Accessor();
            if (!prop)
            {
                return std::nullopt;  // can't access the property this editor is ultimately editing
            }

            if (prop->getName() != *positionPropName)
            {
                return std::nullopt;  // the property this editor is editing isn't a logically positional one
            }

            std::optional<SimTK::Transform> transform = osc::TryGetParentToGroundTransform(*component, m_Model->getState());
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

            OpenSim::Model const& model = m_Model->getModel();
            auto const* frame = osc::FindComponent<OpenSim::Frame>(model, *m_MaybeUserSelectedFrameAbsPath);
            return frame->getTransformInGround(m_Model->getState()).invert();
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

        std::type_info const& implGetTypeInfo() const final
        {
            return typeid(property_type);
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() final
        {
            property_type const* maybeProp = m_Accessor();
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


            ImGui::Separator();

            // draw name of the property in left-hand column
            DrawPropertyName(m_EditedProperty);
            ImGui::NextColumn();

            // top line of right column shows "reexpress in" editor (if applicable)
            drawReexpressionEditorIfApplicable();

            // draw radians/degrees conversion toggle button (if applicable)
            drawDegreesToRadiansConversionToggle();

            // draw `[0, 1]` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < std::max(m_EditedProperty.size(), 1); ++idx)
            {
                ImGui::PushID(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editorRv = drawIthEditor(valueConverter, idx);
                ImGui::PopID();

                if (!rv)
                {
                    rv = std::move(editorRv);
                }
            }
            ImGui::NextColumn();

            return rv;
        }

        void drawReexpressionEditorIfApplicable()
        {
            if (!isPropertyExpressedWithinAParentFrame())
            {
                return;
            }

            osc::CStringView const defaultedLabel = "(parent frame)";
            std::string const preview = m_MaybeUserSelectedFrameAbsPath ?
                m_MaybeUserSelectedFrameAbsPath->getComponentName() :
                std::string{defaultedLabel};

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::BeginCombo("##reexpressioneditor", preview.c_str()))
            {
                ImGui::TextDisabled("Frame (editing)");
                ImGui::SameLine();
                osc::DrawHelpMarker("Note: this only affects the values that the quantities are edited in. It does not change the frame that the component is attached to. You can change the frame attachment by using the component's context menu: Socket > $FRAME > (edit button) > (select new frame)");
                ImGui::Dummy({0.0f, 0.25f*ImGui::GetTextLineHeight()});

                int imguiID = 0;

                // draw "default" (reset) option
                {
                    ImGui::Separator();
                    ImGui::PushID(imguiID++);
                    bool selected = !m_MaybeUserSelectedFrameAbsPath.has_value();
                    if (ImGui::Selectable(defaultedLabel.c_str(), &selected))
                    {
                        m_MaybeUserSelectedFrameAbsPath.reset();
                    }
                    ImGui::PopID();
                    ImGui::Separator();
                }

                // draw selectable for each frame in the model
                for (OpenSim::Frame const& frame : m_Model->getModel().getComponentList<OpenSim::Frame>())
                {
                    OpenSim::ComponentPath const frameAbsPath = osc::GetAbsolutePath(frame);

                    ImGui::PushID(imguiID++);
                    bool selected = frameAbsPath == m_MaybeUserSelectedFrameAbsPath;
                    if (ImGui::Selectable(frame.getName().c_str(), &selected))
                    {
                        m_MaybeUserSelectedFrameAbsPath = frameAbsPath;
                    }
                    ImGui::PopID();
                }

                ImGui::EndCombo();
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
                if (ImGui::Button(ICON_FA_TRASH))
                {
                    rv = MakePropElementDeleter<SimTK::Vec3>(idx);
                }
                ImGui::SameLine();
            }

            // read stored value from edited property
            //
            // care: optional properties have size==0, so perform a range check
            glm::vec3 const rawValue = osc::ToVec3(idx < m_EditedProperty.size() ? m_EditedProperty.getValue(idx) : SimTK::Vec3{});
            glm::vec3 const editedValue = valueConverter.modelValueToEditedValue(rawValue);

            // draw an editor for each component of the Vec3
            bool shouldSave = false;
            for (glm::vec3::length_type i = 0; i < 3; ++i)
            {
                ComponentEditorReturn const componentEditorRv = drawVec3ComponentEditor(idx, i, editedValue, valueConverter);
                shouldSave = shouldSave || componentEditorRv == ComponentEditorReturn::ShouldSave;
            }

            // if any component editor indicated that it should be saved then propagate that upwards
            if (shouldSave)
            {
                rv = MakePropValueSetter<SimTK::Vec3>(idx, m_EditedProperty.getValue(idx));
            }

            return rv;
        }

        enum class ComponentEditorReturn { None, ShouldSave };

        // draws float input for a single component of the Vec3 (e.g. vec.x)
        ComponentEditorReturn drawVec3ComponentEditor(
            int idx,
            glm::vec3::length_type i,
            glm::vec3 editedValue,
            ValueConverter const& valueConverter)
        {
            ImGui::PushID(i);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

            // draw dimension hint (color bar next to the input)
            DrawColoredDimensionHintVerticalLine(IthDimensionColor(i));

            // draw the input editor
            auto frameAnnotation = GenerateVecFrameAnnotationLabel(m_EditedProperty, i);
            auto drawRV = DrawCustomScalarInput("##valueinput", editedValue[i], m_StepSize, frameAnnotation);

            if (drawRV.wasEdited)
            {
                // un-convert the value on save
                glm::vec3 const savedValue = valueConverter.editedValueToModelValue(editedValue);
                m_EditedProperty.setValue(idx, osc::ToSimTKVec3(savedValue));
            }

            ImGui::PopID();

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
                if (ImGui::Button("radians"))
                {
                    m_OrientationValsAreInRadians = !m_OrientationValsAreInRadians;
                }
                osc::App::upd().addFrameAnnotation("ObjectPropertiesEditor::OrientationToggle/" + m_EditedProperty.getName(), osc::GetItemRect());
                osc::DrawTooltipBodyOnlyIfItemHovered("This quantity is edited in radians (click to switch to degrees)");
            }
            else
            {
                if (ImGui::Button("degrees"))
                {
                    m_OrientationValsAreInRadians = !m_OrientationValsAreInRadians;
                }
                osc::App::upd().addFrameAnnotation("ObjectPropertiesEditor::OrientationToggle/" + m_EditedProperty.getName(), osc::GetItemRect());
                osc::DrawTooltipBodyOnlyIfItemHovered("This quantity is edited in degrees (click to switch to radians)");
            }
        }

        std::shared_ptr<osc::UndoableModelStatePair const> m_Model;
        std::function<OpenSim::Object const*()> m_ObjectAccessor;
        std::function<property_type const*()> m_Accessor;
        property_type m_OriginalProperty{"blank", true};
        property_type m_EditedProperty{"blank", true};
        std::optional<OpenSim::ComponentPath> m_MaybeUserSelectedFrameAbsPath;
        float m_StepSize = c_InitialStepSize;
        bool m_OrientationValsAreInRadians = false;
    };

    // concrete property editor for a simple `Vec6` value
    class Vec6PropertyEditor final : public VirtualPropertyEditor {
    public:
        using property_type = OpenSim::SimpleProperty<SimTK::Vec6>;

        Vec6PropertyEditor(
            osc::EditorAPI*,
            std::shared_ptr<osc::UndoableModelStatePair const> const&,
            std::function<OpenSim::Object const*()> const&,
            std::function<property_type const*()> const& accessor_) :  // NOLINT(modernize-pass-by-value)

            m_Accessor{accessor_}
        {
        }
    private:
        std::type_info const& implGetTypeInfo() const final
        {
            return typeid(property_type);
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() final
        {
            property_type const* maybeProp = m_Accessor();
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

            ImGui::Separator();

            // draw name of the property in left-hand column
            DrawPropertyName(m_EditedProperty);
            ImGui::NextColumn();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < std::max(m_EditedProperty.size(), 1); ++idx)
            {
                ImGui::PushID(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editorRv = drawIthEditor(idx);
                ImGui::PopID();

                if (!rv)
                {
                    rv = std::move(editorRv);
                }
            }
            ImGui::NextColumn();

            return rv;
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> drawIthEditor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // draw trash can that can delete an element from the property's list
            if (m_EditedProperty.isListProperty())
            {
                if (ImGui::Button(ICON_FA_TRASH))
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
                ImGui::PushID(i);

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::InputFloat3("##vec6editor", rawValue.data() + static_cast<ptrdiff_t>(3*i), "%.6f"))
                {
                    m_EditedProperty.updValue(idx)[3*i + 0] = static_cast<double>(rawValue[3*i + 0]);
                    m_EditedProperty.updValue(idx)[3*i + 1] = static_cast<double>(rawValue[3*i + 1]);
                    m_EditedProperty.updValue(idx)[3*i + 2] = static_cast<double>(rawValue[3*i + 2]);
                }
                shouldSave = shouldSave || osc::ItemValueShouldBeSaved();
                osc::App::upd().addFrameAnnotation("ObjectPropertiesEditor::Vec6Editor/" + m_EditedProperty.getName(), osc::GetItemRect());

                ImGui::PopID();
            }

            if (shouldSave)
            {
                rv = MakePropValueSetter<SimTK::Vec6>(idx, m_EditedProperty.getValue(idx));
            }

            return rv;
        }

        std::function<property_type const*()> m_Accessor;
        property_type m_OriginalProperty{"blank", true};
        property_type m_EditedProperty{"blank", true};
    };

    class IntPropertyEditor final : public VirtualPropertyEditor {
    public:
        using property_type = OpenSim::SimpleProperty<int>;

        IntPropertyEditor(
            osc::EditorAPI*,
            std::shared_ptr<osc::UndoableModelStatePair const> const&,
            std::function<OpenSim::Object const*()> const&,
            std::function<property_type const*()> const& accessor_) :  // NOLINT(modernize-pass-by-value)

            m_Accessor{accessor_}
        {
        }

    private:
        std::type_info const& implGetTypeInfo() const final
        {
            return typeid(property_type);
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() final
        {
            property_type const* maybeProp = m_Accessor();
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

            ImGui::Separator();

            // draw name of the property in left-hand column
            DrawPropertyName(m_EditedProperty);
            ImGui::NextColumn();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < std::max(m_EditedProperty.size(), 1); ++idx)
            {
                ImGui::PushID(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editorRv = drawIthEditor(idx);
                ImGui::PopID();

                if (!rv)
                {
                    rv = std::move(editorRv);
                }
            }
            ImGui::NextColumn();

            return rv;
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> drawIthEditor(int idx)
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            // draw trash can that can delete an element from the property's list
            if (m_EditedProperty.isListProperty())
            {
                if (ImGui::Button(ICON_FA_TRASH))
                {
                    rv = MakePropElementDeleter<int>(idx);
                }
                ImGui::SameLine();
            }

            // read stored value from edited property
            //
            // care: optional properties have size==0, so perform a range check
            int value = idx < m_EditedProperty.size() ? m_EditedProperty.getValue(idx) : 0;
            bool edited = false;

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::InputInt("##inteditor", &value))
            {
                // update the edited property - don't rely on ImGui to remember edits
                m_EditedProperty.setValue(idx, value);
                edited = true;
            }

            // globally annotate the editor rect, for downstream screenshot automation
            osc::App::upd().addFrameAnnotation("ObjectPropertiesEditor::IntEditor/" + m_EditedProperty.getName(), osc::GetItemRect());

            if (edited || osc::ItemValueShouldBeSaved())
            {
                rv = MakePropValueSetter<int>(idx, m_EditedProperty.getValue(idx));
            }

            return rv;
        }

        std::function<property_type const*()> m_Accessor;
        property_type m_OriginalProperty{"blank", true};
        property_type m_EditedProperty{"blank", true};
    };
}

// concrete property editors for object types
namespace
{
    // concrete property editor for an OpenSim::Appearance
    class AppearancePropertyEditor final : public VirtualPropertyEditor {
    public:
        using property_type = OpenSim::ObjectProperty<OpenSim::Appearance>;

        AppearancePropertyEditor(
            osc::EditorAPI*,
            std::shared_ptr<osc::UndoableModelStatePair const> const&,
            std::function<OpenSim::Object const*()> const&,
            std::function<property_type const*()> const& accessor_) :  // NOLINT(modernize-pass-by-value)

            m_Accessor{accessor_}
        {
        }
    private:
        std::type_info const& implGetTypeInfo() const final
        {
            return typeid(property_type);
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() final
        {
            property_type const* maybeProp = m_Accessor();
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

            ImGui::Separator();

            // draw name of the property in left-hand column
            DrawPropertyName(m_EditedProperty);
            ImGui::NextColumn();

            // draw `n` editors in right-hand column
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;
            for (int idx = 0; idx < std::max(m_EditedProperty.size(), 1); ++idx)
            {
                ImGui::PushID(idx);
                std::optional<std::function<void(OpenSim::AbstractProperty&)>> editorRv = drawIthEditor(idx);
                ImGui::PopID();

                if (!rv)
                {
                    rv = std::move(editorRv);
                }
            }
            ImGui::NextColumn();

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

            osc::Color color = ToColor(m_EditedProperty.getValue());
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

            if (ImGui::ColorEdit4("##coloreditor", osc::ValuePtr(color)))
            {
                SimTK::Vec3 newColor;
                newColor[0] = static_cast<double>(color[0]);
                newColor[1] = static_cast<double>(color[1]);
                newColor[2] = static_cast<double>(color[2]);

                m_EditedProperty.updValue().set_color(newColor);
                m_EditedProperty.updValue().set_opacity(static_cast<double>(color[3]));
            }
            shouldSave = shouldSave || osc::ItemValueShouldBeSaved();

            bool isVisible = m_EditedProperty.getValue().get_visible();
            if (ImGui::Checkbox("is visible", &isVisible))
            {
                m_EditedProperty.updValue().set_visible(isVisible);
            }
            shouldSave = shouldSave || osc::ItemValueShouldBeSaved();

            if (shouldSave)
            {
                rv = MakePropValueSetter<OpenSim::Appearance>(idx, m_EditedProperty.getValue(idx));
            }

            return rv;
        }

        std::function<property_type const*()> m_Accessor;
        property_type m_OriginalProperty{"blank", true};
        property_type m_EditedProperty{"blank", true};
    };

    // concrete property editor for an `OpenSim::HuntCrossleyForce::ContactParametersSet`
    class ContactParameterSetEditor final : public VirtualPropertyEditor {
    public:
        using property_type = OpenSim::ObjectProperty<OpenSim::HuntCrossleyForce::ContactParametersSet>;

        ContactParameterSetEditor(
            osc::EditorAPI* api,
            std::shared_ptr<osc::UndoableModelStatePair const> const& targetModel_,  // NOLINT(modernize-pass-by-value)
            std::function<OpenSim::Object const*()> const&,
            std::function<property_type const*()> const& accessor_) :  // NOLINT(modernize-pass-by-value)

            m_API{api},
            m_TargetModel{targetModel_},
            m_Accessor{accessor_}
        {
        }
    private:
        std::type_info const& implGetTypeInfo() const final
        {
            return typeid(property_type);
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() final
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            property_type const* maybeProp = m_Accessor();
            if (!maybeProp)
            {
                return std::nullopt;  // cannot find property
            }
            property_type const& prop = *maybeProp;

            if (osc::empty(prop.getValue()))
            {
                return std::nullopt;  // no editable contact set on the property
            }

            OpenSim::HuntCrossleyForce::ContactParameters const& params = prop.getValue()[0];

            // update cached editors, if necessary
            if (!m_MaybeNestedEditor)
            {
                m_MaybeNestedEditor.emplace(m_API, m_TargetModel, [&params]() { return &params; });
            }
            osc::ObjectPropertiesEditor& nestedEditor = *m_MaybeNestedEditor;

            ImGui::Columns();
            auto resp = nestedEditor.onDraw();
            ImGui::Columns(2);

            if (resp)
            {
                // careful here: the response has a correct updater but doesn't know the full
                // path to the housing component, so we have to wrap the updater with
                // appropriate lookups etc

                rv = [=](OpenSim::AbstractProperty& p) mutable
                {
                    auto* downcasted = dynamic_cast<OpenSim::Property<OpenSim::HuntCrossleyForce::ContactParametersSet>*>(&p);
                    if (downcasted && !osc::empty(downcasted->getValue()))
                    {
                        OpenSim::HuntCrossleyForce::ContactParameters& contactParams = osc::At(downcasted->updValue(), 0);
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

        osc::EditorAPI* m_API;
        std::shared_ptr<osc::UndoableModelStatePair const> m_TargetModel;
        std::function<property_type const*()> m_Accessor;
        std::optional<osc::ObjectPropertiesEditor> m_MaybeNestedEditor;
    };

    // concrete property editor for an OpenSim::GeometryPath
    class GeometryPathPropertyEditor final : public VirtualPropertyEditor {
    public:
        using property_type = OpenSim::ObjectProperty<OpenSim::GeometryPath>;

        GeometryPathPropertyEditor(
            osc::EditorAPI* api_,
            std::shared_ptr<osc::UndoableModelStatePair const> const& targetModel_,  // NOLINT(modernize-pass-by-value)
            std::function<OpenSim::Object const*()> const&,
            std::function<property_type const*()> const& accessor_) :  // NOLINT(modernize-pass-by-value)

            m_API{api_},
            m_TargetModel{targetModel_},
            m_Accessor{accessor_}
        {
        }
    private:
        std::type_info const& implGetTypeInfo() const final
        {
            return typeid(property_type);
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implOnDraw() final
        {
            property_type const* maybeProp = m_Accessor();
            if (!maybeProp)
            {
                return std::nullopt;
            }
            property_type const& prop = *maybeProp;

            ImGui::Separator();
            DrawPropertyName(prop);
            ImGui::NextColumn();
            if (ImGui::Button(ICON_FA_EDIT))
            {
                m_API->pushPopup(createGeometryPathEditorPopup());
            }
            ImGui::NextColumn();


            if (*m_ReturnValueHolder)
            {
                std::optional<osc::ObjectPropertyEdit> edit;
                std::swap(*m_ReturnValueHolder, edit);
                return edit->getUpdater();
            }
            else
            {
                return std::nullopt;
            }
        }

        std::unique_ptr<osc::Popup> createGeometryPathEditorPopup()
        {
            return std::make_unique<osc::GeometryPathPropertyEditorPopup>(
                "Edit Geometry Path",
                m_TargetModel,
                m_Accessor,
                [rvHolder = m_ReturnValueHolder](osc::ObjectPropertyEdit edit) { *rvHolder = std::move(edit); }
            );
        }

        osc::EditorAPI* m_API;
        std::shared_ptr<osc::UndoableModelStatePair const> m_TargetModel;
        std::function<property_type const*()> m_Accessor;

        // shared between this property editor and a popup it may have spawned
        std::shared_ptr<std::optional<osc::ObjectPropertyEdit>> m_ReturnValueHolder = std::make_shared<std::optional<osc::ObjectPropertyEdit>>();
    };
}

// type-erased registry for all property editors
namespace
{
    // a registry containing all known property editors
    class PropertyEditorRegistry final {
    public:
        PropertyEditorRegistry()
        {
            registerEditor<StringPropertyEditor>();
            registerEditor<DoublePropertyEditor>();
            registerEditor<BoolPropertyEditor>();
            registerEditor<Vec3PropertyEditor>();
            registerEditor<Vec6PropertyEditor>();
            registerEditor<IntPropertyEditor>();
            registerEditor<AppearancePropertyEditor>();
            registerEditor<ContactParameterSetEditor>();
            registerEditor<GeometryPathPropertyEditor>();
        }

        std::unique_ptr<VirtualPropertyEditor> tryCreateEditor(
            osc::EditorAPI* editorAPI,
            std::shared_ptr<osc::UndoableModelStatePair const> const& targetModel,
            std::function<OpenSim::Object const*()> const& objectAccessor,
            std::function<OpenSim::AbstractProperty const*()> const& propertyAccessor) const
        {
            OpenSim::AbstractProperty const* maybeProp = propertyAccessor();
            if (!maybeProp)
            {
                return nullptr;  // cannot access the property
            }
            OpenSim::AbstractProperty const& prop = *maybeProp;

            auto const it = m_Lut.find(typeid(prop));
            if (it == m_Lut.end())
            {
                return nullptr;  // no property editor registered for the given typeid
            }

            // else: instantiate new property editor
            return it->second(editorAPI, targetModel, objectAccessor, propertyAccessor);
        }

    private:
        using TypeInfoRef = std::reference_wrapper<std::type_info const>;

        struct TypeInfoHasher final {
            size_t operator()(TypeInfoRef ref) const
            {
                return ref.get().hash_code();
            }
        };

        struct TypeInfoEqualTo final {
            bool operator()(TypeInfoRef lhs, TypeInfoRef rhs) const
            {
                return lhs.get() == rhs.get();
            }
        };

        using PropertyEditorCtor = std::function<std::unique_ptr<VirtualPropertyEditor>(
            osc::EditorAPI*,
            std::shared_ptr<osc::UndoableModelStatePair const> const&,
            std::function<OpenSim::Object const*()> const&,
            std::function<OpenSim::AbstractProperty const*()> const&
        )>;
        using PropertyEditorLUT = std::unordered_map<TypeInfoRef, PropertyEditorCtor, TypeInfoHasher, TypeInfoEqualTo>;

        template<typename TConcretePropertyEditor>
        void registerEditor()
        {
            m_Lut.insert(MakeRegistryEntry<TConcretePropertyEditor>());
        }

        template<typename TConcretePropertyEditor>
        static PropertyEditorLUT::value_type MakeRegistryEntry()
        {
            TypeInfoRef typeInfo = typeid(typename TConcretePropertyEditor::property_type);
            auto ctor = [](
                osc::EditorAPI* api,
                std::shared_ptr<osc::UndoableModelStatePair const> const& targetModel,
                std::function<OpenSim::Object const*()> const& objectAccessor,
                std::function<OpenSim::AbstractProperty const*()> const& propetyAccessor)
            {
                auto downcastedAccessor = [propetyAccessor]() -> typename TConcretePropertyEditor::property_type const*
                {
                    OpenSim::AbstractProperty const* genericProp = propetyAccessor();
                    if (genericProp)
                    {
                        return dynamic_cast<typename TConcretePropertyEditor::property_type const*>(genericProp);
                    }
                    else
                    {
                        return nullptr;
                    }
                };
                return std::make_unique<TConcretePropertyEditor>(api, targetModel, objectAccessor, downcastedAccessor);
            };
            return typename PropertyEditorLUT::value_type(typeInfo, ctor);
        }

        PropertyEditorLUT m_Lut;
    };

    // returns global registry of available property editors
    PropertyEditorRegistry const& GetGlobalPropertyEditorRegistry()
    {
        static PropertyEditorRegistry const s_Registry;
        return s_Registry;
    }
}

// top-level implementation of the properties editor
class osc::ObjectPropertiesEditor::Impl final {
public:
    Impl(
        EditorAPI* api_,
        std::shared_ptr<osc::UndoableModelStatePair const> targetModel_,
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

        ImGui::Columns(2);
        for (int i = 0; i < obj.getNumProperties(); ++i)
        {
            ImGui::PushID(i);
            std::optional<ObjectPropertyEdit> maybeEdit = tryDrawPropertyEditor(obj, obj.getPropertyByIndex(i));
            ImGui::PopID();

            if (maybeEdit)
            {
                rv = std::move(maybeEdit);
            }
        }
        ImGui::Columns();

        return rv;
    }

    // tries to draw one property editor for one property of an object
    std::optional<ObjectPropertyEdit> tryDrawPropertyEditor(
        OpenSim::Object const& obj,
        OpenSim::AbstractProperty const& prop)
    {
        if (osc::StartsWith(prop.getName(), "socket_"))
        {
            // #542: ignore properties that begin with `socket_`, because
            // they are proxy properties to the object's sockets and should
            // be manipulated via socket, rather than property, editors
            return std::nullopt;
        }
        else if (VirtualPropertyEditor* maybeEditor = tryGetPropertyEditor(prop))
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
        VirtualPropertyEditor& editor)
    {
        ImGui::PushID(prop.getName().c_str());
        std::optional<std::function<void(OpenSim::AbstractProperty&)>> maybeUpdater = editor.onDraw();
        ImGui::PopID();

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
        ImGui::Separator();
        DrawPropertyName(prop);
        ImGui::NextColumn();
        ImGui::TextUnformatted(prop.toString().c_str());
        ImGui::NextColumn();
    }

    // try get/construct a property editor for the given property
    VirtualPropertyEditor* tryGetPropertyEditor(OpenSim::AbstractProperty const& prop)
    {
        auto const [it, inserted] = m_PropertyEditorsByName.try_emplace(prop.getName(), nullptr);

        if (inserted || (it->second && !it->second->isCompatibleWith(prop)))
        {
            // need to create a new editor because either it hasn't been made yet or the existing
            // editor is for a different type

            // wrap property accesses via the object accessor so that they can be runtime-checked
            std::function<OpenSim::AbstractProperty const*()> propertyAccessor = MakePropertyAccessor(m_ObjectGetter, prop.getName());
            it->second = GetGlobalPropertyEditorRegistry().tryCreateEditor(m_API, m_TargetModel, m_ObjectGetter, propertyAccessor);
        }

        return it->second.get();
    }

    EditorAPI* m_API;
    std::shared_ptr<osc::UndoableModelStatePair const> m_TargetModel;
    std::function<OpenSim::Object const*()> m_ObjectGetter;
    OpenSim::Object const* m_PreviousObject = nullptr;
    std::unordered_map<std::string, std::unique_ptr<VirtualPropertyEditor>> m_PropertyEditorsByName;
};


// public API (ObjectPropertiesEditor)

osc::ObjectPropertiesEditor::ObjectPropertiesEditor(
    EditorAPI* api_,
    std::shared_ptr<osc::UndoableModelStatePair const> targetModel_,
    std::function<OpenSim::Object const*()> objectGetter_) :

    m_Impl{std::make_unique<Impl>(api_, std::move(targetModel_), std::move(objectGetter_))}
{
}

osc::ObjectPropertiesEditor::ObjectPropertiesEditor(ObjectPropertiesEditor&&) noexcept = default;
osc::ObjectPropertiesEditor& osc::ObjectPropertiesEditor::operator=(ObjectPropertiesEditor&&) noexcept = default;
osc::ObjectPropertiesEditor::~ObjectPropertiesEditor() noexcept = default;

std::optional<osc::ObjectPropertyEdit> osc::ObjectPropertiesEditor::onDraw()
{
    return m_Impl->onDraw();
}
