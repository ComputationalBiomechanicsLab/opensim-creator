#pragma once

#include <memory>
#include <string_view>

namespace osc { class MeshWarpingTabSharedState; }

namespace osc
{
    // the top toolbar (contains icons for new, save, open, undo, redo, etc.)
    class MeshWarpingTabToolbar final {
    public:
        MeshWarpingTabToolbar(
            std::string_view label,
            std::shared_ptr<MeshWarpingTabSharedState>
        );
        MeshWarpingTabToolbar(const MeshWarpingTabToolbar&) = delete;
        MeshWarpingTabToolbar(MeshWarpingTabToolbar&&) noexcept;
        MeshWarpingTabToolbar& operator=(const MeshWarpingTabToolbar&) = delete;
        MeshWarpingTabToolbar& operator=(MeshWarpingTabToolbar&&) noexcept;
        ~MeshWarpingTabToolbar() noexcept;

        void onDraw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
