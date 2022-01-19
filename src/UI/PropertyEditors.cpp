#include "PropertyEditors.hpp"

#include "src/Assertions.hpp"
#include "src/Utils/ImGuiHelpers.hpp"
#include "src/UI/F3Editor.hpp"

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
std::function<void(OpenSim::AbstractProperty&)> MakePropElementDeleter(int idx)
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
std::function<void(OpenSim::AbstractProperty&)> MakePropValueSetter(int idx, T value)
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
std::function<void(OpenSim::AbstractProperty&)> MakePropValueSetter(T value)
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
    return ImGui::IsItemDeactivatedAfterEdit() || IsAnyKeyPressed({SDL_SCANCODE_RETURN, SDL_SCANCODE_TAB});
}

static void DrawIthStringEditor(
        AbstractPropertyEditor&,
        OpenSim::SimpleProperty<std::string> const& prop,
        int idx,
        std::optional<AbstractPropertyEditor::Response>& rv)
{
    if (!prop.isOneValueProperty())
    {
        if (ImGui::Button("X") && !rv)
        {
            rv = AbstractPropertyEditor::Response{MakePropElementDeleter<std::string>(idx)};
        }
        ImGui::SameLine();
    }

    // copy string into on-stack, editable, buffer
    char buf[64]{};
    buf[sizeof(buf) - 1] = '\0';
    std::strncpy(buf, prop.getValue(idx).c_str(), sizeof(buf) - 1);

    bool edited = false;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
    if (ImGui::InputText("##stringeditor", buf, sizeof(buf)))
    {
        edited = true;
    }

    bool shouldUpdate = edited && !rv && ItemValueShouldBeSaved();
    if (shouldUpdate)
    {
        rv = AbstractPropertyEditor::Response{MakePropValueSetter<std::string>(idx, buf)};
    }
}

// draw a `std::string` property editor
static std::optional<AbstractPropertyEditor::Response> draw_editor(
        AbstractPropertyEditor& ape,
        OpenSim::SimpleProperty<std::string> const& prop)
{

    std::optional<AbstractPropertyEditor::Response> rv = std::nullopt;

    for (int idx = 0; idx < prop.size(); ++idx)
    {
        ImGui::PushID(idx);
        DrawIthStringEditor(ape, prop, idx, rv);
        ImGui::PopID();
    }

    return rv;
}

// draw property editor for a single `double` value
static void Draw1DoubleValueEditor(
        AbstractPropertyEditor&,
        OpenSim::SimpleProperty<double> const& prop,
        std::optional<AbstractPropertyEditor::Response>& rv)
{
    if (prop.size() != 1 || prop.isListProperty())
    {
        return;
    }

    float fv = static_cast<float>(prop.getValue());

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());

    if (ImGui::InputFloat("##doubleditor", &fv, 0.0f, 0.0f, "%.3f") && ItemValueShouldBeSaved())
    {
        double dv = static_cast<double>(fv);
        rv = AbstractPropertyEditor::Response{MakePropValueSetter<double>(dv)};
    }
}

// draw a property editor for 2 double values in a list
static void Draw2DoubleValueEditor(
        AbstractPropertyEditor&,
        OpenSim::SimpleProperty<double> const& prop,
        std::optional<AbstractPropertyEditor::Response>& rv)
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
    if (ImGui::InputFloat2("##vec2editor", vs.data(), "%.3f") && ItemValueShouldBeSaved())
    {
        rv = AbstractPropertyEditor::Response{[vs](OpenSim::AbstractProperty& p)
        {
            auto* pd = dynamic_cast<OpenSim::Property<double>*>(&p);
            if (!pd)
            {
                // ERROR: wrong type passed into the updater at runtime
                return;
            }
            pd->setValue(0, static_cast<double>(vs[0]));
            pd->setValue(1, static_cast<double>(vs[1]));
        }};
    }
}

// draw a `double` property editor
static std::optional<AbstractPropertyEditor::Response> draw_editor(
        AbstractPropertyEditor& st,
        OpenSim::SimpleProperty<double> const& prop)
{
    std::optional<AbstractPropertyEditor::Response> rv = std::nullopt;

    if (!prop.isListProperty() && prop.size() == 0)
    {
        // uh
    }
    else if (!prop.isListProperty() && prop.size() == 1)
    {
        Draw1DoubleValueEditor(st, prop, rv);
    }
    else if (prop.size() == 2)
    {
        Draw2DoubleValueEditor(st, prop, rv);
    }
    else
    {
        ImGui::Text("%s", prop.toString().c_str());
    }

    return rv;
}

