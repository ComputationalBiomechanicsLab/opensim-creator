#pragma once

#include <OpenSimCreator/Documents/Model/BasicModelStatePair.h>
#include <OpenSimCreator/Documents/Model/IConstModelStatePair.h>
#include <OpenSimCreator/Documents/ModelWarper/FrameWarperFactories.h>
#include <OpenSimCreator/Documents/ModelWarper/IValidateable.h>
#include <OpenSimCreator/Documents/ModelWarper/PointWarperFactories.h>
#include <OpenSimCreator/Documents/ModelWarper/ModelWarpConfiguration.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckResult.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckState.h>
#include <OpenSimCreator/Documents/ModelWarper/WarpDetail.h>

#include <oscar/Utils/CopyOnUpdPtr.h>

#include <filesystem>
#include <vector>

namespace OpenSim { class Mesh; }
namespace OpenSim { class Model; }
namespace OpenSim { class PhysicalOffsetFrame; }

namespace osc::mow
{
    // top-level model warping document that contains all the necessary state to render
    // the model warping UI and can, if valid, contain all the necessary state to warp
    // an OpenSim model
    class ModelWarpDocument final : public IValidateable {
    public:
        ModelWarpDocument();
        explicit ModelWarpDocument(std::filesystem::path const& osimFileLocation);
        ModelWarpDocument(ModelWarpDocument const&);
        ModelWarpDocument(ModelWarpDocument&&) noexcept;
        ModelWarpDocument& operator=(ModelWarpDocument const&);
        ModelWarpDocument& operator=(ModelWarpDocument&&) noexcept;
        ~ModelWarpDocument() noexcept;

        OpenSim::Model const& model() const;
        IConstModelStatePair const& modelstate() const;

        std::vector<WarpDetail> details(OpenSim::Mesh const&) const;
        std::vector<ValidationCheckResult> validate(OpenSim::Mesh const&) const;
        ValidationCheckState state(OpenSim::Mesh const&) const;
        IPointWarperFactory const* findMeshWarp(OpenSim::Mesh const&) const;

        std::vector<WarpDetail> details(OpenSim::PhysicalOffsetFrame const&) const;
        std::vector<ValidationCheckResult> validate(OpenSim::PhysicalOffsetFrame const&) const;
        ValidationCheckState state(OpenSim::PhysicalOffsetFrame const&) const;

        float getWarpBlendingFactor() const;
        void setWarpBlendingFactor(float);

        ValidationCheckState state() const;

        // only checks reference equality by leaning on the copy-on-write behavior
        friend bool operator==(ModelWarpDocument const&, ModelWarpDocument const&) = default;
    private:
        std::vector<ValidationCheckResult> implValidate() const;

        CopyOnUpdPtr<BasicModelStatePair> m_ModelState;
        CopyOnUpdPtr<ModelWarpConfiguration> m_ModelWarpConfig;
        CopyOnUpdPtr<PointWarperFactories> m_MeshWarpLookup;
        CopyOnUpdPtr<FrameWarperFactories> m_FrameWarpLookup;
    };
}
