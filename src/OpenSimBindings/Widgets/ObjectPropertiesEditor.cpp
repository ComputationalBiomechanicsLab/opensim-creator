#include "ObjectPropertiesEditor.hpp"

#include "osc_config.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Maths/Constants.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/SimTKHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/Algorithms.hpp"

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
#include <OpenSim/Simulation/Model/HuntCrossleyForce.h>
#include <SimTKcommon/Constants.h>
#include <SimTKcommon/SmallMatrix.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <utility>

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

    // extract linear RGBA values from an OpenSim::Appearance
    glm::vec4 ExtractRgba(OpenSim::Appearance const& appearance)
    {
        SimTK::Vec3 const rgb = appearance.get_color();
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
        std::string propertyName)
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

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> draw()
        {
            return implDraw();
        }

    private:
        virtual std::type_info const& implGetTypeInfo() const = 0;
        virtual std::optional<std::function<void(OpenSim::AbstractProperty&)>> implDraw() = 0;
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
            std::shared_ptr<osc::UndoableModelStatePair const>,
            std::function<property_type const* ()> accessor_) :

            m_Accessor{std::move(accessor_)}
        {
        }
    private:
        std::type_info const& implGetTypeInfo() const final
        {
            return typeid(property_type);
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implDraw() final
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
            if (osc::InputString("##stringeditor", value, 128))
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
            std::shared_ptr<osc::UndoableModelStatePair const>,
            std::function<property_type const*()> accessor_) :

            m_Accessor{std::move(accessor_)}
        {
        }
    private:
        std::type_info const& implGetTypeInfo() const final
        {
            return typeid(property_type);
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implDraw() final
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

            // read stored value from edited property
            //
            // care: optional properties have size==0, so perform a range check
            float value = static_cast<float>(idx < m_EditedProperty.size() ? m_EditedProperty.getValue(idx) : 0.0);

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::InputFloat("##doubleeditor", &value, 0.0f, 0.0f, OSC_DEFAULT_FLOAT_INPUT_FORMAT))
            {
                // update the edited property - don't rely on ImGui to remember edits
                m_EditedProperty.setValue(idx, static_cast<double>(value));
            }

            // globally annotate the editor rect, for downstream screenshot automation
            osc::App::upd().addFrameAnnotation("ObjectPropertiesEditor::DoubleEditor/" + m_EditedProperty.getName(), osc::GetItemRect());

            if (osc::ItemValueShouldBeSaved())
            {
                rv = MakePropValueSetter<double>(idx, m_EditedProperty.getValue(idx));
            }

            return rv;
        }

        std::function<property_type const*()> m_Accessor;
        property_type m_OriginalProperty{"blank", true};
        property_type m_EditedProperty{"blank", true};
    };

    // concrete property editor for a simple `bool` value
    class BoolPropertyEditor final : public VirtualPropertyEditor {
    public:
        using property_type = OpenSim::SimpleProperty<bool>;

        BoolPropertyEditor(
            osc::EditorAPI*,
            std::shared_ptr<osc::UndoableModelStatePair const>,
            std::function<property_type const*()> accessor_) :

            m_Accessor{std::move(accessor_)}
        {
        }
    private:
        std::type_info const& implGetTypeInfo() const final
        {
            return typeid(property_type);
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implDraw() final
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
            std::shared_ptr<osc::UndoableModelStatePair const>,
            std::function<property_type const*()> accessor_) :

            m_Accessor{std::move(accessor_)}
        {
        }
    private:
        std::type_info const& implGetTypeInfo() const final
        {
            return typeid(property_type);
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implDraw() final
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
                    rv = MakePropElementDeleter<SimTK::Vec3>(idx);
                }
                ImGui::SameLine();
            }

            // read stored value from edited property
            //
            // care: optional properties have size==0, so perform a range check
            glm::vec3 rawValue = osc::ToVec3(idx < m_EditedProperty.size() ? m_EditedProperty.getValue(idx) : SimTK::Vec3{});

            // draw button that converts the displayed value for editing (e.g. between radians and degrees)
            float const conversionCoefficient = drawValueConversionToggle();
            rawValue *= conversionCoefficient;

            // draw an editor for each component of the vec3
            bool shouldSave = false;
            for (glm::vec3::length_type i = 0; i < 3; ++i)
            {
                ImGui::PushID(i);
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

                // draw dimension hint (color bar next to the input)
                {
                    glm::vec4 color = {0.0f, 0.0f, 0.0f, 0.6f};
                    color[i] = 1.0f;

                    ImDrawList* const l = ImGui::GetWindowDrawList();
                    glm::vec2 const p = ImGui::GetCursorScreenPos();
                    float const h = ImGui::GetTextLineHeight() + 2.0f*ImGui::GetStyle().FramePadding.y + 2.0f*ImGui::GetStyle().FrameBorderSize;
                    glm::vec2 const dims = glm::vec2{4.0f, h};
                    l->AddRectFilled(p, p + dims, ImGui::ColorConvertFloat4ToU32(color));
                    ImGui::SetCursorScreenPos({p.x + 4.0f, p.y});
                }

                // draw the input editor
                ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, {1.0f, 0.0f});
                if (ImGui::InputScalar("##valueinput", ImGuiDataType_Float, &rawValue[i], &m_StepSize, nullptr, OSC_DEFAULT_FLOAT_INPUT_FORMAT))
                {
                    // un-convert the value on save
                    glm::vec3 const savedValue = (1.0f/conversionCoefficient) * rawValue;
                    m_EditedProperty.setValue(idx, osc::ToSimTKVec3(savedValue));
                }
                ImGui::PopStyleVar();
                shouldSave = shouldSave || osc::ItemValueShouldBeSaved();

                // globally annotate the editor rect, for downstream screenshot automation
                {
                    std::stringstream annotation;
                    annotation << "ObjectPropertiesEditor::Vec3/";
                    annotation << i;
                    annotation << '/';
                    annotation << m_EditedProperty.getName();
                    osc::App::upd().addFrameAnnotation(std::move(annotation).str(), osc::GetItemRect());
                }
                osc::DrawTooltipIfItemHovered("Step Size", "You can right-click to adjust the step size of the buttons");

                // draw a context menu that lets the user "step" the value with a button
                drawStepSizeContextMenu();

                ImGui::PopID();
            }

            if (shouldSave)
            {
                rv = MakePropValueSetter<SimTK::Vec3>(idx, m_EditedProperty.getValue(idx));
            }

            return rv;
        }

        // draws a unit converter toggle button and returns the effective conversion ratio that is
        // initated by the button
        float drawValueConversionToggle()
        {
            if (osc::IsEqualCaseInsensitive(m_EditedProperty.getName(), "orientation"))
            {
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

                return m_OrientationValsAreInRadians ? 1.0f : static_cast<float>(SimTK_RADIAN_TO_DEGREE);
            }
            else
            {
                return 1.0f;
            }
        }

        // draws a context menu that the user can use to change the step size of the +/- buttons
        void drawStepSizeContextMenu()
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
                    ImGui::InputFloat("##stepsizeinput", &m_StepSize, 0.0f, 0.0f, OSC_DEFAULT_FLOAT_INPUT_FORMAT);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Lengths");
                    ImGui::TableSetColumnIndex(1);
                    if (ImGui::Button("10 cm"))
                    {
                        m_StepSize = 0.1f;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("1 cm"))
                    {
                        m_StepSize = 0.01f;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("1 mm"))
                    {
                        m_StepSize = 0.001f;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("0.1 mm"))
                    {
                        m_StepSize = 0.0001f;
                    }

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Angles (Degrees)");
                    ImGui::TableSetColumnIndex(1);
                    if (ImGui::Button("180"))
                    {
                        m_StepSize = 180.0f;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("90"))
                    {
                        m_StepSize = 90.0f;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("45"))
                    {
                        m_StepSize = 45.0f;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("10"))
                    {
                        m_StepSize = 10.0f;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("1"))
                    {
                        m_StepSize = 1.0f;
                    }

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Angles (Radians)");
                    ImGui::TableSetColumnIndex(1);
                    if (ImGui::Button("1 pi"))
                    {
                        m_StepSize = osc::fpi;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("1/2 pi"))
                    {
                        m_StepSize = osc::fpi2;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("1/4 pi"))
                    {
                        m_StepSize = osc::fpi4;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("10/180 pi"))
                    {
                        m_StepSize = (10.0f/180.0f) * osc::fpi;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("1/180 pi"))
                    {
                        m_StepSize = (1.0f/180.0f) * osc::fpi;
                    }

                    ImGui::EndTable();
                }

                ImGui::EndPopup();
            }
        }

        std::function<property_type const*()> m_Accessor;
        property_type m_OriginalProperty{"blank", true};
        property_type m_EditedProperty{"blank", true};
        float m_StepSize = 0.001f;
        bool m_OrientationValsAreInRadians = false;
    };

    // concrete property editor for a simple `Vec6` value
    class Vec6PropertyEditor final : public VirtualPropertyEditor {
    public:
        using property_type = OpenSim::SimpleProperty<SimTK::Vec6>;

        Vec6PropertyEditor(
            osc::EditorAPI*,
            std::shared_ptr<osc::UndoableModelStatePair const>,
            std::function<property_type const*()> accessor_) :

            m_Accessor{std::move(accessor_)}
        {
        }
    private:
        std::type_info const& implGetTypeInfo() const final
        {
            return typeid(property_type);
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implDraw() final
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
                if (ImGui::InputFloat3("##vec6editor", rawValue.data() + 3*i, OSC_DEFAULT_FLOAT_INPUT_FORMAT))
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
            std::shared_ptr<osc::UndoableModelStatePair const>,
            std::function<property_type const*()> accessor_) :

            m_Accessor{std::move(accessor_)}
        {
        }
    private:
        std::type_info const& implGetTypeInfo() const final
        {
            return typeid(property_type);
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implDraw() final
        {
            property_type const* maybeProp = m_Accessor();
            if (!maybeProp)
            {
                return std::nullopt;
            }
            property_type const& prop = *maybeProp;

            if (prop.isListProperty())
            {
                return std::nullopt;  // HACK: ignore list props for now
            }

            if (prop.size() == 0)
            {
                return std::nullopt;  // HACK: ignore optional props for now
            }

            ImGui::Separator();

            // draw name of the property in left-hand column
            DrawPropertyName(prop);
            ImGui::NextColumn();

            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv = std::nullopt;
            glm::vec4 color = ExtractRgba(prop.getValue());

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

            ImGui::PushID(1);
            if (ImGui::ColorEdit4("##coloreditor", glm::value_ptr(color)))
            {
                SimTK::Vec3 newColor;
                newColor[0] = static_cast<double>(color[0]);
                newColor[1] = static_cast<double>(color[1]);
                newColor[2] = static_cast<double>(color[2]);

                OpenSim::Appearance newAppearance{prop.getValue()};
                newAppearance.set_color(newColor);
                newAppearance.set_opacity(static_cast<double>(color[3]));

                rv = MakePropValueSetter<OpenSim::Appearance>(0, newAppearance);
            }
            ImGui::PopID();

            bool isVisible = prop.getValue().get_visible();
            ImGui::PushID(2);
            if (ImGui::Checkbox("is visible", &isVisible))
            {
                OpenSim::Appearance newAppearance{prop.getValue()};
                newAppearance.set_visible(isVisible);

                rv = MakePropValueSetter<OpenSim::Appearance>(0, newAppearance);
            }
            ImGui::PopID();

            ImGui::NextColumn();

            return rv;
        }

        std::function<property_type const*()> m_Accessor;
    };

    // concrete property editor for an `OpenSim::HuntCrossleyForce::ContactParametersSet`
    class ContactParameterSetEditor final : public VirtualPropertyEditor {
    public:
        using property_type = OpenSim::ObjectProperty<OpenSim::HuntCrossleyForce::ContactParametersSet>;

        ContactParameterSetEditor(
            osc::EditorAPI* api,
            std::shared_ptr<osc::UndoableModelStatePair const> targetModel_,
            std::function<property_type const*()> accessor_) :

            m_API{api},
            m_TargetModel{std::move(targetModel_)},
            m_Accessor{std::move(accessor_)}
        {
        }
    private:
        std::type_info const& implGetTypeInfo() const final
        {
            return typeid(property_type);
        }

        std::optional<std::function<void(OpenSim::AbstractProperty&)>> implDraw() final
        {
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> rv;

            property_type const* maybeProp = m_Accessor();
            if (!maybeProp)
            {
                return std::nullopt;  // cannot find property
            }
            property_type const& prop = *maybeProp;

            if (prop.getValue().getSize() <= 0)
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
            auto resp = nestedEditor.draw();
            ImGui::Columns(2);

            if (resp)
            {
                // careful here: the response has a correct updater but doesn't know the full
                // path to the housing component, so we have to wrap the updater with
                // appropriate lookups etc

                rv = [=](OpenSim::AbstractProperty& p) mutable
                {
                    auto* downcasted = dynamic_cast<OpenSim::Property<OpenSim::HuntCrossleyForce::ContactParametersSet>*>(&p);
                    if (downcasted && downcasted->getValue().getSize() > 0)
                    {
                        OpenSim::HuntCrossleyForce::ContactParameters& contactParams = downcasted->getValue()[0];
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
            registerEditor<AppearancePropertyEditor>();
            registerEditor<ContactParameterSetEditor>();
        }

        std::unique_ptr<VirtualPropertyEditor> tryCreateEditor(
            osc::EditorAPI* editorAPI,
            std::shared_ptr<osc::UndoableModelStatePair const> targetModel,
            std::function<OpenSim::AbstractProperty const*()> genericAccessor) const
        {
            OpenSim::AbstractProperty const* maybeProp = genericAccessor();
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
            return it->second(editorAPI, targetModel, genericAccessor);
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

        using PropertyEditorCtor = std::function<std::unique_ptr<VirtualPropertyEditor>(osc::EditorAPI*, std::shared_ptr<osc::UndoableModelStatePair const>, std::function<OpenSim::AbstractProperty const*()>)>;
        using PropertyEditorLUT = std::unordered_map<TypeInfoRef, PropertyEditorCtor, TypeInfoHasher, TypeInfoEqualTo>;

        template<typename TConcretePropertyEditor>
        void registerEditor()
        {
            m_Lut.insert(MakeRegistryEntry<TConcretePropertyEditor>());
        }

        template<typename TConcretePropertyEditor>
        static PropertyEditorLUT::value_type MakeRegistryEntry()
        {
            TypeInfoRef typeInfo = typeid(TConcretePropertyEditor::property_type);
            auto ctor = [](osc::EditorAPI* api, std::shared_ptr<osc::UndoableModelStatePair const> targetModel, std::function<OpenSim::AbstractProperty const*()> genericAccessor)
            {
                auto downcastedAccessor = [genericAccessor]() -> TConcretePropertyEditor::property_type const*
                {
                    OpenSim::AbstractProperty const* genericProp = genericAccessor();
                    if (genericProp)
                    {
                        return dynamic_cast<TConcretePropertyEditor::property_type const*>(genericProp);
                    }
                    else
                    {
                        return nullptr;
                    }
                };
                return std::make_unique<TConcretePropertyEditor>(api, targetModel, downcastedAccessor);
            };
            return PropertyEditorLUT::value_type(typeInfo, ctor);
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

    // draw all property editors for the given object
    std::optional<ObjectPropertyEdit> draw()
    {
        if (OpenSim::Object const* maybeObj = m_ObjectGetter())
        {
            return draw(*maybeObj);
        }
        else
        {
            return std::nullopt;  // object inaccessible: don't show editors
        }
    }
private:

    std::optional<ObjectPropertyEdit> draw(OpenSim::Object const& obj)
    {
        // if necessary, update cached property editors
        if (m_PreviousObject != &obj)
        {
            m_PropertyEditorsByName.clear();
            m_PreviousObject = &obj;
        }

        // draw each editor and return the last property edit (or std::nullopt)
        std::optional<ObjectPropertyEdit> rv;

        ImGui::Columns(2);
        for (int i = 0; i < obj.getNumProperties(); ++i)
        {
            ImGui::PushID(i);
            if (std::optional<ObjectPropertyEdit> maybeEdit = drawPropertyEditor(obj, obj.getPropertyByIndex(i)))
            {
                rv = std::move(maybeEdit);
            }
            ImGui::PopID();
        }
        ImGui::Columns();

        return rv;
    }

    // draw a single property editor for one property of an object
    std::optional<ObjectPropertyEdit> drawPropertyEditor(
        OpenSim::Object const& obj,
        OpenSim::AbstractProperty const& prop)
    {
        std::optional<ObjectPropertyEdit> rv;

        // #542: ignore properties that begin with `socket_`, because they are
        // proxy properties to the object's sockets (which should be manipulated
        // via socket, rather than text, editors)
        if (osc::StartsWith(prop.getName(), "socket_"))
        {
            return rv;
        }

        // three cases:
        //
        // - use existing active property editor
        // - create a new active property editor
        // - there's no property editor available for the given type, so show a not-editable string

        auto const [it, inserted] = m_PropertyEditorsByName.try_emplace(prop.getName(), nullptr);
        if (inserted || (it->second && !it->second->isCompatibleWith(prop)))
        {
            // need to create a new editor because either it hasn't been made yet or the existing
            // editor is for a different type

            // wrap property accesses via the object accessor so that they can be runtime-checked
            std::function<OpenSim::AbstractProperty const*()> accessor = MakePropertyAccessor(m_ObjectGetter, prop.getName());
            it->second = GetGlobalPropertyEditorRegistry().tryCreateEditor(m_API, m_TargetModel, accessor);
        }

        if (it->second)
        {
            // there is an editor, so draw it etc.
            ImGui::PushID(prop.getName().c_str());
            std::optional<std::function<void(OpenSim::AbstractProperty&)>> maybeUpdater = it->second->draw();
            ImGui::PopID();
            if (maybeUpdater)
            {
                rv.emplace(ObjectPropertyEdit{obj, prop, std::move(maybeUpdater).value()});
            }
        }
        else
        {
            // no editor available for this type
            ImGui::Separator();
            DrawPropertyName(prop);
            ImGui::NextColumn();
            ImGui::TextUnformatted(prop.toString().c_str());
            ImGui::NextColumn();
        }

        return rv;
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

std::optional<osc::ObjectPropertyEdit> osc::ObjectPropertiesEditor::draw()
{
    return m_Impl->draw();
}