// draw a `bool` property editor
static std::optional<AbstractPropertyEditor::Response> draw_editor(
        AbstractPropertyEditor&,
        OpenSim::SimpleProperty<bool> const& prop)
{
    std::optional<AbstractPropertyEditor::Response> rv = std::nullopt;

    if (prop.isListProperty())
    {
        ImGui::Text("%s", prop.toString().c_str());
        return rv;
    }

    bool v = prop.getValue();
    if (ImGui::Checkbox("##booleditor", &v))
    {
        return AbstractPropertyEditor::Response{MakePropValueSetter<bool>(v)};
    }

    return rv;
}

// draw a `SimTK::Vec3` property editor
static std::optional<AbstractPropertyEditor::Response> draw_editor(
        AbstractPropertyEditor&,
        OpenSim::SimpleProperty<SimTK::Vec3> const& prop)
{
    std::optional<AbstractPropertyEditor::Response> rv = std::nullopt;

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

    if (ImGui::InputFloat3("##vec3editor", fv.data(), "%.3f") && ItemValueShouldBeSaved())
    {
        v[0] = static_cast<double>(fv[0]);
        v[1] = static_cast<double>(fv[1]);
        v[2] = static_cast<double>(fv[2]);
        rv = AbstractPropertyEditor::Response{MakePropValueSetter<SimTK::Vec3>(v)};
    }

    return rv;
}

