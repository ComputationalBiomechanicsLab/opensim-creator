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
using namespace osc::widgets;

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
static std::optional<property_editor::Response>
    draw_property_editor(property_editor::State&, OpenSim::Property<T> const&);

template<>
std::optional<property_editor::Response>
    draw_property_editor<std::string>(property_editor::State&, OpenSim::Property<std::string> const& prop) {

    std::optional<property_editor::Response> rv = std::nullopt;

    for (int idx = 0; idx < prop.size(); ++idx) {
        char buf[64]{};
        buf[sizeof(buf) - 1] = '\0';
        std::strncpy(buf, prop.getValue(idx).c_str(), sizeof(buf) - 1);

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

        ImGui::PushID(idx);
        if (ImGui::InputText("##stringeditor", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (!rv) {
                rv = property_editor::Response{[idx, s = std::string{buf}](OpenSim::AbstractProperty& p) {
                    OpenSim::Property<std::string>* ps = dynamic_cast<OpenSim::Property<std::string>*>(&p);
                    if (!ps) {
                        return;
                    }
                    ps->setValue(idx, s);
                }};
            }
        }
        ImGui::PopID();
    }

    return rv;
}

template<>
std::optional<property_editor::Response>
    draw_property_editor<double>(property_editor::State& st, OpenSim::Property<double> const& prop) {

    std::optional<property_editor::Response> rv = std::nullopt;

    if (prop.size() == 0) {
        // edge case: empty prop?
    } else if (!prop.isListProperty()) {
        // it's a *single* double

        float v = static_cast<float>(prop.getValue());

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputFloat("##doubleditor", &v, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
            rv = property_editor::Response{[dv = static_cast<double>(v)](OpenSim::AbstractProperty& p) {
                auto* pd = dynamic_cast<OpenSim::Property<double>*>(&p);
                if (!pd) {
                    // ERROR: wrong type passed into the updater at runtime
                    return;
                }
                pd->setValue(dv);
            }};
        }
    } else if (prop.size() == 2) {
        // it's *two* doubles

        // lock btn
        ImGui::Checkbox("##vec2lockbtn", &st.is_locked);
        ImGui::SameLine();

        float vs[2] = {static_cast<float>(prop.getValue(0)), static_cast<float>(prop.getValue(1))};
        float old[2] = {vs[0], vs[1]};

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputFloat2("##vec2editor", vs, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
            double v1;
            double v2;
            if (st.is_locked) {
                float new_val = diff(old, vs, 2);
                v1 = static_cast<double>(new_val);
                v2 = static_cast<double>(new_val);
            } else {
                v1 = static_cast<double>(vs[0]);
                v2 = static_cast<double>(vs[1]);
            }

            rv = property_editor::Response{[v1, v2](OpenSim::AbstractProperty& p) {
                auto* pd = dynamic_cast<OpenSim::Property<double>*>(&p);
                if (!pd) {
                    // ERROR: wrong type passed into the updater at runtime
                    return;
                }
                pd->setValue(0, v1);
                pd->setValue(1, v2);
            }};
        }
    } else {
        ImGui::Text("%s", prop.toString().c_str());
    }

    return rv;
}

template<>
std::optional<property_editor::Response>
    draw_property_editor<bool>(property_editor::State&, OpenSim::Property<bool> const& prop) {

    if (prop.isListProperty()) {
        ImGui::Text("%s", prop.toString().c_str());
        return std::nullopt;
    }

    bool v = prop.getValue();
    if (ImGui::Checkbox("##booleditor", &v)) {
        return property_editor::Response{[v](OpenSim::AbstractProperty& p) {
            auto* pb = dynamic_cast<OpenSim::Property<bool>*>(&p);
            if (!pb) {
                // ERROR: wrong type passed into the updater at runtime
                return;
            }
            pb->setValue(v);
        }};
    }

    return std::nullopt;
}

template<>
std::optional<property_editor::Response>
    draw_property_editor<SimTK::Vec3>(property_editor::State& st, OpenSim::Property<SimTK::Vec3> const& prop) {

    if (prop.isListProperty()) {
        ImGui::Text("%s", prop.toString().c_str());
        return std::nullopt;
    }

    SimTK::Vec3 v = prop.getValue();
    float fv[3] = {
        static_cast<float>(v[0]),
        static_cast<float>(v[1]),
        static_cast<float>(v[2]),
    };

    if (draw_lockable_f3_editor("##vec3lockbtn", "##vec3editor", fv, &st.is_locked)) {
        v[0] = static_cast<double>(fv[0]);
        v[1] = static_cast<double>(fv[1]);
        v[2] = static_cast<double>(fv[2]);
        return property_editor::Response{[v](OpenSim::AbstractProperty& p) {
            auto* pv = dynamic_cast<OpenSim::Property<SimTK::Vec3>*>(&p);
            if (!pv) {
                // ERROR: wrong type passed into the updater at runtime
                return;
            }
            return pv->setValue(v);
        }};
    }

    return std::nullopt;
}

template<>
std::optional<property_editor::Response>
    draw_property_editor<SimTK::Vec6>(property_editor::State&, OpenSim::Property<SimTK::Vec6> const& prop) {

    if (prop.isListProperty()) {
        ImGui::Text("%s", prop.toString().c_str());
        return std::nullopt;
    }

    SimTK::Vec6 v = prop.getValue();
    float vs[6];
    vs[0] = static_cast<float>(v[0]);
    vs[1] = static_cast<float>(v[1]);
    vs[2] = static_cast<float>(v[2]);
    vs[3] = static_cast<float>(v[3]);
    vs[4] = static_cast<float>(v[4]);
    vs[5] = static_cast<float>(v[5]);

    bool edited = false;

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::InputFloat3("##vec6editor_a", vs, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
        v[0] = static_cast<double>(vs[0]);
        v[1] = static_cast<double>(vs[1]);
        v[2] = static_cast<double>(vs[2]);
        v[3] = static_cast<double>(vs[3]);
        v[4] = static_cast<double>(vs[4]);
        v[5] = static_cast<double>(vs[5]);
        edited = true;
    }

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::InputFloat3("##vec6editor_b", vs + 3, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
        v[0] = static_cast<double>(vs[0]);
        v[1] = static_cast<double>(vs[1]);
        v[2] = static_cast<double>(vs[2]);
        v[3] = static_cast<double>(vs[3]);
        v[4] = static_cast<double>(vs[4]);
        v[5] = static_cast<double>(vs[5]);
        edited = true;
    }

    if (edited) {
        return property_editor::Response{[v](OpenSim::AbstractProperty& p) {
            auto* pv = dynamic_cast<OpenSim::Property<SimTK::Vec6>*>(&p);
            if (!pv) {
                // ERROR: wrong type passed into the updater at runtime
                return;
            }
            return pv->setValue(v);
        }};
    }

    return std::nullopt;
}

template<>
std::optional<property_editor::Response> draw_property_editor<OpenSim::Appearance>(
    property_editor::State&, OpenSim::Property<OpenSim::Appearance> const& prop) {

    std::optional<property_editor::Response> rv = std::nullopt;

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

        rv = property_editor::Response{[newColor, opacity = static_cast<double>(rgb[3])](OpenSim::AbstractProperty& p) {
            auto* pa = dynamic_cast<OpenSim::Property<OpenSim::Appearance>*>(&p);
            if (!pa) {
                // ERROR: wrong type passed into the updater at runtime
                return;
            }
            pa->updValue().set_color(newColor);
            pa->updValue().set_opacity(opacity);
        }};
    }

    bool is_visible = app.get_visible();
    if (ImGui::Checkbox("is visible", &is_visible)) {
        rv = property_editor::Response{[is_visible](OpenSim::AbstractProperty& p) {
            auto* pa = dynamic_cast<OpenSim::Property<OpenSim::Appearance>*>(&p);
            if (!pa) {
                // ERROR: wrong type passed into the updater at runtime
                return;
            }
            pa->updValue().set_visible(is_visible);
        }};
    }

    return rv;
}

