#pragma once

#include <OpenSimCreator/Documents/Model/IConstModelStatePair.h>
#include <OpenSimCreator/Documents/ModelWarper/CachedModelWarper.h>
#include <OpenSimCreator/Documents/ModelWarper/Document.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheck.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationState.h>
#include <OpenSimCreator/Documents/ModelWarper/WarpDetail.h>
#include <oscar/UI/Tabs/ITabHost.h>
#include <oscar/Utils/ParentPtr.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

namespace OpenSim { class Mesh; }
namespace OpenSim { class Model; }
namespace OpenSim { class PhysicalOffsetFrame; }
namespace osc { template<typename> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc::mow
{
    class UIState final {
    public:
        explicit UIState(ParentPtr<ITabHost> const& tabHost) :
            m_TabHost{tabHost}
        {}

        OpenSim::Model const& model() const { return m_Document->model(); }
        IConstModelStatePair const& modelstate() const { return m_Document->modelstate(); }

        std::vector<WarpDetail> details(OpenSim::Mesh const& mesh) const { return m_Document->details(mesh); }
        std::vector<ValidationCheck> validate(OpenSim::Mesh const& mesh) const { return m_Document->validate(mesh); }
        ValidationState state(OpenSim::Mesh const& mesh) const { return m_Document->state(mesh); }

        std::vector<WarpDetail> details(OpenSim::PhysicalOffsetFrame const& pof) const { return m_Document->details(pof); }
        std::vector<ValidationCheck> validate(OpenSim::PhysicalOffsetFrame const& pof) const { return m_Document->validate(pof); }
        ValidationState state(OpenSim::PhysicalOffsetFrame const& pof) const { return m_Document->state(pof); }

        float getWarpBlendingFactor() const { return m_Document->getWarpBlendingFactor(); }
        void setWarpBlendingFactor(float v) { m_Document->setWarpBlendingFactor(v); }

        ValidationState state() const { return m_Document->state(); }
        bool canWarpModel() const { return state() != ValidationState::Error; }
        std::shared_ptr<IConstModelStatePair const> tryGetWarpedModel()
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
        ParentPtr<ITabHost> m_TabHost;
        std::shared_ptr<Document> m_Document = std::make_shared<Document>();
        CachedModelWarper m_ModelWarper;
    };
}
