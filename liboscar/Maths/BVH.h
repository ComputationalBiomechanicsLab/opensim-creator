#pragma once

#include <liboscar/Maths/AABB.h>
#include <liboscar/Maths/BVHCollision.h>
#include <liboscar/Maths/BVHNode.h>
#include <liboscar/Maths/BVHPrim.h>
#include <liboscar/Maths/Vec3.h>

#include <cstdint>
#include <cstddef>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace osc { struct Ray; }

namespace osc
{
    // a bounding volume hierarchy (BVH) of numerically IDed `AABB`s
    //
    // the `AABB`s may be computed from triangles, commonly called a "triangle BVH"
    class BVH final {
    public:
        void clear();

        // triangle `BVH`es
        //
        // `prim.id()` will refer to the index of the first vertex in the triangle
        void build_from_indexed_triangles(
            std::span<const Vec3> vertices,
            std::span<const uint16_t> indices
        );
        void build_from_indexed_triangles(
            std::span<const Vec3> vertices,
            std::span<const uint32_t> indices
        );

        // returns the position of the closest ray-triangle collision along the ray, if any
        std::optional<BVHCollision> closest_ray_indexed_triangle_collision(
            std::span<const Vec3> vertices,
            std::span<const uint16_t> indices,
            const Ray&
        ) const;
        std::optional<BVHCollision> closest_ray_indexed_triangle_collision(
            std::span<const Vec3> vertices,
            std::span<const uint32_t> indices,
            const Ray&
        ) const;

        // `AABB` `BVH`es
        //
        // `prim.id()` will refer to the index of the `AABB`
        void build_from_aabbs(std::span<const AABB>);

        // calls the callback with each collision between the `Ray` and an `AABB` in
        // the `BVH`
        void for_each_ray_aabb_collision(const Ray&, const std::function<void(BVHCollision)>&) const;

        // returns `true` if the `BVH` contains no `BVHNode`s
        [[nodiscard]] bool empty() const;

        // returns the maximum depth of the `BVH` tree
        size_t max_depth() const;

        // returns the `AABB` of the root node, or `std::nullopt` if there are no nodes in
        // the tree
        std::optional<AABB> bounds() const;

        // calls the given function with each leaf node in the tree
        void for_each_leaf_node(const std::function<void(const BVHNode&)>&) const;

        // calls the given function with each leaf or inner node in the tree
        void for_each_leaf_or_inner_node(const std::function<void(const BVHNode&)>&) const;

    private:
        // nodes in the hierarchy
        std::vector<BVHNode> nodes_;

        // primitives (triangles, `AABB`s) that the nodes reference
        std::vector<BVHPrim> prims_;
    };
}
