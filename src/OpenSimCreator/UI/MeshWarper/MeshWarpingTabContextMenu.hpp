#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementID.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.hpp>

#include <imgui.h>
#include <oscar/UI/Widgets/StandardPopup.hpp>

#include <memory>
#include <string_view>
#include <utility>

namespace osc
{
    class MeshWarpingTabContextMenu final : public StandardPopup {
    public:
        MeshWarpingTabContextMenu(
            std::string_view label_,
            std::shared_ptr<MeshWarpingTabSharedState> shared_,
            TPSDocumentElementID rightClickedID_) :
            StandardPopup{label_, {10.0f, 10.0f}, ImGuiWindowFlags_NoMouseInputs},
            m_Shared{std::move(shared_)},
            m_ElementID{std::move(rightClickedID_)}
        {
            setModal(false);
        }
    private:
        void implDrawContent() final
        {
            // TODO: lookup scene element
            // - provide way of renaming it (or both, if paired)
            // - provide way of deleting it
            ImGui::Text("hello");
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_Shared;
        TPSDocumentElementID m_ElementID;
    };
}
