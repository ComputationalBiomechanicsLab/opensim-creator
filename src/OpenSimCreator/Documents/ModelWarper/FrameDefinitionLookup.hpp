#pragma once

#include <OpenSimCreator/Documents/ModelWarper/IWarpableFrameDefinition.hpp>

#include <oscar/Utils/ClonePtr.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace OpenSim { class Model; }
namespace osc::mow { class ModelWarpConfiguration; }

namespace osc::mow
{
    class FrameDefinitionLookup final {
    public:
        FrameDefinitionLookup() = default;

        FrameDefinitionLookup(OpenSim::Model const&, ModelWarpConfiguration const&);

        IWarpableFrameDefinition const* lookup(std::string const& frameComponentAbsPath) const
        {
            if (auto const it = m_Lut.find(frameComponentAbsPath); it != m_Lut.end())
            {
                return it->second.get();
            }
            else
            {
                return nullptr;
            }
        }
    private:
        std::unordered_map<std::string, ClonePtr<IWarpableFrameDefinition>> m_Lut;
    };
}
