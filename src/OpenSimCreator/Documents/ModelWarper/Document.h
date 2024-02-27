#pragma once

#include <OpenSimCreator/Documents/Model/BasicModelStatePair.h>
#include <OpenSimCreator/Documents/Model/IConstModelStatePair.h>
#include <OpenSimCreator/Documents/ModelWarper/FrameWarpLookup.h>
#include <OpenSimCreator/Documents/ModelWarper/IValidateable.h>
#include <OpenSimCreator/Documents/ModelWarper/MeshWarpLookup.h>
#include <OpenSimCreator/Documents/ModelWarper/ModelWarpConfiguration.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheck.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationState.h>
#include <OpenSimCreator/Documents/ModelWarper/WarpDetail.h>

#include <oscar/Utils/CopyOnUpdPtr.h>

#include <filesystem>
#include <vector>

namespace OpenSim { class Mesh; }
namespace OpenSim { class Model; }
namespace OpenSim { class PhysicalOffsetFrame; }

namespace osc::mow
{
    // the document that the model warping UI represents/manipulates
    class Document final : public IValidateable {
    public:
        Document();
        explicit Document(std::filesystem::path const& osimFileLocation);
        Document(Document const&);
        Document(Document&&) noexcept;
        Document& operator=(Document const&);
        Document& operator=(Document&&) noexcept;
        ~Document() noexcept;

        OpenSim::Model const& model() const;
        IConstModelStatePair const& modelstate() const;

        std::vector<WarpDetail> details(OpenSim::Mesh const&) const;
        std::vector<ValidationCheck> validate(OpenSim::Mesh const&) const;
        ValidationState state(OpenSim::Mesh const&) const;

        std::vector<WarpDetail> details(OpenSim::PhysicalOffsetFrame const&) const;
        std::vector<ValidationCheck> validate(OpenSim::PhysicalOffsetFrame const&) const;
        ValidationState state(OpenSim::PhysicalOffsetFrame const&) const;

        ValidationState state() const;

        // only checks reference equality by leaning on the copy-on-write behavior
        friend bool operator==(Document const&, Document const&) = default;
    private:
        std::vector<ValidationCheck> implValidate() const;

        CopyOnUpdPtr<BasicModelStatePair> m_ModelState;
        CopyOnUpdPtr<ModelWarpConfiguration> m_ModelWarpConfig;
        CopyOnUpdPtr<MeshWarpLookup> m_MeshWarpLookup;
        CopyOnUpdPtr<FrameWarpLookup> m_FrameWarpLookup;
    };
}
