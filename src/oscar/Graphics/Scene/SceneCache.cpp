#include "SceneCache.h"

#include <oscar/Graphics/Geometries.h>
#include <oscar/Graphics/Materials/MeshBasicMaterial.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/Scene/SceneHelpers.h>
#include <oscar/Graphics/Shader.h>
#include <oscar/Maths/BVH.h>
#include <oscar/Platform/FilesystemResourceLoader.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/ResourceLoader.h>
#include <oscar/Platform/ResourcePath.h>
#include <oscar/Utils/HashHelpers.h>
#include <oscar/Utils/SynchronizedValue.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace osc;

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
        rv.setTopology(MeshTopology::Lines);
        rv.setVerts({{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}});
        rv.setNormals({{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}});  // just give them *something* in-case they are rendered through a shader that requires normals
        rv.setIndices({0, 1});
        return rv;
    }
}

template<>
struct std::hash<ShaderLookupKey> final {

    size_t operator()(const ShaderLookupKey& inputs) const
    {
        return inputs.hash;
    }
};

template<>
struct std::hash<TorusParameters> final {
    size_t operator()(const TorusParameters& p) const
    {
        return hash_of(p.tube_center_radius, p.tube_radius);
    }
};

class osc::SceneCache::Impl final {
public:
    Impl() :
        Impl{make_resource_loader<FilesystemResourceLoader>(".")}
    {}

    explicit Impl(ResourceLoader resource_loader) :
        resource_loader_{std::move(resource_loader)}
    {}

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

    Mesh sphere = SphereGeometry{1.0f, 16, 16};
    Mesh circle = CircleGeometry{1.0f, 16};
    Mesh cylinder = CylinderGeometry{1.0f, 1.0f, 2.0f, 16};
    Mesh uncapped_cylinder = CylinderGeometry{1.0f, 1.0f, 2.0f, 16, 1, true};
    Mesh cube = BoxGeometry{2.0f, 2.0f, 2.0f};
    Mesh cone = ConeGeometry{1.0f, 2.0f, 16};
    Mesh floor = PlaneGeometry{2.0f, 2.0f, 1, 1};
    Mesh grid100x100 = GridGeometry{2.0f, 1000};
    Mesh cube_wireframe = AABBGeometry{};
    Mesh y_line = generate_y_to_y_line_mesh();
    Mesh textured_quad = floor;

    SynchronizedValue<std::unordered_map<TorusParameters, Mesh>> torus_cache;
    SynchronizedValue<std::unordered_map<std::string, Mesh>> mesh_cache;
    SynchronizedValue<std::unordered_map<Mesh, std::unique_ptr<BVH>>> bvh_cache;

    // shader stuff
    ResourceLoader resource_loader_;
    SynchronizedValue<std::unordered_map<ShaderLookupKey, Shader>> shader_cache_;
    std::optional<MeshBasicMaterial> basic_material_;
    std::optional<MeshBasicMaterial> wireframe_material_;
};

osc::SceneCache::SceneCache() :
    m_Impl{std::make_unique<Impl>()}
{}

osc::SceneCache::SceneCache(const ResourceLoader& resource_loader) :
    m_Impl{std::make_unique<Impl>(resource_loader)}
{}

osc::SceneCache::SceneCache(SceneCache&&) noexcept = default;
osc::SceneCache& osc::SceneCache::operator=(SceneCache&&) noexcept = default;
osc::SceneCache::~SceneCache() noexcept = default;

void osc::SceneCache::clear_meshes()
{
    m_Impl->mesh_cache.lock()->clear();
    m_Impl->bvh_cache.lock()->clear();
    m_Impl->torus_cache.lock()->clear();
}

Mesh osc::SceneCache::get_mesh(
    const std::string& key,
    const std::function<Mesh()>& getter)
{
    auto guard = m_Impl->mesh_cache.lock();

    auto [it, inserted] = guard->try_emplace(key, m_Impl->cube);
    if (inserted) {
        it->second = getter();
    }

    return it->second;
}

Mesh osc::SceneCache::sphere_mesh()
{
    return m_Impl->sphere;
}

Mesh osc::SceneCache::circle_mesh()
{
    return m_Impl->circle;
}

Mesh osc::SceneCache::cylinder_mesh()
{
    return m_Impl->cylinder;
}

Mesh osc::SceneCache::uncapped_cylinder_mesh()
{
    return m_Impl->uncapped_cylinder;
}

Mesh osc::SceneCache::brick_mesh()
{
    return m_Impl->cube;
}

Mesh osc::SceneCache::cone_mesh()
{
    return m_Impl->cone;
}

Mesh osc::SceneCache::floor_mesh()
{
    return m_Impl->floor;
}

Mesh osc::SceneCache::grid_mesh()
{
    return m_Impl->grid100x100;
}

Mesh osc::SceneCache::cube_wireframe_mesh()
{
    return m_Impl->cube_wireframe;
}

Mesh osc::SceneCache::yline_mesh()
{
    return m_Impl->y_line;
}

Mesh osc::SceneCache::quad_mesh()
{
    return m_Impl->textured_quad;
}

Mesh osc::SceneCache::torus_mesh(float tube_center_radius, float tube_radius)
{
    const TorusParameters key{tube_center_radius, tube_radius};

    auto guard = m_Impl->torus_cache.lock();
    auto [it, inserted] = guard->try_emplace(key, m_Impl->cube);
    if (inserted) {
        it->second = TorusGeometry{key.tube_center_radius, key.tube_radius, 12, 12, Degrees{360}};
    }

    return it->second;
}

const BVH& osc::SceneCache::get_bvh(const Mesh& mesh)
{
    auto guard = m_Impl->bvh_cache.lock();
    auto [it, inserted] = guard->try_emplace(mesh, nullptr);
    if (inserted) {
        it->second = std::make_unique<BVH>(create_triangle_bvh(mesh));
    }
    return *it->second;
}

const Shader& osc::SceneCache::get_shader(
    const ResourcePath& vertex_shader_path,
    const ResourcePath& fragment_shader_path)
{
    return m_Impl->load(vertex_shader_path, fragment_shader_path);
}

const Shader& osc::SceneCache::get_shader(
    const ResourcePath& vertex_shader_path,
    const ResourcePath& geometry_shader_path,
    const ResourcePath& fragment_shader_path)
{
    return m_Impl->load(vertex_shader_path, geometry_shader_path, fragment_shader_path);
}

const MeshBasicMaterial& osc::SceneCache::basic_material()
{
    return m_Impl->basic_material();
}

const MeshBasicMaterial& osc::SceneCache::wireframe_material()
{
    return m_Impl->wireframe_material();
}
