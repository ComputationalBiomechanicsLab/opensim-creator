#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Platform/ResourcePath.h>

#include <functional>
#include <memory>
#include <string>

namespace osc { class BVH; }
namespace osc { class MeshBasicMaterial; }
namespace osc { class ResourceLoader; }
namespace osc { class Shader; }

namespace osc
{
    // a persistent cache that can be used to accelerate initializing
    // scene-related data (meshes, shaders, materials, etc.)
    //
    // this is usually used when rendering multiple images that are likely
    // to share these datastructures (e.g. you'll keep this around as state
    // across multiple frames and share it between multiple `SceneRenderer`s)
    class SceneCache final {
    public:
        // constructs the cache with a defaulted `ResourceLoader`, which will be used
        // with any method that uses a `ResourcePath`
        SceneCache();

        // constructs the cache with the provided `ResourceLoader`, which will be used
        // with any method that uses a `ResourcePath`
        explicit SceneCache(const ResourceLoader&);
        SceneCache(const SceneCache&) = delete;
        SceneCache(SceneCache&&) noexcept;
        SceneCache& operator=(const SceneCache&) = delete;
        SceneCache& operator=(SceneCache&&) noexcept;
        ~SceneCache() noexcept;

        // clear all cached meshes (can be slow: forces a full reload)
        void clear_meshes();

        // always returns (it will use a dummy cube and print a log error if something fails)
        Mesh get_mesh(const std::string& key, const std::function<Mesh()>& getter);

        Mesh sphere_mesh();
        Mesh circle_mesh();
        Mesh cylinder_mesh();
        Mesh uncapped_cylinder_mesh();
        Mesh brick_mesh();
        Mesh cone_mesh();
        Mesh floor_mesh();
        Mesh grid_mesh();
        Mesh cube_wireframe_mesh();
        Mesh yline_mesh();
        Mesh quad_mesh();
        Mesh torus_mesh(float tube_center_radius, float tube_radius);

        const BVH& get_bvh(const Mesh&);

        // returns a `Shader` loaded via the `ResourceLoader` that was provided to the constructor
        const Shader& get_shader(
            const ResourcePath& vertex_shader_path,
            const ResourcePath& fragment_shader_path
        );

        // returns a `Shader` loaded via the `ResourceLoader` that was provided to the constructor
        const Shader& get_shader(
            const ResourcePath& vertex_shader_path,
            const ResourcePath& geometry_shader_path,
            const ResourcePath& fragment_shader_path
        );

        const MeshBasicMaterial& basic_material();
        const MeshBasicMaterial& wireframe_material();

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
