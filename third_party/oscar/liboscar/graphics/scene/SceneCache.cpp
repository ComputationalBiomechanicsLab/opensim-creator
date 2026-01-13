#include "SceneCache.h"

#include <liboscar/graphics/geometries/AABBGeometry.h>
#include <liboscar/graphics/geometries/BoxGeometry.h>
#include <liboscar/graphics/geometries/CircleGeometry.h>
#include <liboscar/graphics/geometries/ConeGeometry.h>
#include <liboscar/graphics/geometries/CylinderGeometry.h>
#include <liboscar/graphics/geometries/GridGeometry.h>
#include <liboscar/graphics/geometries/PlaneGeometry.h>
#include <liboscar/graphics/geometries/SphereGeometry.h>
#include <liboscar/graphics/geometries/TorusGeometry.h>
#include <liboscar/graphics/materials/MeshBasicMaterial.h>
#include <liboscar/graphics/Mesh.h>
#include <liboscar/graphics/scene/SceneHelpers.h>
#include <liboscar/graphics/Shader.h>
#include <liboscar/maths/BVH.h>
#include <liboscar/platform/Log.h>
#include <liboscar/platform/NativeFilesystem.h>
#include <liboscar/platform/ResourceLoader.h>
#include <liboscar/platform/ResourcePath.h>
#include <liboscar/utils/HashHelpers.h>
#include <liboscar/utils/SynchronizedValue.h>
#include <liboscar/utils/TypeInfoReference.h>

#include <ankerl/unordered_dense.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace osc;
using namespace osc::literals;

namespace
{
    struct TorusParameters final {

        TorusParameters(
            float tube_center_radius_,
            float tube_radius_) :

            tube_center_radius{tube_center_radius_},
            tube_radius{tube_radius_}
        {}

        friend bool operator==(const TorusParameters&, const TorusParameters&) = default;

        float tube_center_radius;
        float tube_radius;
    };

    // parameters for a shader, to be used as a key into the shader cache
    struct ShaderLookupKey final {

        ShaderLookupKey(
            ResourcePath vertex_shader_path_,
            ResourcePath fragment_shader_path_) :

            vertex_shader_path{std::move(vertex_shader_path_)},
            fragment_shader_path{std::move(fragment_shader_path_)}
        {}

        ShaderLookupKey(
            ResourcePath vertex_shader_path_,
            ResourcePath geometry_shader_path_,
            ResourcePath fragment_shader_path_) :

            vertex_shader_path{std::move(vertex_shader_path_)},
            geometry_shader_path{std::move(geometry_shader_path_)},
            fragment_shader_path{std::move(fragment_shader_path_)}
        {}

        friend bool operator==(const ShaderLookupKey&, const ShaderLookupKey&) = default;

        ResourcePath vertex_shader_path;
        ResourcePath geometry_shader_path;
        ResourcePath fragment_shader_path;

        size_t hash = hash_of(vertex_shader_path, geometry_shader_path, fragment_shader_path);
    };

    Mesh generate_y_to_y_line_mesh()
    {
        Mesh rv;
        rv.set_topology(MeshTopology::Lines);
        rv.set_vertices({{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}});
        rv.set_normals({{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}});  // just give them *something* in-case they are rendered through a shader that requires normals
        rv.set_indices({0, 1});
        return rv;
    }
}

template<>
struct std::hash<ShaderLookupKey> final {

    size_t operator()(const ShaderLookupKey& inputs) const noexcept
    {
        return inputs.hash;
    }
};

template<>
struct std::hash<TorusParameters> final {
    size_t operator()(const TorusParameters& params) const noexcept
    {
        return hash_of(params.tube_center_radius, params.tube_radius);
    }
};

class osc::SceneCache::Impl final {
public:
    Impl() :
        Impl{make_resource_loader<NativeFilesystem>(".")}
    {}

    explicit Impl(ResourceLoader resource_loader) :
        resource_loader_{std::move(resource_loader)}
    {}

    void clear_meshes()
    {
        mesh_cache.lock()->clear();
        bvh_cache.lock()->clear();
        torus_cache.lock()->clear();
    }

    Mesh get_mesh(
        const std::string& key,
        const std::function<Mesh()>& getter)
    {
        auto guard = mesh_cache.lock();

        auto [it, inserted] = guard->try_emplace(key, cube);
        if (inserted) {
            it->second = getter();
        }

        return it->second;
    }

