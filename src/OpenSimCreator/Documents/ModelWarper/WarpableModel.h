#pragma once

#include <OpenSimCreator/Documents/Model/BasicModelStatePair.h>
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
    // a top-level datastructure that can produce a warped `OpenSim::Model` from
    // appropriate inputs
    //
    // i.e. this ties together:
    //
    // - an input `OpenSim::Model`
    // - (optional) a warp configuration, which tells the engine how to warp the model
    //
    // because this may be polled or used by the UI, it may (hopefully, temporarily) be
    // in an error/warning state that the user is expected to resolve at runtime
    class WarpableModel final : public IValidateable {
    public:
        WarpableModel();
        explicit WarpableModel(const std::filesystem::path& osimFileLocation);
        WarpableModel(const WarpableModel&);
        WarpableModel(WarpableModel&&) noexcept;
        WarpableModel& operator=(const WarpableModel&);
        WarpableModel& operator=(WarpableModel&&) noexcept;
        ~WarpableModel() noexcept;

        const OpenSim::Model& model() const;
        const IModelStatePair& modelstate() const;

        std::vector<WarpDetail> details(const OpenSim::Mesh&) const;
        std::vector<ValidationCheckResult> validate(const OpenSim::Mesh&) const;
        ValidationCheckState state(const OpenSim::Mesh&) const;
        const IPointWarperFactory* findMeshWarp(const OpenSim::Mesh&) const;

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

        // returns `true` if both the left- and right-hand side _point_ to the same information
        friend bool operator==(const WarpableModel&, const WarpableModel&) = default;
    private:
        std::vector<ValidationCheckResult> implValidate(const WarpableModel&) const final;

        CopyOnUpdPtr<BasicModelStatePair> m_ModelState;
        CopyOnUpdPtr<ModelWarpConfiguration> m_ModelWarpConfig;
        CopyOnUpdPtr<PointWarperFactories> m_MeshWarpLookup;
        CopyOnUpdPtr<FrameWarperFactories> m_FrameWarpLookup;
    };
}
