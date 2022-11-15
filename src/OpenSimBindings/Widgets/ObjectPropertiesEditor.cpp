#include "ObjectPropertiesEditor.hpp"

#include "osc_config.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/Algorithms.hpp"

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

// returns the first value that changed between the first `n` elements of `old` and `newer`
template<typename Coll1, typename Coll2>
static float diff(Coll1 const& old, Coll2 const& newer, size_t n)
{
    for (int i = 0; i < static_cast<int>(n); ++i)
    {
        if (static_cast<float>(old[i]) != static_cast<float>(newer[i]))
        {
            return newer[i];
        }
    }
    return static_cast<float>(old[0]);
}

// returns an updater function that deletes an element from a list property
template<typename T>
static std::function<void(OpenSim::AbstractProperty&)> MakePropElementDeleter(int idx)
{
    return [idx](OpenSim::AbstractProperty& p)
    {
        auto* ps = dynamic_cast<OpenSim::SimpleProperty<T>*>(&p);
        if (!ps)
        {
            return;
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
static std::function<void(OpenSim::AbstractProperty&)> MakePropValueSetter(int idx, T value)
{
    return [idx, value](OpenSim::AbstractProperty& p)
    {
        auto* ps = dynamic_cast<OpenSim::Property<T>*>(&p);
        if (!ps)
        {
            return;
        }
        ps->setValue(idx, value);
    };
}

// returns an updater function that sets the value of a property
template<typename T>
static std::function<void(OpenSim::AbstractProperty&)> MakePropValueSetter(T value)
{
    return [value](OpenSim::AbstractProperty& p)
    {
        auto* ps = dynamic_cast<OpenSim::Property<T>*>(&p);
        if (!ps)
        {
            return;
        }
        ps->setValue(value);
    };
}

static bool ItemValueShouldBeSaved()
{
    return ImGui::IsItemDeactivatedAfterEdit() ||
            (ImGui::IsItemEdited() && osc::IsAnyKeyPressed({ImGuiKey_Enter, ImGuiKey_Tab}));
}

using UpdateFn = std::function<void(OpenSim::AbstractProperty&)>;

static void DrawIthStringEditor(
        OpenSim::SimpleProperty<std::string> const& prop,
        int idx,
        std::optional<UpdateFn>& rv)
{
    if (prop.getMaxListSize() > 1)
    {
        if (ImGui::Button(ICON_FA_TRASH) && !rv)
        {
            rv = MakePropElementDeleter<std::string>(idx);
        }
        ImGui::SameLine();
    }

    // care: optional values can have size==0 but can have a value later
    //                assigned to that slot
    std::string curValue = prop.size() <= idx ? "" : prop.getValue(idx);

    bool edited = false;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (osc::InputString("##stringeditor", curValue, 128))
    {
        edited = true;
    }
    osc::App::upd().addFrameAnnotation("ObjectPropertiesEditor::StringEditor/" + prop.getName(), osc::GetItemRect());

    bool shouldUpdate = edited && !rv && ItemValueShouldBeSaved();
    if (shouldUpdate)
    {
        rv = MakePropValueSetter<std::string>(idx, curValue);
    }
}

// draw property editor for a single `double` value
static void Draw1DoubleValueEditor(
        OpenSim::SimpleProperty<double> const& prop,
        std::optional<UpdateFn>& rv)
{
    if (prop.size() != 1 || prop.isListProperty())
    {
        return;
    }

    float fv = static_cast<float>(prop.getValue());

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    if (ImGui::InputFloat("##doubleditor", &fv, 0.0f, 0.0f, OSC_DEFAULT_FLOAT_INPUT_FORMAT) && ItemValueShouldBeSaved())
    {
        double dv = static_cast<double>(fv);
        rv = MakePropValueSetter<double>(dv);
    }
    osc::App::upd().addFrameAnnotation("ObjectPropertiesEditor::DoubleEditor/" + prop.getName(), osc::GetItemRect());
}

// draw a property editor for 2 double values in a list
static void Draw2DoubleValueEditor(
        OpenSim::SimpleProperty<double> const& prop,
        std::optional<UpdateFn>& rv)
{
    if (prop.size() != 2)
    {
        return;
    }

    std::array<float, 2> vs =
    {
        static_cast<float>(prop.getValue(0)),
        static_cast<float>(prop.getValue(1))
    };

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::InputFloat2("##vec2editor", vs.data(), OSC_DEFAULT_FLOAT_INPUT_FORMAT) && ItemValueShouldBeSaved())
    {
        rv = [vs](OpenSim::AbstractProperty& p)
        {
            auto* pd = dynamic_cast<OpenSim::Property<double>*>(&p);
            if (!pd)
            {
                // ERROR: wrong type passed into the updater at runtime
                return;
            }
            pd->setValue(0, static_cast<double>(vs[0]));
            pd->setValue(1, static_cast<double>(vs[1]));
        };
    }
    osc::App::upd().addFrameAnnotation("ObjectPropertiesEditor::2DoubleEditor/" + prop.getName(), osc::GetItemRect());
}


static void DrawPropertyName(OpenSim::AbstractProperty const& prop)
{
    ImGui::TextUnformatted(prop.getName().c_str());
    std::string const& comment = prop.getComment();
    if (!comment.empty())
    {
        ImGui::SameLine();
        osc::DrawHelpMarker(prop.getComment().c_str());
    }
}

static std::string GetAbsPathOrEmptyIfNotAComponent(OpenSim::Object const& obj)
{
    if (auto c = dynamic_cast<OpenSim::Component const*>(&obj))
    {
        return c->getAbsolutePathString();
    }
    else
    {
        return {};
    }
}

namespace
{
    // type-erased property editor
    //
    // *must* be called with the correct type (use the typeids as registry keys)
    class PropertyEditor {
    public:
        virtual ~PropertyEditor() noexcept = default;

        bool isEditorFor(OpenSim::AbstractProperty const& prop)
        {
            return isEditorForImpl(prop);
        }

        std::optional<UpdateFn> draw(OpenSim::AbstractProperty const& prop)
        {
            return drawImpl(prop);
        }

    private:
        virtual std::optional<UpdateFn> drawImpl(OpenSim::AbstractProperty const&) = 0;
        virtual bool isEditorForImpl(OpenSim::AbstractProperty const&) = 0;
    };

    // typed property editor
    //
    // performs runtime downcasting, but the caller *must* ensure it calls with the
    // correct type
    template<typename T>
    class PropertyEditorT : public PropertyEditor {
    public:
        using PropertyType = T;

    private:
        virtual std::optional<UpdateFn> drawDowncastedImpl(PropertyType const& prop) = 0;

        std::optional<UpdateFn> drawImpl(OpenSim::AbstractProperty const& abstractProp) override final
        {
            OSC_ASSERT(typeid(abstractProp) == typeid(PropertyType));
            return drawDowncastedImpl(static_cast<PropertyType const&>(abstractProp));
        }

        bool isEditorForImpl(OpenSim::AbstractProperty const& prop) override
        {
            return typeid(prop) == typeid(PropertyType);
        }
    };

    class StringPropertyEditor final : public PropertyEditorT<OpenSim::SimpleProperty<std::string>> {
    private:
        std::optional<UpdateFn> drawDowncastedImpl(PropertyType const& prop) override
        {
            DrawPropertyName(prop);
            ImGui::NextColumn();

            std::optional<UpdateFn> rv = std::nullopt;

            int nEls = std::max(prop.size(), 1);  // care: optional properties have size==0

            for (int idx = 0; idx < nEls; ++idx)
            {
                ImGui::PushID(idx);
                DrawIthStringEditor(prop, idx, rv);
                ImGui::PopID();
            }

            ImGui::NextColumn();

            return rv;
        }
    };

    class DoublePropertyEditor final : public PropertyEditorT<OpenSim::SimpleProperty<double>> {
    private:
        std::optional<UpdateFn> drawDowncastedImpl(PropertyType const& prop) override
        {
            DrawPropertyName(prop);
            ImGui::NextColumn();

            std::optional<UpdateFn> rv = std::nullopt;

            if (!prop.isListProperty() && prop.size() == 0)
            {
                // uh
            }
            else if (!prop.isListProperty() && prop.size() == 1)
            {
                Draw1DoubleValueEditor(prop, rv);
            }
            else if (prop.size() == 2)
            {
                Draw2DoubleValueEditor(prop, rv);
            }
            else
            {
                ImGui::Text("%s", prop.toString().c_str());
            }

            ImGui::NextColumn();

            return rv;
        }
    };

    class BoolPropertyEditor final : public PropertyEditorT<OpenSim::SimpleProperty<bool>> {
    private:
        std::optional<UpdateFn> drawDowncastedImpl(PropertyType const& prop) override
        {
            DrawPropertyName(prop);
            ImGui::NextColumn();

            std::optional<UpdateFn> rv = std::nullopt;

            if (prop.isListProperty())
            {
                ImGui::Text("%s", prop.toString().c_str());
                ImGui::NextColumn();
                return rv;
            }

            bool v = prop.getValue();
            if (ImGui::Checkbox("##booleditor", &v))
            {
                ImGui::NextColumn();
                return MakePropValueSetter<bool>(v);
            }
            ImGui::NextColumn();

            return rv;
        }
    };

    class Vec3PropertyEditor final : public PropertyEditorT<OpenSim::SimpleProperty<SimTK::Vec3>> {
    private:
        std::optional<UpdateFn> drawDowncastedImpl(PropertyType const& prop) override
        {
            DrawPropertyName(prop);
            ImGui::NextColumn();

            std::optional<UpdateFn> rv = std::nullopt;

            if (prop.isListProperty())
            {
                ImGui::Text("%s", prop.toString().c_str());
                ImGui::NextColumn();
                return rv;
            }

            SimTK::Vec3 originalValue = prop.getValue();

            double conversionCoefficient = 1.0;

            // HACK: provide auto-converters for angular quantities
            if (osc::IsEqualCaseInsensitive(prop.getName(), "orientation"))
            {
                if (m_OrientationValsAreInRadians)
                {
                    if (ImGui::Button("radians"))
                    {
                        m_OrientationValsAreInRadians = !m_OrientationValsAreInRadians;
                    }
                    osc::App::upd().addFrameAnnotation("ObjectPropertiesEditor::OrientationToggle/" + prop.getName(), osc::GetItemRect());
                    osc::DrawTooltipBodyOnlyIfItemHovered("This quantity is edited in radians (click to switch to degrees)");
                }
                else
                {
                    if (ImGui::Button("degrees"))
                    {
                        m_OrientationValsAreInRadians = !m_OrientationValsAreInRadians;
                    }
                    osc::App::upd().addFrameAnnotation("ObjectPropertiesEditor::OrientationToggle/" + prop.getName(), osc::GetItemRect());
                    osc::DrawTooltipBodyOnlyIfItemHovered("This quantity is edited in degrees (click to switch to radians)");
                }

                conversionCoefficient = m_OrientationValsAreInRadians ? 1.0 : SimTK_RADIAN_TO_DEGREE;
            }

            std::array<float, 3> fv =
            {
                static_cast<float>(conversionCoefficient * originalValue[0]),
                static_cast<float>(conversionCoefficient * originalValue[1]),
                static_cast<float>(conversionCoefficient * originalValue[2]),
            };

            bool save = false;
            for (int i = 0; i < static_cast<int>(fv.size()); ++i)
            {
                ImGui::PushID(i);
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

                // dimension hint
                {
                    glm::vec4 color = {0.0f, 0.0f, 0.0f, 0.6f};
                    color[i] = 1.0f;

                    ImDrawList* l = ImGui::GetWindowDrawList();
                    glm::vec2 p = ImGui::GetCursorScreenPos();
                    float h = ImGui::GetTextLineHeight() + 2.0f*ImGui::GetStyle().FramePadding.y + 2.0f*ImGui::GetStyle().FrameBorderSize;
                    glm::vec2 dims = glm::vec2{4.0f, h};
                    l->AddRectFilled(p, p + dims, ImGui::ColorConvertFloat4ToU32(color));
                    ImGui::SetCursorScreenPos({p.x + 4.0f, p.y});
                }
                ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, {1.0f, 0.0f});
                if (ImGui::InputScalar("##valueinput", ImGuiDataType_Float, &fv[i], &m_StepSize, nullptr, OSC_DEFAULT_FLOAT_INPUT_FORMAT))
                {
                    double inverseConversionCoefficient = 1.0/conversionCoefficient;
                    m_ActiveEdits[i] = inverseConversionCoefficient * static_cast<double>(fv[i]);
                }
                ImGui::PopStyleVar();

                // annotate the control, for screenshots
                {
                    std::stringstream annotation;
                    annotation << "ObjectPropertiesEditor::Vec3/";
                    annotation << i;
                    annotation << '/';
                    annotation << prop.getName();
                    osc::App::upd().addFrameAnnotation(std::move(annotation).str(), osc::GetItemRect());
                }

                if (ItemValueShouldBeSaved())
                {
                    save = true;
                }
                osc::DrawTooltipIfItemHovered("Step Size", "You can right-click to adjust the step size of the buttons");

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
                            m_StepSize = 180.0f;
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("1/2 pi"))
                        {
                            m_StepSize = 90.0f;
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("1/4 pi"))
                        {
                            m_StepSize = 45.0f;
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("10/180 pi"))
                        {
                            m_StepSize = 10.0f;
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("1/180 pi"))
                        {
                            m_StepSize = 1.0f;
                        }

                        ImGui::EndTable();
                    }

                    ImGui::EndPopup();
                }
                ImGui::PopID();
            }

            if (save)
            {
                SimTK::Vec3 newValue = originalValue;
                for (int i = 0; i < static_cast<int>(m_ActiveEdits.size()); ++i)
                {
                    if (m_ActiveEdits[i])
                    {
                        newValue[i] = *m_ActiveEdits[i];
                    }
                }
                rv = MakePropValueSetter<SimTK::Vec3>(newValue);
            }

            ImGui::NextColumn();

            return rv;
        }

        float m_StepSize = 0.001f;
        bool m_OrientationValsAreInRadians = false;
        std::array<std::optional<double>, 3> m_ActiveEdits{};
    };

    class Vec6PropertyEditor final : public PropertyEditorT<OpenSim::SimpleProperty<SimTK::Vec6>> {
    private:
        std::optional<UpdateFn> drawDowncastedImpl(PropertyType const& prop) override
        {
            DrawPropertyName(prop);
            ImGui::NextColumn();

            std::optional<UpdateFn> rv = std::nullopt;

            if (prop.isListProperty())
            {
                ImGui::Text("%s", prop.toString().c_str());
                ImGui::NextColumn();
                return rv;
            }

            SimTK::Vec6 v = prop.getValue();
            std::array<float, 6> fv;
            for (size_t i = 0; i < fv.size(); ++i)
            {
                fv[i] = static_cast<float>(v[static_cast<int>(i)]);
            }

            bool shouldSave = false;

            for (int i = 0; i < 2; ++i)
            {
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::PushID(i);
                if (ImGui::InputFloat3("##vec6editor_a", fv.data() + 3*i, OSC_DEFAULT_FLOAT_INPUT_FORMAT))
                {
                    m_RetainedValue[3*i + 0] = static_cast<double>(fv[3*i + 0]);
                    m_RetainedValue[3*i + 1] = static_cast<double>(fv[3*i + 1]);
                    m_RetainedValue[3*i + 2] = static_cast<double>(fv[3*i + 2]);
                }
                osc::App::upd().addFrameAnnotation("ObjectPropertiesEditor::Vec6Editor/" + prop.getName(), osc::GetItemRect());
                shouldSave = shouldSave || ItemValueShouldBeSaved();
                ImGui::PopID();
            }

            if (shouldSave)
            {
                rv = MakePropValueSetter<SimTK::Vec6>(m_RetainedValue);
            }

            ImGui::NextColumn();

            return rv;
        }

        SimTK::Vec6 m_RetainedValue{};
    };

    class AppearancePropertyEditor final : public PropertyEditorT<OpenSim::ObjectProperty<OpenSim::Appearance>> {
    private:
        std::optional<UpdateFn> drawDowncastedImpl(PropertyType const& prop) override
        {
            DrawPropertyName(prop);
            ImGui::NextColumn();

            std::optional<UpdateFn> rv = std::nullopt;

            OpenSim::Appearance const& app = prop.getValue();
            SimTK::Vec3 color = app.get_color();

            float rgb[4] =
            {
                static_cast<float>(color[0]),
                static_cast<float>(color[1]),
                static_cast<float>(color[2]),
                static_cast<float>(app.get_opacity())
            };

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

            ImGui::PushID(1);
            if (ImGui::ColorEdit4("##coloreditor", rgb))
            {
                SimTK::Vec3 newColor;
                newColor[0] = static_cast<double>(rgb[0]);
                newColor[1] = static_cast<double>(rgb[1]);
                newColor[2] = static_cast<double>(rgb[2]);

                OpenSim::Appearance newAppearance{app};
                newAppearance.set_color(newColor);
                newAppearance.set_opacity(static_cast<double>(rgb[3]));

                rv = MakePropValueSetter<OpenSim::Appearance>(newAppearance);
            }
            ImGui::PopID();

            bool is_visible = app.get_visible();
            ImGui::PushID(2);
            if (ImGui::Checkbox("is visible", &is_visible))
            {
                OpenSim::Appearance newAppearance{app};
                newAppearance.set_visible(is_visible);

                rv = MakePropValueSetter<OpenSim::Appearance>(newAppearance);
            }
            ImGui::PopID();

            ImGui::NextColumn();

            return rv;
        }
    };

    class ContactParameterSetEditor final : public PropertyEditorT<OpenSim::ObjectProperty<OpenSim::HuntCrossleyForce::ContactParametersSet>> {
    private:
        std::optional<UpdateFn> drawDowncastedImpl(OpenSim::ObjectProperty<OpenSim::HuntCrossleyForce::ContactParametersSet> const& cps) override
        {
            std::optional<UpdateFn> rv;
            if (cps.getValue().getSize() > 0)
            {
                OpenSim::HuntCrossleyForce::ContactParameters const& params = cps.getValue()[0];

                ImGui::Columns();
                auto resp = m_NestedEditors.draw(params);
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
            }
            return rv;
        }

        osc::ObjectPropertiesEditor m_NestedEditors;
    };

    using PropEditorCtor = std::unique_ptr<PropertyEditor>(*)(void);

    template<typename T>
    std::pair<size_t, PropEditorCtor> MakeLookupEntry()
    {
        static_assert(std::is_base_of_v<PropertyEditor, T>);
        return {
            typeid(typename T::PropertyType).hash_code(),
            []() { return std::unique_ptr<PropertyEditor>{new T{}}; }
        };
    }

    std::unordered_map<size_t, PropEditorCtor> const& GetPropertyEditorLookup()
    {
        // edit this to add more property editors
        static std::unordered_map<size_t, PropEditorCtor> g_PropertyEditors =
        {
            MakeLookupEntry<StringPropertyEditor>(),
            MakeLookupEntry<DoublePropertyEditor>(),
            MakeLookupEntry<BoolPropertyEditor>(),
            MakeLookupEntry<Vec3PropertyEditor>(),
            MakeLookupEntry<Vec6PropertyEditor>(),
            MakeLookupEntry<AppearancePropertyEditor>(),
            MakeLookupEntry<ContactParameterSetEditor>(),
        };

        return g_PropertyEditors;
    }

    bool CanBeEdited(OpenSim::AbstractProperty const& p)
    {
        return osc::ContainsKey(GetPropertyEditorLookup(), typeid(p).hash_code());
    }

    std::unique_ptr<PropertyEditor> CreatePropertyEditorFor(OpenSim::AbstractProperty const& p)
    {
        auto const& lookup = GetPropertyEditorLookup();

        auto it = lookup.find(typeid(p).hash_code());

        if (it != lookup.end())
        {
            return it->second();
        }
        else
        {
            return nullptr;
        }
    }
}

