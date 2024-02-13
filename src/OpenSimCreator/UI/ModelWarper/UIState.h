#pragma once

#include <OpenSimCreator/Documents/ModelWarper/Document.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheck.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationState.h>
#include <OpenSimCreator/Documents/ModelWarper/WarpDetail.h>

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
        OpenSim::Model const& model() const { return m_Document->model(); }

        std::vector<WarpDetail> details(OpenSim::Mesh const& mesh) const { return m_Document->details(mesh); }
        std::vector<ValidationCheck> validate(OpenSim::Mesh const& mesh) const { return m_Document->validate(mesh); }
        ValidationState state(OpenSim::Mesh const& mesh) const { return m_Document->state(mesh); }

        std::vector<WarpDetail> details(OpenSim::PhysicalOffsetFrame const& pof) const { return m_Document->details(pof); }
        std::vector<ValidationCheck> validate(OpenSim::PhysicalOffsetFrame const& pof) const { return m_Document->validate(pof); }
        ValidationState state(OpenSim::PhysicalOffsetFrame const& pof) const { return m_Document->state(pof); }

        void actionOpenOsimOrPromptUser(
            std::optional<std::filesystem::path> maybeOsimPath = std::nullopt
        );
    private:
        std::shared_ptr<Document> m_Document = std::make_shared<Document>();
    };
}
