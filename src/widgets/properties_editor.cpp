#include "properties_editor.hpp"

#include "src/assertions.hpp"
#include "src/widgets/help_marker.hpp"
#include "src/widgets/lockable_f3_editor.hpp"

#include <OpenSim/Common/AbstractProperty.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/Object.h>
#include <OpenSim/Simulation/Model/Appearance.h>
#include <SimTKcommon.h>
#include <imgui.h>

#include <cstring>
#include <string>
#include <unordered_map>

using namespace osc;

template<typename Coll1, typename Coll2>
static float diff(Coll1 const& older, Coll2 const& newer, size_t n) {
    for (int i = 0; i < static_cast<int>(n); ++i) {
        if (static_cast<float>(older[i]) != static_cast<float>(newer[i])) {
            return newer[i];
        }
    }
    return static_cast<float>(older[0]);
}

template<typename T>
static void draw_property_editor(
    Property_editor_state&,
    OpenSim::Object&,
    int i,
    OpenSim::Property<T> const&,
    std::function<void()> const& before_property_edited,
    std::function<void()> const& after_property_edited);

template<>
void draw_property_editor<std::string>(
    Property_editor_state&,
    OpenSim::Object& obj,
    int parent_idx,
    OpenSim::Property<std::string> const& prop,
    std::function<void()> const& before_property_edited,
    std::function<void()> const& after_property_edited) {

    for (int idx = 0; idx < prop.size(); ++idx) {
        char buf[64]{};
        buf[sizeof(buf) - 1] = '\0';
        std::strncpy(buf, prop.getValue(idx).c_str(), sizeof(buf) - 1);

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

        ImGui::PushID(idx);
        if (ImGui::InputText("##stringeditor", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
            before_property_edited();
            static_cast<OpenSim::Property<std::string>&>(obj.updPropertyByIndex(parent_idx)).setValue(0, buf);
            after_property_edited();
        }
        ImGui::PopID();
    }
}

template<>
void draw_property_editor<double>(
    Property_editor_state& st,
    OpenSim::Object& obj,
    int i,
    OpenSim::Property<double> const& prop,
    std::function<void()> const& before_property_edited,
    std::function<void()> const& after_property_edited) {

    if (prop.size() == 0) {
        // edge case: empty prop?
    } else if (!prop.isListProperty()) {
        // it's a *single* double

        float v = static_cast<float>(prop.getValue());

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputFloat("##doubleditor", &v, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
            before_property_edited();
            static_cast<OpenSim::Property<double>&>(obj.updPropertyByIndex(i)).setValue(static_cast<double>(v));
            after_property_edited();
        }
    } else if (prop.size() == 2) {
        // it's *two* doubles

        // lock btn
        bool locked = st.is_locked;
        if (ImGui::Checkbox("##vec2lockbtn", &locked)) {
            before_property_edited();
            st.is_locked = locked;
            after_property_edited();
        }
        ImGui::SameLine();

        float vs[2] = {static_cast<float>(prop.getValue(0)), static_cast<float>(prop.getValue(1))};
        float old[2] = {vs[0], vs[1]};

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputFloat2("##vec2editor", vs, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
            double v1;
            double v2;
            if (locked) {
                float new_val = diff(old, vs, 2);
                v1 = static_cast<double>(new_val);
                v2 = static_cast<double>(new_val);
            } else {
                v1 = static_cast<double>(vs[0]);
                v2 = static_cast<double>(vs[1]);
            }

            before_property_edited();
            auto& mutable_prop = static_cast<OpenSim::Property<double>&>(obj.updPropertyByIndex(i));
            mutable_prop.setValue(0, v1);
            mutable_prop.setValue(1, v2);
            after_property_edited();
        }
    } else {
        ImGui::Text("%s", prop.toString().c_str());
    }
}

template<>
void draw_property_editor<bool>(
    Property_editor_state&,
    OpenSim::Object& obj,
    int i,
    OpenSim::Property<bool> const& prop,
    std::function<void()> const& before_property_edited,
    std::function<void()> const& after_property_edited) {

    if (prop.isListProperty()) {
        ImGui::Text("%s", prop.toString().c_str());
        return;
    }

    bool v = prop.getValue();
    if (ImGui::Checkbox("##booleditor", &v)) {
        before_property_edited();
        static_cast<OpenSim::Property<bool>&>(obj.updPropertyByIndex(i)).setValue(v);
        after_property_edited();
    }
}

template<>
void draw_property_editor<SimTK::Vec3>(
    Property_editor_state& st,
    OpenSim::Object& obj,
    int i,
    OpenSim::Property<SimTK::Vec3> const& prop,
    std::function<void()> const& before_property_edited,
    std::function<void()> const& after_property_edited) {

    if (prop.isListProperty()) {
        ImGui::Text("%s", prop.toString().c_str());
        return;
    }

    SimTK::Vec3 v = prop.getValue();
    float fv[3] = {
        static_cast<float>(v[0]),
        static_cast<float>(v[1]),
        static_cast<float>(v[2]),
    };

    bool locked = st.is_locked;
    if (draw_lockable_f3_editor("##vec3lockbtn", "##vec3editor", fv, &locked)) {
        before_property_edited();
        st.is_locked = locked;
        v[0] = static_cast<double>(fv[0]);
        v[1] = static_cast<double>(fv[1]);
        v[2] = static_cast<double>(fv[2]);
        static_cast<OpenSim::Property<SimTK::Vec3>&>(obj.updPropertyByIndex(i)).setValue(v);
        after_property_edited();
    }
}

template<>
void draw_property_editor<SimTK::Vec6>(
    Property_editor_state&,
    OpenSim::Object& obj,
    int i,
    OpenSim::Property<SimTK::Vec6> const& prop,
    std::function<void()> const& before_property_edited,
    std::function<void()> const& after_property_edited) {

    if (prop.isListProperty()) {
        ImGui::Text("%s", prop.toString().c_str());
        return;
    }

    SimTK::Vec6 const& v = prop.getValue();
    float vs[6];
    vs[0] = static_cast<float>(v[0]);
    vs[1] = static_cast<float>(v[1]);
    vs[2] = static_cast<float>(v[2]);
    vs[3] = static_cast<float>(v[3]);
    vs[4] = static_cast<float>(v[4]);
    vs[5] = static_cast<float>(v[5]);

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::InputFloat3("##vec6editor_a", vs, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
        before_property_edited();
        static_cast<OpenSim::Property<SimTK::Vec6>&>(obj.updPropertyByIndex(i))
            .setValue(SimTK::Vec6{static_cast<double>(vs[0]),
                                  static_cast<double>(vs[1]),
                                  static_cast<double>(vs[2]),
                                  static_cast<double>(vs[3]),
                                  static_cast<double>(vs[4]),
                                  static_cast<double>(vs[5])});
        after_property_edited();
    }

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::InputFloat3("##vec6editor_b", vs + 3, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
        before_property_edited();
        static_cast<OpenSim::Property<SimTK::Vec6>&>(obj.updPropertyByIndex(i))
            .setValue(SimTK::Vec6{static_cast<double>(vs[0]),
                                  static_cast<double>(vs[1]),
                                  static_cast<double>(vs[2]),
                                  static_cast<double>(vs[3]),
                                  static_cast<double>(vs[4]),
                                  static_cast<double>(vs[5])});
        after_property_edited();
    }
}

template<>
void draw_property_editor<OpenSim::Appearance>(
    Property_editor_state&,
    OpenSim::Object& obj,
    int i,
    OpenSim::Property<OpenSim::Appearance> const& prop,
    std::function<void()> const& before_property_edited,
    std::function<void()> const& after_property_edited) {

    OpenSim::Appearance const& app = prop.getValue();
    SimTK::Vec3 color = app.get_color();
    float rgb[4];
    rgb[0] = static_cast<float>(color[0]);
    rgb[1] = static_cast<float>(color[1]);
    rgb[2] = static_cast<float>(color[2]);
    rgb[3] = static_cast<float>(app.get_opacity());
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::ColorEdit4("##coloreditor", rgb)) {
        SimTK::Vec3 newColor;
        newColor[0] = static_cast<double>(rgb[0]);
        newColor[1] = static_cast<double>(rgb[1]);
        newColor[2] = static_cast<double>(rgb[2]);
        before_property_edited();
        static_cast<OpenSim::Property<OpenSim::Appearance>&>(obj.updPropertyByIndex(i)).updValue().set_color(newColor);
        static_cast<OpenSim::Property<OpenSim::Appearance>&>(obj.updPropertyByIndex(i))
            .updValue()
            .set_opacity(static_cast<double>(rgb[3]));
        after_property_edited();
    }

    bool is_visible = app.get_visible();
    if (ImGui::Checkbox("is visible", &is_visible)) {
        before_property_edited();
        const_cast<OpenSim::Appearance&>(app).set_visible(is_visible);
        after_property_edited();
    }
}

