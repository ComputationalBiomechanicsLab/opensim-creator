#pragma once

#include <liboscar/graphics/Mesh.h>
#include <liboscar/platform/ResourcePath.h>

#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <typeinfo>

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
        Mesh sphere_octant_mesh();

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

        // Returns an object with the given type via `typeid`, default-constructing it if it isn't
        // already in the cache
        //
        // - `T` must be exactly the required type, not something derived from it (it's typeid-based)
        // - If an instance of `T` doesn't already exist in this cache, it will be default-constructed
        //   and placed in the cache.
        // - This typeid-based caching mechanism is independent of other caching methods. E.g. if some
        //   other member method of the cache returns an instance of `T` then it operates independently
        //   of this method.
        template<typename T>
        requires std::is_default_constructible_v<T> and std::is_destructible_v<T>
        const T& get()
        {
            if (const void* ptr = try_get(typeid(T))) {
                return *static_cast<const T*>(ptr);
            }

            auto constructed = std::make_shared<T>();
            const T& rv = *constructed;
            insert(typeid(T), std::move(constructed));
            return rv;
        }

        const MeshBasicMaterial& basic_material();
        const MeshBasicMaterial& wireframe_material();

    private:
        const void* try_get(const std::type_info&) const;
        void insert(const std::type_info&, std::shared_ptr<void>);

        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