    Mesh sphere_mesh() { return sphere; }
    Mesh circle_mesh() { return circle; }
    Mesh cylinder_mesh() { return cylinder; }
    Mesh uncapped_cylinder_mesh() { return uncapped_cylinder; }
    Mesh brick_mesh() { return cube; }
    Mesh cone_mesh() { return cone; }
    Mesh floor_mesh() { return floor; }
    Mesh grid_mesh() { return grid100x100; }
    Mesh cube_wireframe_mesh() { return cube_wireframe; }
    Mesh yline_mesh() { return y_line; }
    Mesh quad_mesh() { return textured_quad; }
    Mesh torus_mesh(float tube_center_radius, float tube_radius)
    {
        const TorusParameters key{tube_center_radius, tube_radius};

        auto guard = torus_cache.lock();
        auto [it, inserted] = guard->try_emplace(key, cube);
        if (inserted) {
            it->second = TorusGeometry{{
                .tube_center_radius = key.tube_center_radius,
                .tube_radius = key.tube_radius,
                .num_radial_segments = 12,
                .num_tubular_segments = 12,
                .arc = Degrees{360},
            }}.mesh();
        }

        return it->second;
    }
    Mesh sphere_octant_mesh() { return sphere_octant_; }

    const BVH& get_bvh(const Mesh& mesh)
    {
        auto guard = bvh_cache.lock();
        auto [it, inserted] = guard->try_emplace(mesh, nullptr);
        if (inserted) {
            it->second = std::make_unique<BVH>(create_triangle_bvh(mesh));
        }
        return *it->second;
    }

    const Shader& load(
        const ResourcePath& vertex_shader_path,
        const ResourcePath& fragment_shader_path)
    {
        const ShaderLookupKey key{vertex_shader_path, fragment_shader_path};

        auto guard = shader_cache_.lock();
        const auto it = guard->find(key);
        if (it != guard->end()) {
            return it->second;
        }
        else {
            const std::string vertex_shader_src = resource_loader_.slurp(key.vertex_shader_path);
            const std::string fragment_shader_src = resource_loader_.slurp(key.fragment_shader_path);
            return guard->emplace_hint(it, key, Shader{vertex_shader_src, fragment_shader_src})->second;
        }
    }
    const Shader& load(
        const ResourcePath& vertex_shader_path,
        const ResourcePath& geometry_shader_path,
        const ResourcePath& fragment_shader_path)
    {
        const ShaderLookupKey key{vertex_shader_path, geometry_shader_path, fragment_shader_path};

        auto guard = shader_cache_.lock();
        const auto it = guard->find(key);
        if (it != guard->end()) {
            return it->second;
        }
        else {
            const std::string vertex_shader_src = resource_loader_.slurp(key.vertex_shader_path);
            const std::string geometry_shader_src = resource_loader_.slurp(key.geometry_shader_path);
            const std::string fragment_shader_src = resource_loader_.slurp(key.fragment_shader_path);
            return guard->emplace_hint(it, key, Shader{vertex_shader_src, geometry_shader_src, fragment_shader_src})->second;
        }
    }

    const void* try_get(const std::type_info& type_info) const
    {
        const auto guard = generic_objects_.lock();
        if (const auto it = guard->find(type_info); it != guard->end()) {
            return it->second.get();
        }
        else {
            return nullptr;
        }
    }

    void insert(const std::type_info& type_info, std::shared_ptr<void> ptr)
    {
        generic_objects_.lock()->insert_or_assign(type_info, ptr);
    }

    const MeshBasicMaterial& basic_material()
    {
        return basic_material_.emplace();
    }

    const MeshBasicMaterial& wireframe_material()
    {
        if (not wireframe_material_) {
            wireframe_material_.emplace();
            wireframe_material_->set_color({0.0f, 0.6f});
            wireframe_material_->set_wireframe(true);
            wireframe_material_->set_transparent(true);
        }
        return *wireframe_material_;
    }

private:
    Mesh sphere = SphereGeometry{{.num_width_segments = 16, .num_height_segments = 16}};
    Mesh sphere_octant_ = SphereGeometry{{
        .num_width_segments = 16,
        .num_height_segments = 16,
        .phi_start = 0_deg,
        .phi_length = 90_deg,
        .theta_start = 0_deg,
        .theta_length = 90_deg,
    }};
    Mesh circle = CircleGeometry{{.radius = 1.0f, .num_segments = 16}};
    Mesh cylinder = CylinderGeometry{{.height = 2.0f, .num_radial_segments = 16}};
    Mesh uncapped_cylinder = CylinderGeometry{{.height = 2.0f, .num_radial_segments = 16, .open_ended = true}};
    Mesh cube = BoxGeometry{{.dimensions = Vector3{2.0f}}};
    Mesh cone = ConeGeometry{{.radius = 1.0f, .height = 2.0f, .num_radial_segments = 16}};
    Mesh floor = PlaneGeometry{{.dimensions = Vector2{2.0f}}};
    Mesh grid100x100 = GridGeometry{{.num_divisions = 1000}};
    Mesh cube_wireframe = AABBGeometry{}.mesh();
    Mesh y_line = generate_y_to_y_line_mesh();
    Mesh textured_quad = floor;

