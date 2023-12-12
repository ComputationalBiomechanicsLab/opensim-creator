#pragma once

#include <OpenSimCreator/Documents/Frames/FrameDefinition.hpp>

#include <cstddef>
#include <utility>
#include <vector>

namespace osc::frames
{
    class FramesFile final {
    public:
        FramesFile(std::vector<FrameDefinition> frameDefs_) :
            m_FrameDefs{std::move(frameDefs_)}
        {
        }

        bool hasFrameDefinitions() const
        {
            return !m_FrameDefs.empty();
        }

        size_t getNumFrameDefinitions() const
        {
            return m_FrameDefs.size();
        }

        FrameDefinition const& getFrameDefinition(size_t i) const
        {
            return m_FrameDefs.at(i);
        }
    private:
        std::vector<FrameDefinition> m_FrameDefs;
    };
}
