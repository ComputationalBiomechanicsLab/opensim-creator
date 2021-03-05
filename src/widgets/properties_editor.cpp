#include "properties_editor.hpp"

#include <OpenSim/Common/AbstractProperty.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Appearance.h>
#include <SimTKcommon.h>
#include <imgui.h>

#include <cstring>
#include <string>

template<typename Coll1, typename Coll2>
static float diff(Coll1 const& older, Coll2 const& newer, size_t n) {
    for (int i = 0; i < n; ++i) {
        if (older[i] != newer[i]) {
            return newer[i];
        }
    }
    return older[0];
}

bool osmv::Properties_editor::draw(OpenSim::Component& component) {
    ImGui::Columns(2);
    property_locked.resize(static_cast<size_t>(component.getNumProperties()), true);
    for (int i = 0; i < component.getNumProperties(); ++i) {
        OpenSim::AbstractProperty& p = component.updPropertyByIndex(i);
        ImGui::Text("%s", p.getName().c_str());
        ImGui::NextColumn();

        ImGui::PushID(p.getName().c_str());
        bool editor_rendered = false;

        // try string
        {
            auto* sp = dynamic_cast<OpenSim::Property<std::string>*>(&p);
            if (sp) {
                char buf[64]{};
                buf[sizeof(buf) - 1] = '\0';
                std::strncpy(buf, sp->getValue().c_str(), sizeof(buf) - 1);

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText("##stringeditor", buf, sizeof(buf));

                editor_rendered = true;
            }
        }

        // try double
        {
            auto* dp = dynamic_cast<OpenSim::Property<double>*>(&p);

            // it's a *single* double
            if (dp and not dp->isListProperty()) {
                float v = static_cast<float>(dp->getValue());

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::InputFloat("##doubleditor", &v)) {
                    dp->setValue(static_cast<double>(v));
                }

                editor_rendered = true;
            }

            // it's two doubles
            if (dp and dp->isListProperty() and dp->size() == 2) {
                // lock btn
                bool locked = property_locked[static_cast<size_t>(i)];
                if (ImGui::Checkbox("##vec2lockbtn", &locked)) {
                    property_locked[static_cast<size_t>(i)] = locked;
                }
                ImGui::SameLine();

                float vs[2] = {static_cast<float>(dp->getValue(0)), static_cast<float>(dp->getValue(1))};
                float old[2] = {vs[0], vs[1]};

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::InputFloat2("##vec2editor", vs)) {
                    if (locked) {
                        float nv = diff(old, vs, 2);
                        dp->setValue(0, static_cast<double>(nv));
                        dp->setValue(1, static_cast<double>(nv));
                    } else {
                        dp->setValue(0, static_cast<double>(vs[0]));
                        dp->setValue(1, static_cast<double>(vs[1]));
                    }
                }

                editor_rendered = true;
            }
        }

        // try bool
        {
            auto* bp = dynamic_cast<OpenSim::Property<bool>*>(&p);

            if (bp and not bp->isListProperty()) {
                bool v = bp->getValue();

                if (ImGui::Checkbox("##booleditor", &v)) {
                    bp->setValue(v);
                }

                editor_rendered = true;
            }
        }

        // try Vec3
        {
            auto* vp = dynamic_cast<OpenSim::Property<SimTK::Vec3>*>(&p);
            if (vp and not vp->isListProperty()) {
                // lock btn
                bool locked = property_locked[static_cast<size_t>(i)];
                if (ImGui::Checkbox("##vec2lockbtn", &locked)) {
                    property_locked[static_cast<size_t>(i)] = locked;
                }
                ImGui::SameLine();

                SimTK::Vec3 const& v = vp->getValue();
                float vs[3];
                vs[0] = static_cast<float>(v[0]);
                vs[1] = static_cast<float>(v[1]);
                vs[2] = static_cast<float>(v[2]);

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::InputFloat3("##vec3editor", vs)) {
                    if (locked) {
                        double locked_val = static_cast<double>(diff(vp->getValue(), vs, 3));
                        vp->setValue(SimTK::Vec3{locked_val, locked_val, locked_val});
                    } else {
                        vp->setValue(SimTK::Vec3{
                            static_cast<double>(vs[0]), static_cast<double>(vs[1]), static_cast<double>(vs[2])});
                    }
                }

                editor_rendered = true;
            }
        }

        // try Vec6 (e.g. body inertia)
        {
            auto* vp = dynamic_cast<OpenSim::Property<SimTK::Vec6>*>(&p);
            if (vp and not vp->isListProperty()) {
                SimTK::Vec6 const& v = vp->getValue();
                float vs[6];
                vs[0] = static_cast<float>(v[0]);
                vs[1] = static_cast<float>(v[1]);
                vs[2] = static_cast<float>(v[2]);
                vs[3] = static_cast<float>(v[3]);
                vs[4] = static_cast<float>(v[4]);
                vs[5] = static_cast<float>(v[5]);

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::InputFloat3("##vec6editor_a", vs)) {
                    vp->setValue(SimTK::Vec6{static_cast<double>(vs[0]),
                                             static_cast<double>(vs[1]),
                                             static_cast<double>(vs[2]),
                                             static_cast<double>(vs[3]),
                                             static_cast<double>(vs[4]),
                                             static_cast<double>(vs[5])});
                }

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::InputFloat3("##vec6editor_b", vs + 3)) {
                    vp->setValue(SimTK::Vec6{static_cast<double>(vs[0]),
                                             static_cast<double>(vs[1]),
                                             static_cast<double>(vs[2]),
                                             static_cast<double>(vs[3]),
                                             static_cast<double>(vs[4]),
                                             static_cast<double>(vs[5])});
                }

                editor_rendered = true;
            }
        }

        // try Appearance
        if (auto* ptr = dynamic_cast<OpenSim::Property<OpenSim::Appearance>*>(&p); ptr) {
            OpenSim::Appearance& app = ptr->updValue();
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
                app.set_color(newColor);
                app.set_opacity(static_cast<double>(rgb[3]));
            }

            bool is_visible = app.get_visible();
            if (ImGui::Checkbox("is visible", &is_visible)) {
                app.set_visible(is_visible);
            }

            editor_rendered = true;
        }

        if (not editor_rendered) {
            ImGui::Text("%s", p.toString().c_str());
        }

        ImGui::Dummy(ImVec2{0.0f, 1.0f});

        ImGui::PopID();

        ImGui::NextColumn();
    }
    ImGui::Columns(1);

    return false;
}
