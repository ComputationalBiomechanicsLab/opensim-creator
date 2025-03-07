#pragma once

#include <libopensimcreator/Documents/Model/IModelStatePair.h>
#include <libopensimcreator/Documents/ModelWarper/CachedModelWarper.h>
#include <libopensimcreator/Documents/ModelWarper/ValidationCheckResult.h>
#include <libopensimcreator/Documents/ModelWarper/ValidationCheckState.h>
#include <libopensimcreator/Documents/ModelWarper/WarpableModel.h>
#include <libopensimcreator/Documents/ModelWarper/WarpDetail.h>

#include <liboscar/Maths/PolarPerspectiveCamera.h>
#include <liboscar/Platform/Widget.h>
#include <liboscar/Utils/LifetimedPtr.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

namespace OpenSim { class Mesh; }
namespace OpenSim { class Model; }
namespace OpenSim { class PhysicalOffsetFrame; }

namespace osc::mow
{
    class UIState final {
    public:
        explicit UIState(Widget* parent) :
            m_Parent{parent}
        {}

        const OpenSim::Model& model() const { return m_Document->getModel(); }
        const IModelStatePair& modelstate() const { return *m_Document; }
        std::shared_ptr<IModelStatePair> modelstatePtr() { return m_Document; }

        std::vector<WarpDetail> details(const OpenSim::Mesh& mesh) const { return m_Document->details(mesh); }
        std::vector<ValidationCheckResult> validate(const OpenSim::Mesh& mesh) const { return m_Document->validate(mesh); }
        ValidationCheckState state(const OpenSim::Mesh& mesh) const { return m_Document->state(mesh); }

        std::vector<WarpDetail> details(const OpenSim::PhysicalOffsetFrame& pof) const { return m_Document->details(pof); }
        std::vector<ValidationCheckResult> validate(const OpenSim::PhysicalOffsetFrame& pof) const { return m_Document->validate(pof); }
        ValidationCheckState state(const OpenSim::PhysicalOffsetFrame& pof) const { return m_Document->state(pof); }

        float getWarpBlendingFactor() const { return m_Document->getWarpBlendingFactor(); }
        void setWarpBlendingFactor(float v) { m_Document->setWarpBlendingFactor(v); }

        bool isCameraLinked() const { return m_LinkCameras; }
        void setCameraLinked(bool v) { m_LinkCameras = v; }
        bool isOnlyCameraRotationLinked() const { return m_OnlyLinkRotation; }
        void setOnlyCameraRotationLinked(bool v) { m_OnlyLinkRotation = v; }
        const PolarPerspectiveCamera& getLinkedCamera() const { return m_LinkedCamera; }
        void setLinkedCamera(const PolarPerspectiveCamera& camera) { m_LinkedCamera = camera; }

        ValidationCheckState state() const { return m_Document->state(); }
        bool canWarpModel() const { return state() != ValidationCheckState::Error; }
        std::shared_ptr<IModelStatePair> tryGetWarpedModel()
        {
            if (canWarpModel()) {
                return m_ModelWarper.warp(*m_Document);
            }
            else {
                return nullptr;
            }
        }

        void actionOpenOsimOrPromptUser(
            std::optional<std::filesystem::path> maybeOsimPath = std::nullopt
        );

        void actionWarpModelAndOpenInModelEditor();
    private:
        Widget* m_Parent;
        std::shared_ptr<WarpableModel> m_Document = std::make_shared<WarpableModel>();
        CachedModelWarper m_ModelWarper;

        bool m_LinkCameras = true;
        bool m_OnlyLinkRotation = false;
        PolarPerspectiveCamera m_LinkedCamera;
    };
}
