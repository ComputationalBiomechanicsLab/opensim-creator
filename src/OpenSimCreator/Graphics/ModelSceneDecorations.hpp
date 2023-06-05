#pragma once

#include <oscar/Graphics/SceneCollision.hpp>
#include <oscar/Graphics/SceneDecoration.hpp>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/BVH.hpp>
#include <oscar/Maths/Rect.hpp>

#include <glm/vec2.hpp>
#include <nonstd/span.hpp>

#include <cstddef>
#include <optional>
#include <vector>

namespace osc { struct PolarPerspectiveCamera; }

namespace osc
{
    class ModelSceneDecorations final {
    public:
        ModelSceneDecorations();
        void clear();
        void reserve(size_t);
        void computeBVH();

        nonstd::span<SceneDecoration const> getDrawlist() const
        {
            return m_Drawlist;
        }

        void push_back(SceneDecoration&& decoration)
        {
            m_Drawlist.push_back(decoration);
        }

        size_t size() const
        {
            return m_Drawlist.size();
        }

        SceneDecoration const& operator[](size_t i) const
        {
            return m_Drawlist[i];
        }

        BVH const& getBVH() const
        {
            return m_BVH;
        }

        std::optional<AABB> getRootAABB() const;

        std::optional<SceneCollision> getClosestCollision(
            PolarPerspectiveCamera const&,
            glm::vec2 mouseScreenPos,
            Rect const& viewportScreenRect
        ) const;

    private:
        std::vector<SceneDecoration> m_Drawlist;
        BVH m_BVH;
    };
}