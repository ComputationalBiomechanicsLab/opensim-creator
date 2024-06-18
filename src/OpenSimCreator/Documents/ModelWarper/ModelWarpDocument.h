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
#include <optional>
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
        explicit ModelWarpDocument(const std::filesystem::path& osimFileLocation);
        ModelWarpDocument(const ModelWarpDocument&);
        ModelWarpDocument(ModelWarpDocument&&) noexcept;
        ModelWarpDocument& operator=(const ModelWarpDocument&);
        ModelWarpDocument& operator=(ModelWarpDocument&&) noexcept;
        ~ModelWarpDocument() noexcept;

        const OpenSim::Model& model() const;
        const IConstModelStatePair& modelstate() const;

        std::vector<WarpDetail> details(const OpenSim::Mesh&) const;
        std::vector<ValidationCheckResult> validate(const OpenSim::Mesh&) const;
        ValidationCheckState state(const OpenSim::Mesh&) const;
        IPointWarperFactory const* findMeshWarp(const OpenSim::Mesh&) const;

        std::vector<WarpDetail> details(const OpenSim::PhysicalOffsetFrame&) const;
        std::vector<ValidationCheckResult> validate(const OpenSim::PhysicalOffsetFrame&) const;
        ValidationCheckState state(const OpenSim::PhysicalOffsetFrame&) const;

        float getWarpBlendingFactor() const;
        void setWarpBlendingFactor(float);

        bool getShouldWriteWarpedMeshesToDisk() const;
        void setShouldWriteWarpedMeshesToDisk(bool);

        std::optional<std::filesystem::path> getWarpedMeshesOutputDirectory() const;

        std::optional<std::filesystem::path> getOsimFileLocation() const;

        ValidationCheckState state() const;

        // only checks reference equality by leaning on the copy-on-write behavior
        friend bool operator==(const ModelWarpDocument&, const ModelWarpDocument&) = default;
    private:
        std::vector<ValidationCheckResult> implValidate() const;

        CopyOnUpdPtr<BasicModelStatePair> m_ModelState;
        CopyOnUpdPtr<ModelWarpConfiguration> m_ModelWarpConfig;
        CopyOnUpdPtr<PointWarperFactories> m_MeshWarpLookup;
        CopyOnUpdPtr<FrameWarperFactories> m_FrameWarpLookup;
    };
}
