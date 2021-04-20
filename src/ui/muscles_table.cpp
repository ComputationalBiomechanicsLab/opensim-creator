#include "muscles_table.hpp"

#include "src/assertions.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>

#include <imgui.h>

static constexpr char const* names[] = OSC_MUSCLE_SORT_NAMES;

osc::ui::muscles_table::Response
    osc::ui::muscles_table::draw(State& st, OpenSim::Model const& model, SimTK::State const& stkst) {

    Response rv;

    // extract muscles details from model
    st.muscles.clear();
    for (OpenSim::Muscle const& musc : model.getComponentList<OpenSim::Muscle>()) {
        st.muscles.push_back(&musc);
    }

    ImGui::Text("filters:");
    ImGui::Dummy({0.0f, 2.5f});
    ImGui::Separator();

    ImGui::Columns(2);

    ImGui::Text("search");
    ImGui::NextColumn();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputText("##muscles search filter", st.filter, sizeof(st.filter));
    ImGui::NextColumn();

    ImGui::Text("min length");
    ImGui::NextColumn();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputFloat("##muscles min filter", &st.min_len);
    ImGui::NextColumn();

    ImGui::Text("max length");
    ImGui::NextColumn();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputFloat("##muscles max filter", &st.max_len);
    ImGui::NextColumn();

    ImGui::Text("inverse length range");
    ImGui::NextColumn();
    ImGui::Checkbox("##muscles inverse range filter", &st.inverse_range);
    ImGui::NextColumn();

    ImGui::Text("sort by");
    ImGui::NextColumn();
    ImGui::PushID("muscles sort by checkbox");
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    if (ImGui::BeginCombo(" ", names[st.sort_choice], ImGuiComboFlags_None)) {
        for (int n = 0; n < SortChoice_NUM_CHOICES; n++) {
            bool is_selected = (st.sort_choice == n);
            if (ImGui::Selectable(names[n], is_selected)) {
                st.sort_choice = n;
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopID();
    ImGui::NextColumn();

    ImGui::Text("reverse results");
    ImGui::NextColumn();
    ImGui::Checkbox("##muscles reverse results chechbox", &st.reverse_results);
    ImGui::NextColumn();

    ImGui::Columns();

    // all user filters handled, transform the muscle list accordingly.

    // filter muscle list
    {
        auto filter_fn = [&](OpenSim::Muscle const* m) {
            bool in_range = st.min_len <= static_cast<float>(m->getLength(stkst)) &&
                            static_cast<float>(m->getLength(stkst)) <= st.max_len;

            if (st.inverse_range) {
                in_range = !in_range;
            }

            if (!in_range) {
                return true;
            }

            bool matches_filter = m->getName().find(st.filter) != m->getName().npos;

            return !matches_filter;
        };

        auto it = std::remove_if(st.muscles.begin(), st.muscles.end(), filter_fn);
        st.muscles.erase(it, st.muscles.end());
    }

    // sort muscle list
    OSC_ASSERT(st.sort_choice < SortChoice_NUM_CHOICES);
    switch (st.sort_choice) {
    case SortChoice_Length: {
        std::sort(st.muscles.begin(), st.muscles.end(), [&stkst](OpenSim::Muscle const* m1, OpenSim::Muscle const* m2) {
            return m1->getLength(stkst) > m2->getLength(stkst);
        });
        break;
    }
    case SortChoice_Strain: {
        std::sort(st.muscles.begin(), st.muscles.end(), [&stkst](OpenSim::Muscle const* m1, OpenSim::Muscle const* m2) {
            return m1->getTendonStrain(stkst) > m2->getTendonStrain(stkst);
        });
        break;
    }
    default:
        break;  // skip sorting
    }

    // reverse list (if necessary)
    if (st.reverse_results) {
        std::reverse(st.muscles.begin(), st.muscles.end());
    }

    ImGui::Dummy({0.0f, 20.0f});
    ImGui::Text("results (%zu):", st.muscles.size());
    ImGui::Dummy({0.0f, 2.5f});
    ImGui::Separator();

    // muscle table header
    ImGui::Columns(4);
    ImGui::Text("name");
    ImGui::NextColumn();
    ImGui::Text("length");
    ImGui::NextColumn();
    ImGui::Text("strain");
    ImGui::NextColumn();
    ImGui::Text("force");
    ImGui::NextColumn();
    ImGui::Columns();
    ImGui::Separator();

    // muscle table rows
    ImGui::Columns(4);
    for (OpenSim::Muscle const* musc : st.muscles) {
        ImGui::Text("%s", musc->getName().c_str());
        if (ImGui::IsItemHovered()) {
            rv.type = HoverChanged;
            rv.ptr = musc;
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            rv.type = SelectionChanged;
            rv.ptr = musc;
        }
        ImGui::NextColumn();
        ImGui::Text("%.3f", static_cast<double>(musc->getLength(stkst)));
        ImGui::NextColumn();
        ImGui::Text("%.3f", 100.0 * musc->getTendonStrain(stkst));
        ImGui::NextColumn();
        ImGui::Text("%.3f", musc->getTendonForce(stkst));
        ImGui::NextColumn();
    }
    ImGui::Columns();

    return rv;
}