// everything in here is a bunch of type lookup magic so that the implementation can use one O(1)
// lookup to figure out which property editor to render. Otherwise, the implementation would need
// to chain a bunch of `if (dynamic_cast<T>)...`s together, which is probably also fine, but would
// gradually get slower (and uglier) as more editors are added
namespace {
    using editor_rendering_fn =
        std::optional<property_editor::Response>(property_editor::State&, OpenSim::AbstractProperty const&);

    // this magic is here so that the various type-monomorphized functions can be treated with
    // identical function signatures
    template<typename T>
    static std::optional<property_editor::Response>
        draw_property_editor_TYPE_ERASED(property_editor::State& st, OpenSim::AbstractProperty const& prop) {
        return draw_property_editor(st, dynamic_cast<OpenSim::Property<T> const&>(prop));
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

std::optional<osc::widgets::property_editor::Response>
    osc::widgets::property_editor::draw(State& st, OpenSim::AbstractProperty const& prop) {
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

    std::optional<property_editor::Response> rv = std::nullopt;

    // right column: editor
    ImGui::PushID(std::addressof(prop));
    auto it = editors.find(typeid(prop).hash_code());
    if (it != editors.end()) {
        rv = it->second(st, prop);
    } else {
        // no editor available for this type
        ImGui::Text("%s", prop.toString().c_str());
    }
    ImGui::PopID();
    ImGui::NextColumn();

    return rv;
}

std::optional<osc::widgets::properties_editor::Response>
    osc::widgets::properties_editor::draw(State& st, OpenSim::Object& obj) {

    int num_props = obj.getNumProperties();
    OSC_ASSERT(num_props >= 0);

    st.property_editors.resize(static_cast<size_t>(num_props));

    std::optional<Response> rv = std::nullopt;

    ImGui::Columns(2);
    for (int i = 0; i < num_props; ++i) {
        OpenSim::AbstractProperty const& p = obj.getPropertyByIndex(i);
        property_editor::State& substate = st.property_editors[static_cast<size_t>(i)];
        auto maybe_rv = draw(substate, p);
        if (!rv && maybe_rv) {
            rv.emplace(p, std::move(maybe_rv->updater));
        }
    }
    ImGui::Columns(1);

    return rv;
}

std::optional<osc::widgets::properties_editor::Response>
    osc::widgets::properties_editor::draw(State& st, OpenSim::Object& obj, nonstd::span<int const> indices) {

    int highest = *std::max_element(indices.begin(), indices.end());
    int nprops = obj.getNumProperties();
    OSC_ASSERT(highest >= 0);
    OSC_ASSERT(highest < nprops);

    st.property_editors.resize(static_cast<size_t>(highest));

    std::optional<Response> rv;

    ImGui::Columns(2);
    for (int idx : indices) {
        int propidx = indices[static_cast<size_t>(idx)];
        OpenSim::AbstractProperty const& p = obj.getPropertyByIndex(propidx);
        property_editor::State& substate = st.property_editors[static_cast<size_t>(idx)];
        auto maybe_rv = draw(substate, p);
        if (!rv && maybe_rv) {
            rv.emplace(p, std::move(maybe_rv->updater));
        }
    }
    ImGui::Columns(1);

    return rv;
}