// top-level implementation of the properties editor
class osc::ObjectPropertiesEditor::Impl final {

private:
    PropertyEditor* tryLookupOrCreateEditor(OpenSim::AbstractProperty const& p)
    {
        if (!CanBeEdited(p))
        {
            // not an editable type, so the implementation should handle this nullptr
            // and print information in the UI
            return nullptr;
        }

        auto [it, inserted] = m_PropertyEditors.try_emplace(p.getName(), nullptr);

        if (inserted)
        {
            // first time a property with this name has been looked up - create
            // a new editor
            it->second = CreatePropertyEditorFor(p);
        }
        else if (!it->second->isEditorFor(p))
        {
            // property with this name is already being managed, but with a different
            // type, so create a new editor for the correct type
            it->second = CreatePropertyEditorFor(p);
        }

        return it->second.get();
    }

    std::optional<ObjectPropertyEdit> drawPropertyEditor(OpenSim::Object const& obj, OpenSim::AbstractProperty const& prop)
    {
        PropertyEditor* editor = tryLookupOrCreateEditor(prop);
        std::optional<ObjectPropertyEdit> rv;

        ImGui::PushID(std::addressof(prop));
        if (editor)
        {
            std::optional<UpdateFn> fn = editor->draw(prop);
            if (fn)
            {
                rv.emplace(ObjectPropertyEdit{obj, prop, std::move(fn).value()});
            }
        }
        else
        {
            // no editor available for this type
            DrawPropertyName(prop);
            ImGui::NextColumn();
            ImGui::TextUnformatted(prop.toString().c_str());
            ImGui::NextColumn();
        }
        ImGui::PopID();

        return rv;
    }

public:

    std::optional<ObjectPropertyEdit> draw(OpenSim::Object const& obj)
    {
        // clear cache, if necessary
        if (m_PreviousObject != &obj)
        {
            m_PropertyEditors.clear();
            m_PreviousObject = &obj;
        }

        std::optional<ObjectPropertyEdit> rv = std::nullopt;

        ImGui::Columns(2);
        for (int i = 0; i < obj.getNumProperties(); ++i)
        {
            ImGui::PushID(i);

            ImGui::Separator();

            OpenSim::AbstractProperty const& prop = obj.getPropertyByIndex(i);
            std::optional<ObjectPropertyEdit> resp = drawPropertyEditor(obj, prop);

            if (resp && !rv)
            {
                rv.emplace(std::move(resp).value());
            }

            ImGui::PopID();
        }
        ImGui::Columns();

        return rv;
    }

private:
    std::unordered_map<std::string, std::unique_ptr<PropertyEditor>> m_PropertyEditors;
    OpenSim::Object const* m_PreviousObject = nullptr;
};


// public API

osc::ObjectPropertyEdit::ObjectPropertyEdit(
    OpenSim::Object const& obj,
    OpenSim::AbstractProperty const& prop,
    std::function<void(OpenSim::AbstractProperty&)> updater) :

    m_ComponentAbsPath{GetAbsPathOrEmptyIfNotAComponent(obj)},
    m_PropertyName{prop.getName()},
    m_Updater{std::move(updater)}
{
}

std::string const& osc::ObjectPropertyEdit::getComponentAbsPath() const
{
    return m_ComponentAbsPath;
}

std::string const& osc::ObjectPropertyEdit::getPropertyName() const
{
    return m_PropertyName;
}

void osc::ObjectPropertyEdit::apply(OpenSim::AbstractProperty& prop)
{
    m_Updater(prop);
}

osc::ObjectPropertiesEditor::ObjectPropertiesEditor() :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::ObjectPropertiesEditor::ObjectPropertiesEditor(ObjectPropertiesEditor&&) noexcept = default;
osc::ObjectPropertiesEditor& osc::ObjectPropertiesEditor::operator=(ObjectPropertiesEditor&&) noexcept = default;
osc::ObjectPropertiesEditor::~ObjectPropertiesEditor() noexcept = default;

std::optional<osc::ObjectPropertyEdit> osc::ObjectPropertiesEditor::draw(OpenSim::Object const& obj)
{
    return m_Impl->draw(obj);
}