// everything in here is a bunch of type lookup magic so that the implementation can use one O(1)
// lookup to figure out which property editor to render. Otherwise, the implementation would need
// to chain a bunch of `if (dynamic_cast<T>)...`s together, which is probably also fine, but would
// gradually get slower (and uglier) as more editors are added
namespace {
    using editor_rendering_fn = void(
        Property_editor_state&,
        OpenSim::Object&,
        int,
        OpenSim::AbstractProperty const&,
        std::function<void()> const&,
        std::function<void()> const&);

    // this magic is here so that the various type-monomorphized functions can be treated with
    // identical function signatures
    template<typename T>
    static void draw_property_editor_TYPE_ERASED(
        Property_editor_state& st,
        OpenSim::Object& component,
        int i,
        OpenSim::AbstractProperty const& prop,
        std::function<void()> const& before_property_edited,
        std::function<void()> const& after_property_edited) {

        draw_property_editor(
            st,
            component,
            i,
            dynamic_cast<OpenSim::Property<T> const&>(prop),
            before_property_edited,
            after_property_edited);
    }

    template<typename T>
    static constexpr std::pair<size_t, editor_rendering_fn*> simple_prop_editor() {
        return {typeid(OpenSim::SimpleProperty<T>).hash_code(), draw_property_editor_TYPE_ERASED<T>};
    }