// draw a `SimTK::Vec6` property editor
std::optional<AbstractPropertyEditor::Response> draw_editor(
        AbstractPropertyEditor&,
        OpenSim::SimpleProperty<SimTK::Vec6> const& prop)
{
    std::optional<AbstractPropertyEditor::Response> rv = std::nullopt;

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

    bool edited = false;
    bool shouldSave = false;

    for (int i = 0; i < 2; ++i)
    {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
        if (ImGui::InputFloat3("##vec6editor_a", fv.data() + 3*i, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::IsItemDeactivatedAfterEdit())
        {
            v[i+0] = static_cast<double>(fv[i+0]);
            v[i+1] = static_cast<double>(fv[i+1]);
            v[i+2] = static_cast<double>(fv[i+2]);
            edited = true;
        }
        shouldSave = shouldSave || ItemValueShouldBeSaved();
    }

    if (edited && shouldSave)
    {
        rv = AbstractPropertyEditor::Response{MakePropValueSetter<SimTK::Vec6>(v)};
    }

    return rv;
}

// draw a `OpenSim::Appearance` property editor
std::optional<AbstractPropertyEditor::Response> draw_editor(
        AbstractPropertyEditor&,
        OpenSim::ObjectProperty<OpenSim::Appearance> const& prop)
{
    std::optional<AbstractPropertyEditor::Response> rv = std::nullopt;

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

    if (ImGui::ColorEdit4("##coloreditor", rgb))
    {
        SimTK::Vec3 newColor;
        newColor[0] = static_cast<double>(rgb[0]);
        newColor[1] = static_cast<double>(rgb[1]);
        newColor[2] = static_cast<double>(rgb[2]);

        OpenSim::Appearance newAppearance{app};
        newAppearance.set_color(newColor);
        newAppearance.set_opacity(static_cast<double>(rgb[3]));

        rv = AbstractPropertyEditor::Response{MakePropValueSetter<OpenSim::Appearance>(newAppearance)};
    }

    bool is_visible = app.get_visible();
    if (ImGui::Checkbox("is visible", &is_visible))
    {
        OpenSim::Appearance newAppearance{app};
        newAppearance.set_visible(is_visible);

        rv = AbstractPropertyEditor::Response{MakePropValueSetter<OpenSim::Appearance>(newAppearance)};
    }

    return rv;
}

// typedef for a function that can render an editor for an AbstractProperty
//
// all of the above functions have a similar signature, but are specialized to `T`
using draw_editor_type_erased_fn = std::optional<AbstractPropertyEditor::Response>(AbstractPropertyEditor&, OpenSim::AbstractProperty const&);

// template that generates a type-erased version of the `draw_editor` functions above
template<typename TProperty>
static std::optional<AbstractPropertyEditor::Response> draw_editor_type_erased(
        AbstractPropertyEditor& st,
        OpenSim::AbstractProperty const& prop) {

    // call into the not-type-erased version
    return draw_editor(st, dynamic_cast<TProperty const&>(prop));
}

// returns an entry suitable for insertion into the lookup
template<typename TProperty>
static constexpr std::pair<size_t, draw_editor_type_erased_fn*> entry() {
    size_t k = typeid(TProperty).hash_code();
    draw_editor_type_erased_fn* f = draw_editor_type_erased<TProperty>;
    return {k, f};
}

std::optional<AbstractPropertyEditor::Response> osc::AbstractPropertyEditor::draw(
        OpenSim::AbstractProperty const& prop) {

    // global property editor lookup table
    static std::unordered_map<size_t, draw_editor_type_erased_fn*> const g_PropertyEditors = {{
        entry<OpenSim::SimpleProperty<std::string>>(),
        entry<OpenSim::SimpleProperty<double>>(),
        entry<OpenSim::SimpleProperty<bool>>(),
        entry<OpenSim::SimpleProperty<SimTK::Vec3>>(),
        entry<OpenSim::SimpleProperty<SimTK::Vec6>>(),
        entry<OpenSim::ObjectProperty<OpenSim::Appearance>>(),
    }};

    // left column: property name
    ImGui::Text("%s", prop.getName().c_str());
    {
        std::string const& comment = prop.getComment();
        if (!comment.empty()) {
            ImGui::SameLine();
            DrawHelpMarker(prop.getComment().c_str());
        }
    }
    ImGui::NextColumn();

    std::optional<AbstractPropertyEditor::Response> rv = std::nullopt;

    // right column: editor
    ImGui::PushID(std::addressof(prop));
    auto it = g_PropertyEditors.find(typeid(prop).hash_code());
    if (it != g_PropertyEditors.end()) {
        rv = it->second(*this, prop);
    } else {
        // no editor available for this type
        ImGui::Text("%s", prop.toString().c_str());
    }
    ImGui::PopID();
    ImGui::NextColumn();

    return rv;
}

std::optional<ObjectPropertiesEditor::Response> osc::ObjectPropertiesEditor::draw(
        OpenSim::Object const& obj) {

    int num_props = obj.getNumProperties();
    OSC_ASSERT(num_props >= 0);

    propertyEditors.resize(static_cast<size_t>(num_props));

    std::optional<Response> rv = std::nullopt;

    ImGui::Columns(2);
    for (int i = 0; i < num_props; ++i) {
        ImGui::PushID(i);
        OpenSim::AbstractProperty const& p = obj.getPropertyByIndex(i);
        AbstractPropertyEditor& substate = propertyEditors[static_cast<size_t>(i)];
        auto maybe_rv = substate.draw(p);
        if (!rv && maybe_rv) {
            rv.emplace(p, std::move(maybe_rv->updater));
        }
        ImGui::PopID();
    }
    ImGui::Columns(1);

    return rv;
}

std::optional<ObjectPropertiesEditor::Response> osc::ObjectPropertiesEditor::draw(
        OpenSim::Object const& obj,
        nonstd::span<int const> indices) {

    int highest = *std::max_element(indices.begin(), indices.end());
    int nprops = obj.getNumProperties();
    OSC_ASSERT(highest >= 0);
    OSC_ASSERT(highest < nprops);

    propertyEditors.resize(static_cast<size_t>(highest));

    std::optional<Response> rv;

    ImGui::Columns(2);
    for (int propidx : indices) {
        ImGui::PushID(propidx);
        OpenSim::AbstractProperty const& p = obj.getPropertyByIndex(propidx);
        AbstractPropertyEditor& substate = propertyEditors[static_cast<size_t>(propidx)];
        auto maybe_rv = substate.draw(p);
        if (!rv && maybe_rv) {
            rv.emplace(p, std::move(maybe_rv->updater));
        }
        ImGui::PopID();
    }
    ImGui::Columns(1);

    return rv;
}
