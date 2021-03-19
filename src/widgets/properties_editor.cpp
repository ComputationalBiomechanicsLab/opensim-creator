#include "properties_editor.hpp"

#include "src/assertions.hpp"
#include "src/widgets/lockable_f3_editor.hpp"

#include <OpenSim/Common/AbstractProperty.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Appearance.h>
#include <SimTKcommon.h>
#include <imgui.h>

#include <cstring>
#include <string>

template<typename Coll1, typename Coll2>
static float diff(Coll1 const& older, Coll2 const& newer, size_t n) {
    for (int i = 0; i < static_cast<int>(n); ++i) {
        if (static_cast<float>(older[i]) != static_cast<float>(newer[i])) {
            return newer[i];
        }
    }
    return static_cast<float>(older[0]);
}

void osmv::draw_properties_editor(
    Properties_editor_state& st,
    OpenSim::Component& component,
    std::function<void()> const& before_property_edited,
    std::function<void()> const& after_property_edited) {

    ImGui::Columns(2);
    st.property_locked.resize(static_cast<size_t>(component.getNumProperties()), true);

    for (int i = 0; i < component.getNumProperties(); ++i) {
        OpenSim::AbstractProperty const& p = component.getPropertyByIndex(i);
        ImGui::Text("%s", p.getName().c_str());
        ImGui::NextColumn();

        ImGui::PushID(p.getName().c_str());
        bool editor_rendered = false;

        // try string
        {
            auto const* sp = dynamic_cast<OpenSim::Property<std::string> const*>(&p);
            if (sp) {
                char buf[64]{};
                buf[sizeof(buf) - 1] = '\0';
                std::strncpy(buf, sp->getValue().c_str(), sizeof(buf) - 1);

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText("##stringeditor", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue);

                editor_rendered = true;
            }
        }

        // try double
        {
            auto const* dp = dynamic_cast<OpenSim::Property<double> const*>(&p);

            // it's a *single* double
            if (dp && !dp->isListProperty()) {
                float v = static_cast<float>(dp->getValue());

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::InputFloat("##doubleditor", &v, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
                    before_property_edited();
                    const_cast<OpenSim::Property<double>*>(dp)->setValue(static_cast<double>(v));
                    after_property_edited();
                }

                editor_rendered = true;
            }

            // it's two doubles
            if (dp && dp->isListProperty() && dp->size() == 2) {
                // lock btn
                bool locked = st.property_locked[static_cast<size_t>(i)];
                if (ImGui::Checkbox("##vec2lockbtn", &locked)) {
                    st.property_locked[static_cast<size_t>(i)] = locked;
                }
                ImGui::SameLine();

                float vs[2] = {static_cast<float>(dp->getValue(0)), static_cast<float>(dp->getValue(1))};
                float old[2] = {vs[0], vs[1]};

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::InputFloat2("##vec2editor", vs, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
                    before_property_edited();
                    auto* mdp = const_cast<OpenSim::Property<double>*>(dp);
                    if (locked) {
                        float nv = diff(old, vs, 2);
                        mdp->setValue(0, static_cast<double>(nv));
                        mdp->setValue(1, static_cast<double>(nv));
                    } else {
                        mdp->setValue(0, static_cast<double>(vs[0]));
                        mdp->setValue(1, static_cast<double>(vs[1]));
                    }
                    after_property_edited();
                }

                editor_rendered = true;
            }
        }

        // try bool
        {
            auto const* bp = dynamic_cast<OpenSim::Property<bool> const*>(&p);

            if (bp && !bp->isListProperty()) {
                bool v = bp->getValue();

                if (ImGui::Checkbox("##booleditor", &v)) {
                    before_property_edited();
                    const_cast<OpenSim::Property<bool>*>(bp)->setValue(v);
                    after_property_edited();
                }

                editor_rendered = true;
            }
        }

        // try Vec3
        {
            auto const* vp = dynamic_cast<OpenSim::Property<SimTK::Vec3> const*>(&p);
            if (vp && !vp->isListProperty()) {
                SimTK::Vec3 v = vp->getValue();
                float fv[3] = {
                    static_cast<float>(v[0]),
                    static_cast<float>(v[1]),
                    static_cast<float>(v[2]),
                };

                bool locked = st.property_locked[static_cast<size_t>(i)];

                if (draw_lockable_f3_editor("##vec3lockbtn", "##vec3editor", fv, &locked)) {
                    before_property_edited();
                    st.property_locked[static_cast<size_t>(i)] = locked;
                    v[0] = static_cast<double>(fv[0]);
                    v[1] = static_cast<double>(fv[1]);
                    v[2] = static_cast<double>(fv[2]);
                    const_cast<OpenSim::Property<SimTK::Vec3>*>(vp)->setValue(v);
                    after_property_edited();
                }

                editor_rendered = true;
            }
        }

        // try Vec6 (e.g. body inertia)
        {
            auto const* vp = dynamic_cast<OpenSim::Property<SimTK::Vec6> const*>(&p);
            if (vp && !vp->isListProperty()) {
                SimTK::Vec6 const& v = vp->getValue();
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
                    const_cast<OpenSim::Property<SimTK::Vec6>*>(vp)->setValue(SimTK::Vec6{static_cast<double>(vs[0]),
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
                    const_cast<OpenSim::Property<SimTK::Vec6>*>(vp)->setValue(SimTK::Vec6{static_cast<double>(vs[0]),
                                                                                          static_cast<double>(vs[1]),
                                                                                          static_cast<double>(vs[2]),
                                                                                          static_cast<double>(vs[3]),
                                                                                          static_cast<double>(vs[4]),
                                                                                          static_cast<double>(vs[5])});
                    after_property_edited();
                }

                editor_rendered = true;
            }
        }

        // try Appearance
        if (auto const* ptr = dynamic_cast<OpenSim::Property<OpenSim::Appearance> const*>(&p); ptr) {
            OpenSim::Appearance const& app = ptr->getValue();
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
                const_cast<OpenSim::Appearance&>(app).set_color(newColor);
                const_cast<OpenSim::Appearance&>(app).set_opacity(static_cast<double>(rgb[3]));
                after_property_edited();
            }

            bool is_visible = app.get_visible();
            if (ImGui::Checkbox("is visible", &is_visible)) {
                before_property_edited();
                const_cast<OpenSim::Appearance&>(app).set_visible(is_visible);
                after_property_edited();
            }

            editor_rendered = true;
        }

        if (!editor_rendered) {
            ImGui::Text("%s", p.toString().c_str());
        }

        ImGui::Dummy(ImVec2{0.0f, 1.0f});

        ImGui::PopID();

        ImGui::NextColumn();
    }
    ImGui::Columns(1);
}