    template<typename T>
    static constexpr std::pair<size_t, editor_rendering_fn*> object_prop_editor() {
        return {typeid(OpenSim::ObjectProperty<T>).hash_code(), draw_property_editor_TYPE_ERASED<T>};
    }

    static const std::unordered_map<size_t, editor_rendering_fn*> editors = {simple_prop_editor<std::string>(),
                                                                             simple_prop_editor<double>(),
                                                                             simple_prop_editor<bool>(),
                                                                             simple_prop_editor<SimTK::Vec3>(),
                                                                             simple_prop_editor<SimTK::Vec6>(),

                                                                             object_prop_editor<OpenSim::Appearance>()};
}

static void draw_property_editor(
    Property_editor_state& st,
    OpenSim::Object& parent,
    int prop_idx_in_parent,
    OpenSim::AbstractProperty const& prop,
    std::function<void()> const& before_property_edited,
    std::function<void()> const& after_property_edited) {

    // left column: property name
    ImGui::Text("%s", prop.getName().c_str());
    {
        std::string const& comment = prop.getComment();
        if (!comment.empty()) {
            ImGui::SameLine();
            draw_help_marker(prop.getComment().c_str());
        }
    }
    ImGui::NextColumn();

    // right column: editor
    ImGui::PushID(std::addressof(prop));
    auto it = editors.find(typeid(prop).hash_code());
    if (it != editors.end()) {
        it->second(st, parent, prop_idx_in_parent, prop, before_property_edited, after_property_edited);
    } else {
        // no editor available for this type
        ImGui::Text("%s", prop.toString().c_str());
    }
    ImGui::PopID();
    ImGui::NextColumn();
}

void osc::draw_properties_editor(
    Properties_editor_state& st,
    OpenSim::Object& obj,
    std::function<void()> const& before_property_edited,
    std::function<void()> const& after_property_edited) {

    int num_props = obj.getNumProperties();
    OSC_ASSERT(num_props >= 0);

    st.property_editor_states.resize(static_cast<size_t>(num_props));

    ImGui::Columns(2);
    for (int i = 0; i < num_props; ++i) {
        OpenSim::AbstractProperty const& p = obj.getPropertyByIndex(i);
        Property_editor_state& substate = st.property_editor_states[static_cast<size_t>(i)];
        draw_property_editor(substate, obj, i, p, before_property_edited, after_property_edited);
    }
    ImGui::Columns(1);
}

void osc::draw_properties_editor_for_props_with_indices(
    Properties_editor_state& st,
    OpenSim::Object& obj,
    int* indices,
    size_t nindices,
    std::function<void()> const& before_property_edited,
    std::function<void()> const& after_property_edited) {

    int highest = *std::max_element(indices, indices + nindices);
    int nprops = obj.getNumProperties();
    OSC_ASSERT(highest >= 0);
    OSC_ASSERT(highest < nprops);

    st.property_editor_states.resize(static_cast<size_t>(highest));

    ImGui::Columns(2);
    for (size_t i = 0; i < nindices; ++i) {
        int propidx = indices[i];
        OpenSim::AbstractProperty const& p = obj.getPropertyByIndex(propidx);
        Property_editor_state& substate = st.property_editor_states[static_cast<size_t>(i)];
        draw_property_editor(substate, obj, propidx, p, before_property_edited, after_property_edited);
    }
    ImGui::Columns(1);
}
