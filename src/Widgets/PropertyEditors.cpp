#include "PropertyEditors.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Platform/Log.hpp"
#include "osc_config.hpp"

#include <OpenSim/Common/AbstractProperty.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/Object.h>
#include <OpenSim/Simulation/Model/Appearance.h>
#include <OpenSim/Simulation/Model/ModelVisualPreferences.h>
#include <SDL_events.h>
#include <SimTKcommon.h>
#include <imgui.h>

#include <cstring>
#include <string>
#include <unordered_map>

using namespace osc;

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
            (ImGui::IsItemEdited() && IsAnyKeyPressed({SDL_SCANCODE_RETURN, SDL_SCANCODE_TAB}));
}

using UpdateFn = std::function<void(OpenSim::AbstractProperty&)>;

static void DrawIthStringEditor(
        OpenSim::SimpleProperty<std::string> const& prop,
        int idx,
        std::optional<UpdateFn>& rv)
{
    if (prop.getMaxListSize() > 1)
    {
        if (ImGui::Button("X") && !rv)
        {
            rv = MakePropElementDeleter<std::string>(idx);
        }
        ImGui::SameLine();
    }

    // care: optional values can have size==0 but can have a value later
    //                assigned to that slot
    std::string curValue = prop.size() <= idx ? "" : prop.getValue(idx);

    bool edited = false;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
    if (InputString("##stringeditor", curValue, 128))
    {
        edited = true;
    }

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

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());

    if (ImGui::InputFloat("##doubleditor", &fv, 0.0f, 0.0f, OSC_DEFAULT_FLOAT_INPUT_FORMAT) && ItemValueShouldBeSaved())
    {
        double dv = static_cast<double>(fv);
        rv = MakePropValueSetter<double>(dv);
    }
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

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
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
            std::optional<UpdateFn> rv = std::nullopt;

            int nEls = std::max(prop.size(), 1);  // care: optional properties have size==0

            for (int idx = 0; idx < nEls; ++idx)
            {
                ImGui::PushID(idx);
                DrawIthStringEditor(prop, idx, rv);
                ImGui::PopID();
            }

            return rv;
        }
    };

    class DoublePropertyEditor final : public PropertyEditorT<OpenSim::SimpleProperty<double>> {
    private:
        std::optional<UpdateFn> drawDowncastedImpl(PropertyType const& prop) override
        {
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

            return rv;
        }
    };

    class BoolPropertyEditor final : public PropertyEditorT<OpenSim::SimpleProperty<bool>> {
    private:
        std::optional<UpdateFn> drawDowncastedImpl(PropertyType const& prop) override
        {
            std::optional<UpdateFn> rv = std::nullopt;

            if (prop.isListProperty())
            {
                ImGui::Text("%s", prop.toString().c_str());
                return rv;
            }

            bool v = prop.getValue();
            if (ImGui::Checkbox("##booleditor", &v))
            {
                return MakePropValueSetter<bool>(v);
            }

            return rv;
        }
    };

    class Vec3PropertyEditor final : public PropertyEditorT<OpenSim::SimpleProperty<SimTK::Vec3>> {
    private:
        std::optional<UpdateFn> drawDowncastedImpl(PropertyType const& prop) override
        {
            std::optional<UpdateFn> rv = std::nullopt;

            if (prop.isListProperty())
            {
                ImGui::Text("%s", prop.toString().c_str());
                return rv;
            }

            SimTK::Vec3 v = prop.getValue();
            std::array<float, 3> fv =
            {
                static_cast<float>(v[0]),
                static_cast<float>(v[1]),
                static_cast<float>(v[2])
            };

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());

            if (ImGui::InputFloat3("##vec3editor", fv.data(), OSC_DEFAULT_FLOAT_INPUT_FORMAT))
            {
                m_RetainedValue[0] = static_cast<double>(fv[0]);
                m_RetainedValue[1] = static_cast<double>(fv[1]);
                m_RetainedValue[2] = static_cast<double>(fv[2]);
            }

            if (ItemValueShouldBeSaved())
            {
                rv = MakePropValueSetter<SimTK::Vec3>(m_RetainedValue);
            }

            return rv;
        }

        SimTK::Vec3 m_RetainedValue{};
    };

    class Vec6PropertyEditor final : public PropertyEditorT<OpenSim::SimpleProperty<SimTK::Vec6>> {
    private:
        std::optional<UpdateFn> drawDowncastedImpl(PropertyType const& prop) override
        {
            std::optional<UpdateFn> rv = std::nullopt;

            if (prop.isListProperty())
            {
                ImGui::Text("%s", prop.toString().c_str());
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
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
                ImGui::PushID(i);
                if (ImGui::InputFloat3("##vec6editor_a", fv.data() + 3*i, OSC_DEFAULT_FLOAT_INPUT_FORMAT))
                {
                    m_RetainedValue[3*i + 0] = static_cast<double>(fv[3*i + 0]);
                    m_RetainedValue[3*i + 1] = static_cast<double>(fv[3*i + 1]);
                    m_RetainedValue[3*i + 2] = static_cast<double>(fv[3*i + 2]);
                }
                shouldSave = shouldSave || ItemValueShouldBeSaved();
                ImGui::PopID();
            }

            if (shouldSave)
            {
                rv = MakePropValueSetter<SimTK::Vec6>(m_RetainedValue);
            }

            return rv;
        }

        SimTK::Vec6 m_RetainedValue{};
    };

    class AppearancePropertyEditor final : public PropertyEditorT<OpenSim::ObjectProperty<OpenSim::Appearance>> {
    private:
        std::optional<UpdateFn> drawDowncastedImpl(PropertyType const& prop) override
        {
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

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());

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

            return rv;
        }
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
            MakeLookupEntry<AppearancePropertyEditor>()
        };

        return g_PropertyEditors;
    }

    bool CanBeEdited(OpenSim::AbstractProperty const& p)
    {
        return ContainsKey(GetPropertyEditorLookup(), typeid(p).hash_code());
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
struct osc::ObjectPropertiesEditor::Impl final {

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

    void ensurePropertyEditorsValidFor(OpenSim::Object const& obj)
    {
        if (m_PreviousObject != &obj)
        {
            m_PropertyEditors.clear();
            m_PreviousObject = &obj;
        }
    }

    std::optional<ObjectPropertiesEditor::Response> draw(OpenSim::AbstractProperty const& p)
    {
        // left column: property name
        ImGui::Text("%s", p.getName().c_str());
        {
            std::string const& comment = p.getComment();
            if (!comment.empty())
            {
                ImGui::SameLine();
                DrawHelpMarker(p.getComment().c_str());
            }
        }
        ImGui::NextColumn();

        // right column: editor

        PropertyEditor* editor = tryLookupOrCreateEditor(p);
        std::optional<ObjectPropertiesEditor::Response> rv;

        ImGui::PushID(std::addressof(p));
        if (editor)
        {
            std::optional<UpdateFn> fn = editor->draw(p);
            if (fn)
            {
                rv.emplace(ObjectPropertiesEditor::Response{p, std::move(fn).value()});
            }
        }
        else
        {
            // no editor available for this type
            ImGui::Text("%s", p.toString().c_str());
        }
        ImGui::PopID();
        ImGui::NextColumn();

        return rv;
    }

    void drawPropertyWithIndex(OpenSim::Object const& obj,
                               int idx,
                               std::optional<ObjectPropertiesEditor::Response>& out)
    {
        ImGui::PushID(idx);
        OpenSim::AbstractProperty const& p = obj.getPropertyByIndex(idx);
        std::optional<ObjectPropertiesEditor::Response> resp = draw(p);
        if (resp && !out)
        {
            out.emplace(std::move(resp).value());
        }
        ImGui::PopID();
    }

public:

    std::optional<ObjectPropertiesEditor::Response> draw(OpenSim::Object const& obj)
    {
        std::optional<ObjectPropertiesEditor::Response> rv = std::nullopt;

        int numProps = obj.getNumProperties();

        if (numProps <= 0)
        {
            return rv;
        }

        ensurePropertyEditorsValidFor(obj);

        ImGui::Columns(2);
        for (int i = 0; i < numProps; ++i)
        {
            drawPropertyWithIndex(obj, i, rv);
        }
        ImGui::Columns();

        return rv;
    }

    std::optional<ObjectPropertiesEditor::Response> draw(OpenSim::Object const& obj,
                                                         nonstd::span<int const> indices)
    {
        std::optional<ObjectPropertiesEditor::Response> rv = std::nullopt;

        int highest = *std::max_element(indices.begin(), indices.end());
        int nprops = obj.getNumProperties();

        if (highest < 0)
        {
            return rv;
        }

        if (nprops < highest)
        {
            return rv;
        }

        ensurePropertyEditorsValidFor(obj);

        ImGui::Columns(2);
        for (int propidx : indices)
        {
            drawPropertyWithIndex(obj, propidx, rv);
        }
        ImGui::Columns();

        return rv;
    }

private:

    std::unordered_map<std::string, std::unique_ptr<PropertyEditor>> m_PropertyEditors;
    OpenSim::Object const* m_PreviousObject = nullptr;
};


// public API

osc::ObjectPropertiesEditor::ObjectPropertiesEditor() :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::ObjectPropertiesEditor::ObjectPropertiesEditor(ObjectPropertiesEditor&&) noexcept = default;

osc::ObjectPropertiesEditor& osc::ObjectPropertiesEditor::operator=(ObjectPropertiesEditor&&) noexcept = default;

osc::ObjectPropertiesEditor::~ObjectPropertiesEditor() noexcept = default;

std::optional<ObjectPropertiesEditor::Response> osc::ObjectPropertiesEditor::draw(OpenSim::Object const& obj)
{
    return m_Impl->draw(obj);
}

std::optional<ObjectPropertiesEditor::Response> osc::ObjectPropertiesEditor::draw(
        OpenSim::Object const& obj,
        nonstd::span<int const> indices)
{
    return m_Impl->draw(obj, indices);
}
