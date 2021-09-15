#include "PropertyEditors.hpp"

#include "src/Assertions.hpp"
#include "src/UI/HelpMarker.hpp"
#include "src/UI/F3Editor.hpp"

#include <OpenSim/Common/AbstractProperty.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/Object.h>
#include <OpenSim/Simulation/Model/Appearance.h>
#include <OpenSim/Simulation/Model/ModelVisualPreferences.h>
#include <SimTKcommon.h>
#include <imgui.h>

#include <cstring>
#include <string>
#include <unordered_map>

using namespace osc;

namespace {

    // returns the first value that changed between the first `n` elements of `old` and `newer`
    template<typename Coll1, typename Coll2>
    float diff(Coll1 const& old, Coll2 const& newer, size_t n) {
        for (int i = 0; i < static_cast<int>(n); ++i) {
            if (static_cast<float>(old[i]) != static_cast<float>(newer[i])) {
                return newer[i];
            }
        }
        return static_cast<float>(old[0]);
    }

    // declares the signature of a property editor for T
    //
    // below, the implementation specializes for each T that can be edited
    template<typename T>
    std::optional<AbstractPropertyEditor::Response> draw_editor(AbstractPropertyEditor&, OpenSim::Property<T> const&);

    // draw a `std::string` property editor
    template<>
    std::optional<AbstractPropertyEditor::Response> draw_editor<std::string>(
            AbstractPropertyEditor&,
            OpenSim::Property<std::string> const& prop) {

        std::optional<AbstractPropertyEditor::Response> rv = std::nullopt;

        for (int idx = 0; idx < prop.size(); ++idx) {
            char buf[64]{};
            buf[sizeof(buf) - 1] = '\0';
            std::strncpy(buf, prop.getValue(idx).c_str(), sizeof(buf) - 1);

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

            ImGui::PushID(idx);
            if (ImGui::InputText("##stringeditor", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
                if (!rv) {
                    rv = AbstractPropertyEditor::Response{[idx, s = std::string{buf}](OpenSim::AbstractProperty& p) {
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

    // draw a `double` property editor
    template<>
    std::optional<AbstractPropertyEditor::Response> draw_editor<double>(
            AbstractPropertyEditor& st,
            OpenSim::Property<double> const& prop) {

        std::optional<AbstractPropertyEditor::Response> rv = std::nullopt;

        if (prop.size() == 0) {
            // edge case: empty prop?
        } else if (!prop.isListProperty()) {
            // it's a *single* double

            float v = static_cast<float>(prop.getValue());

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::InputFloat("##doubleditor", &v, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
                rv = AbstractPropertyEditor::Response{[dv = static_cast<double>(v)](OpenSim::AbstractProperty& p) {
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
            ImGui::Checkbox("##vec2lockbtn", &st.isLocked);
            ImGui::SameLine();

            float vs[2] = {static_cast<float>(prop.getValue(0)), static_cast<float>(prop.getValue(1))};
            float old[2] = {vs[0], vs[1]};

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::InputFloat2("##vec2editor", vs, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
                double v1;
                double v2;
                if (st.isLocked) {
                    float new_val = diff(old, vs, 2);
                    v1 = static_cast<double>(new_val);
                    v2 = static_cast<double>(new_val);
                } else {
                    v1 = static_cast<double>(vs[0]);
                    v2 = static_cast<double>(vs[1]);
                }

                rv = AbstractPropertyEditor::Response{[v1, v2](OpenSim::AbstractProperty& p) {
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

    // draw a `bool` property editor
    template<>
    std::optional<AbstractPropertyEditor::Response> draw_editor<bool>(
            AbstractPropertyEditor&,
            OpenSim::Property<bool> const& prop) {

        if (prop.isListProperty()) {
            ImGui::Text("%s", prop.toString().c_str());
            return std::nullopt;
        }

        bool v = prop.getValue();
        if (ImGui::Checkbox("##booleditor", &v)) {
            return AbstractPropertyEditor::Response{[v](OpenSim::AbstractProperty& p) {
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

    // draw a `SimTK::Vec3` property editor
    template<>
    std::optional<AbstractPropertyEditor::Response> draw_editor<SimTK::Vec3>(
            AbstractPropertyEditor& st,
            OpenSim::Property<SimTK::Vec3> const& prop) {

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

        if (DrawF3Editor("##vec3lockbtn", "##vec3editor", fv, &st.isLocked)) {
            v[0] = static_cast<double>(fv[0]);
            v[1] = static_cast<double>(fv[1]);
            v[2] = static_cast<double>(fv[2]);
            return AbstractPropertyEditor::Response{[v](OpenSim::AbstractProperty& p) {
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

    // draw a `SimTK::Vec6` property editor
    template<>
    std::optional<AbstractPropertyEditor::Response> draw_editor<SimTK::Vec6>(
            AbstractPropertyEditor&,
            OpenSim::Property<SimTK::Vec6> const& prop) {

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
            return AbstractPropertyEditor::Response{[v](OpenSim::AbstractProperty& p) {
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

    // draw a `OpenSim::Appearance` property editor
    template<>
    std::optional<AbstractPropertyEditor::Response> draw_editor<OpenSim::Appearance>(
            AbstractPropertyEditor&,
            OpenSim::Property<OpenSim::Appearance> const& prop) {

        std::optional<AbstractPropertyEditor::Response> rv = std::nullopt;

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

            rv = AbstractPropertyEditor::Response{[newColor, opacity = static_cast<double>(rgb[3])](OpenSim::AbstractProperty& p) {
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
            rv = AbstractPropertyEditor::Response{[is_visible](OpenSim::AbstractProperty& p) {
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

    // typedef for a function that can render an editor for an AbstractProperty
    //
    // all of the above functions have a similar signature, but are specialized to `T`
    using draw_editor_type_erased_fn = std::optional<AbstractPropertyEditor::Response>(AbstractPropertyEditor&, OpenSim::AbstractProperty const&);

    // template that generates a type-erased version of the `draw_editor` functions above
    template<typename T>
    std::optional<AbstractPropertyEditor::Response> draw_editor_type_erased(
            AbstractPropertyEditor& st,
            OpenSim::AbstractProperty const& prop) {

        // call into the not-type-erased version
        return draw_editor(st, dynamic_cast<OpenSim::Property<T> const&>(prop));
    }

    // returns an entry suitable for insertion into the lookup
    template<template<typename> typename PropertyType , typename ValueType>
    constexpr std::pair<size_t, draw_editor_type_erased_fn*> entry() {
        size_t k = typeid(PropertyType<ValueType>).hash_code();
        draw_editor_type_erased_fn* f = draw_editor_type_erased<ValueType>;

        return {k, f};
    }

    // global property editor lookup table
    std::unordered_map<size_t, draw_editor_type_erased_fn*> const g_PropertyEditors = {{
        entry<OpenSim::SimpleProperty, std::string>(),
        entry<OpenSim::SimpleProperty, double>(),
        entry<OpenSim::SimpleProperty, bool>(),
        entry<OpenSim::SimpleProperty, SimTK::Vec3>(),
        entry<OpenSim::SimpleProperty, SimTK::Vec6>(),
        entry<OpenSim::ObjectProperty, OpenSim::Appearance>(),
    }};
}

std::optional<AbstractPropertyEditor::Response> osc::AbstractPropertyEditor::draw(
        OpenSim::AbstractProperty const& prop) {

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

std::optional<ObjectPropertiesEditor::Response> osc::ObjectPropertiesEditor::draw(OpenSim::Object& obj) {

    int num_props = obj.getNumProperties();
    OSC_ASSERT(num_props >= 0);

    propertyEditors.resize(static_cast<size_t>(num_props));

    std::optional<Response> rv = std::nullopt;

    ImGui::Columns(2);
    for (int i = 0; i < num_props; ++i) {
        OpenSim::AbstractProperty const& p = obj.getPropertyByIndex(i);
        AbstractPropertyEditor& substate = propertyEditors[static_cast<size_t>(i)];
        auto maybe_rv = substate.draw(p);
        if (!rv && maybe_rv) {
            rv.emplace(p, std::move(maybe_rv->updater));
        }
    }
    ImGui::Columns(1);

    return rv;
}

std::optional<ObjectPropertiesEditor::Response> osc::ObjectPropertiesEditor::draw(
        OpenSim::Object& obj,
        nonstd::span<int const> indices) {

    int highest = *std::max_element(indices.begin(), indices.end());
    int nprops = obj.getNumProperties();
    OSC_ASSERT(highest >= 0);
    OSC_ASSERT(highest < nprops);

    propertyEditors.resize(static_cast<size_t>(highest));

    std::optional<Response> rv;

    ImGui::Columns(2);
    for (int idx : indices) {
        int propidx = indices[static_cast<size_t>(idx)];
        OpenSim::AbstractProperty const& p = obj.getPropertyByIndex(propidx);
        AbstractPropertyEditor& substate = propertyEditors[static_cast<size_t>(idx)];
        auto maybe_rv = substate.draw(p);
        if (!rv && maybe_rv) {
            rv.emplace(p, std::move(maybe_rv->updater));
        }
    }
    ImGui::Columns(1);

    return rv;
}
