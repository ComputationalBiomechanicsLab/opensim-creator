#pragma once

#include <liboscar/graphics/detail/maybe_index.h>
#include <liboscar/graphics/material.h>
#include <liboscar/graphics/material_property_block.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/maths/aabb_functions.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/matrix3x3.h>
#include <liboscar/maths/matrix4x4.h>
#include <liboscar/maths/matrix_functions.h>
#include <liboscar/maths/transform.h>
#include <liboscar/maths/transform_functions.h>
#include <liboscar/maths/vector3.h>

#include <cstddef>
#include <iterator>
#include <optional>
#include <ranges>
#include <type_traits>
#include <vector>

namespace osc::detail
{
    // Represents what's queued up whenever a caller calls `graphics::draw`
    class RenderQueue final {
    public:
        using handle_type = size_t;
        using handle_iterator = std::vector<handle_type>::iterator;
        using handle_const_iterator = std::vector<handle_type>::const_iterator;
        using handle_subrange = std::ranges::subrange<handle_iterator>;
        using handle_const_subrange = std::ranges::subrange<handle_const_iterator>;
        using size_type = size_t;

        friend bool operator==(const RenderQueue&, const RenderQueue&) = default;

        handle_type emplace(
            const Mesh& mesh,
            const Transform& transform,
            const Material& material)
        {
            return emplace(mesh, matrix4x4_cast(transform), material);
        }
        handle_type emplace(
            const Mesh& mesh,
            const Transform& transform,
            const Material& material,
            const MaterialPropertyBlock& material_prop_block)
        {
            return emplace(mesh, matrix4x4_cast(transform), material, material_prop_block);
        }
        handle_type emplace(
            const Mesh& mesh,
            const Transform& transform,
            const Material& material,
            size_t submesh_index)
        {
            return emplace(mesh, matrix4x4_cast(transform), material, submesh_index);
        }
        handle_type emplace(
            const Mesh& mesh,
            const Transform& transform,
            const Material& material,
            const MaterialPropertyBlock& material_prop_block,
            size_t submesh_index)
        {
            return emplace(mesh, matrix4x4_cast(transform), material, material_prop_block, submesh_index);
        }
        handle_type emplace(const Mesh& mesh, const Matrix4x4& transform, const Material& material)
        {
            materials_.push_back(material);
            material_property_blocks_.push_back(blank_property_block_);
            meshes_.push_back(mesh);
            maybe_submesh_indices_.emplace_back(std::nullopt);
            model_matrices_.push_back(transform);
            return handles_.emplace_back(handles_.size());
        }
        handle_type emplace(
            const Mesh& mesh,
            const Matrix4x4& transform,
            const Material& material,
            const MaterialPropertyBlock& material_prop_block)
        {
            materials_.push_back(material);
            material_property_blocks_.push_back(material_prop_block);
            meshes_.push_back(mesh);
            maybe_submesh_indices_.emplace_back(std::nullopt);
            model_matrices_.push_back(transform);
            return handles_.emplace_back(handles_.size());
        }
        handle_type emplace(
            const Mesh& mesh,
            const Matrix4x4& transform,
            const Material& material,
            size_t submesh_index)
        {
            materials_.push_back(material);
            material_property_blocks_.push_back(blank_property_block_);
            meshes_.push_back(mesh);
            maybe_submesh_indices_.emplace_back(submesh_index);
            model_matrices_.push_back(transform);
            return handles_.emplace_back(handles_.size());
        }
        handle_type emplace(
            const Mesh& mesh,
            const Matrix4x4& transform,
            const Material& material,
            const MaterialPropertyBlock& material_prop_block,
            size_t submesh_index)
        {
            materials_.push_back(material);
            material_property_blocks_.push_back(material_prop_block);
            meshes_.push_back(mesh);
            maybe_submesh_indices_.emplace_back(submesh_index);
            model_matrices_.push_back(transform);
            return handles_.emplace_back(handles_.size());
        }

        void clear()
        {
            materials_.clear();
            material_property_blocks_.clear();
            meshes_.clear();
            maybe_submesh_indices_.clear();
            model_matrices_.clear();
            handles_.clear();
        }

        [[nodiscard]] bool empty() const { return materials_.empty(); }

        const Material& material(handle_type id) const { return materials_[id]; }
        Material& material(handle_type id) { return materials_[id]; }

        const MaterialPropertyBlock& material_property_block(handle_type id) const { return material_property_blocks_[id]; }
        MaterialPropertyBlock& material_property_block(handle_type id) { return material_property_blocks_[id]; }

        const Mesh& mesh(handle_type id) const { return meshes_[id]; }
        Mesh& mesh(handle_type id) { return meshes_[id]; }

        const MaybeIndex& maybe_submesh_index(handle_type id) const { return maybe_submesh_indices_[id]; }
        MaybeIndex& maybe_submesh_index(handle_type id) { return maybe_submesh_indices_[id]; }

        const Matrix4x4& model_matrix(handle_type id) const { return model_matrices_[id]; }
        Matrix4x4& model_matrix(handle_type id) { return model_matrices_[id]; }

        Vector3 world_space_centroid(handle_type id) const
        {
            if (const auto local_centroid = mesh(id).centroid()) {
                return transform_point(model_matrix(id), *local_centroid);
            } else {
                return Vector3{0.0f};  // Just something for scene sorting
            }
        }

        bool is_opaque(handle_type id) const { return not material(id).is_transparent(); }
        bool is_depth_tested(handle_type id) const { return material(id).is_depth_tested(); }
        Matrix3x3 normal_matrix3x3(handle_type id) const { return osc::normal_matrix(model_matrix(id)); }
        Matrix4x4 normal_matrix4x4(handle_type id) const { return osc::normal_matrix4x4(model_matrix(id)); }

        const auto& handles() const { return handles_; }
        auto& handles() { return handles_; }

        size_type size() const { return handles_.size(); }

    private:
        MaterialPropertyBlock blank_property_block_;
        std::vector<Material> materials_;
        std::vector<MaterialPropertyBlock> material_property_blocks_;
        std::vector<Mesh> meshes_;
        std::vector<MaybeIndex> maybe_submesh_indices_;
        std::vector<Matrix4x4> model_matrices_;
        std::vector<handle_type> handles_;
    };
}