    SynchronizedValue<ankerl::unordered_dense::map<TorusParameters, Mesh>> torus_cache;
    SynchronizedValue<ankerl::unordered_dense::map<std::string, Mesh>> mesh_cache;
    SynchronizedValue<ankerl::unordered_dense::map<Mesh, std::unique_ptr<BVH>>> bvh_cache;

    // shader stuff
    ResourceLoader resource_loader_;
    SynchronizedValue<ankerl::unordered_dense::map<ShaderLookupKey, Shader>> shader_cache_;
    std::optional<MeshBasicMaterial> basic_material_;
    std::optional<MeshBasicMaterial> wireframe_material_;

    // typeid (generic) cache
    SynchronizedValue<ankerl::unordered_dense::map<TypeInfoReference, std::shared_ptr<void>>> generic_objects_;
};

osc::SceneCache::SceneCache() :
    impl_{std::make_unique<Impl>()}
{}
osc::SceneCache::SceneCache(const ResourceLoader& resource_loader) :
    impl_{std::make_unique<Impl>(resource_loader)}
{}
osc::SceneCache::SceneCache(SceneCache&&) noexcept = default;
osc::SceneCache& osc::SceneCache::operator=(SceneCache&&) noexcept = default;
osc::SceneCache::~SceneCache() noexcept = default;

void osc::SceneCache::clear_meshes()
{
    impl_->clear_meshes();
}

Mesh osc::SceneCache::get_mesh(
    const std::string& key,
    const std::function<Mesh()>& getter)
{
    return impl_->get_mesh(key, getter);
}

Mesh osc::SceneCache::sphere_mesh() { return impl_->sphere_mesh(); }
Mesh osc::SceneCache::circle_mesh() { return impl_->circle_mesh(); }
Mesh osc::SceneCache::cylinder_mesh() { return impl_->cylinder_mesh(); }
Mesh osc::SceneCache::uncapped_cylinder_mesh() { return impl_->uncapped_cylinder_mesh(); }
Mesh osc::SceneCache::brick_mesh() { return impl_->brick_mesh(); }
Mesh osc::SceneCache::cone_mesh() { return impl_->cone_mesh(); }
Mesh osc::SceneCache::floor_mesh() { return impl_->floor_mesh(); }
Mesh osc::SceneCache::grid_mesh() { return impl_->grid_mesh(); }
Mesh osc::SceneCache::cube_wireframe_mesh() { return impl_->cube_wireframe_mesh(); }
Mesh osc::SceneCache::yline_mesh() { return impl_->yline_mesh(); }
Mesh osc::SceneCache::quad_mesh() { return impl_->quad_mesh(); }
Mesh osc::SceneCache::torus_mesh(float tube_center_radius, float tube_radius)
{
    return impl_->torus_mesh(tube_center_radius, tube_radius);
}
Mesh osc::SceneCache::sphere_octant_mesh()
{
    return impl_->sphere_octant_mesh();
}

const BVH& osc::SceneCache::get_bvh(const Mesh& mesh)
{
    return impl_->get_bvh(mesh);
}

const Shader& osc::SceneCache::get_shader(
    const ResourcePath& vertex_shader_path,
    const ResourcePath& fragment_shader_path)
{
    return impl_->load(vertex_shader_path, fragment_shader_path);
}

const Shader& osc::SceneCache::get_shader(
    const ResourcePath& vertex_shader_path,
    const ResourcePath& geometry_shader_path,
    const ResourcePath& fragment_shader_path)
{
    return impl_->load(vertex_shader_path, geometry_shader_path, fragment_shader_path);
}

const void* osc::SceneCache::try_get(const std::type_info& type_info) const
{
    return impl_->try_get(type_info);
}

void osc::SceneCache::insert(const std::type_info& type_info, std::shared_ptr<void> ptr)
{
    impl_->insert(type_info, std::move(ptr));
}

const MeshBasicMaterial& osc::SceneCache::basic_material()
{
    return impl_->basic_material();
}

const MeshBasicMaterial& osc::SceneCache::wireframe_material()
{
    return impl_->wireframe_material();
}
