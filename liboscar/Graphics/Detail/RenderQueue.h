#pragma once

#include <liboscar/Graphics/Detail/MaybeIndex.h>
#include <liboscar/Graphics/Material.h>
#include <liboscar/Graphics/MaterialPropertyBlock.h>
#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Maths/AABBFunctions.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/Matrix3x3.h>
#include <liboscar/Maths/Matrix4x4.h>
#include <liboscar/Maths/MatrixFunctions.h>
#include <liboscar/Maths/Transform.h>
#include <liboscar/Maths/TransformFunctions.h>
#include <liboscar/Maths/Vector3.h>

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
        // Handle that's used to indirectly access an element in the queue.
        using handle_type = size_t;

    public:
        // Proxies the behavior of references to a single render object in the
        // render queue. The primary use of `ReferenceProxy` is to provide an
        // an indirectly assignable value in the `RenderQueue` that algorithms
        // can operate on (e.g. queue sorting).
        template<bool IsConst = true>
        class ReferenceProxy final {
        private:
            using render_queue_pointer = typename std::conditional_t<IsConst, const RenderQueue*, RenderQueue*>;
            using handle_pointer = typename std::conditional_t<IsConst, const handle_type*, handle_type*>;
        public:
            explicit ReferenceProxy(
                render_queue_pointer render_queue_pointer,
                handle_pointer handle_pointer) :

                render_queue_pointer_{render_queue_pointer},
                handle_pointer_{handle_pointer}
            {}
            ReferenceProxy(const ReferenceProxy&) = default;
            ~ReferenceProxy() noexcept = default;

            // Assignment has reference semantics (it reassigns the handle, not the data).
            ReferenceProxy& operator=(const ReferenceProxy& rhs) noexcept { *handle_pointer_ = *rhs.handle_pointer_; return *this; }
            // `const` required to satisfy the `std::indirectly_assignable` concept.
            const ReferenceProxy& operator=(const ReferenceProxy& rhs) const noexcept { *handle_pointer_ = *rhs.handle_pointer_; return *this; }

            // A non-`const` `ReferenceProxy` can be implicitly converted into a `const` one
            operator ReferenceProxy<true> () const requires (not IsConst) { return ReferenceProxy<true>{render_queue_pointer_, handle_pointer_}; }

            // `std::swap` only works on lvalues, which this proxy emulates, so we need a custom `std::swap`
            friend void swap(ReferenceProxy a, ReferenceProxy b) { std::swap(*a.handle_pointer_, *b.handle_pointer_); }

            auto& material() const { return render_queue_pointer_->materials_[*handle_pointer_]; }
            auto& material_property_block() const { return render_queue_pointer_->material_property_blocks_[*handle_pointer_]; }
            auto& mesh() const { return render_queue_pointer_->meshes_[*handle_pointer_]; }
            auto& maybe_submesh_index() const { return render_queue_pointer_->maybe_submesh_indices_[*handle_pointer_]; }
            auto& model_matrix() const { return render_queue_pointer_->model_matrices_[*handle_pointer_]; }
            Vector3 world_space_centroid() const { return transform_point(model_matrix(), centroid_of(mesh().bounds())); }

            bool is_opaque() const { return not material().is_transparent(); }
            bool is_depth_tested() const { return material().is_depth_tested(); }
            Matrix3x3 normal_matrix3x3() const { return osc::normal_matrix(model_matrix()); }
            Matrix4x4 normal_matrix4x4() const { return osc::normal_matrix4x4(model_matrix()); }
        private:
            render_queue_pointer render_queue_pointer_;
            handle_pointer handle_pointer_;
        };

        template<bool IsConst>
        class ArrowProxy final {
        public:
            explicit ArrowProxy(ReferenceProxy<IsConst> reference_proxy) :
                reference_proxy_{reference_proxy}
            {}

            ReferenceProxy<IsConst>* operator->() { return &reference_proxy_; }
        private:
            ReferenceProxy<IsConst> reference_proxy_;
        };

        // A random-access iterator over the (proxied-to) render objects in the `RenderQueue`
        template<bool IsConst>
        class Iterator final {
        public:
            using difference_type = ptrdiff_t;
            using value_type = ReferenceProxy<IsConst>;
            using reference = ReferenceProxy<IsConst>;
            using pointer = ArrowProxy<IsConst>;
            using iterator_category = std::random_access_iterator_tag;
            using render_queue_pointer = typename std::conditional_t<IsConst, const RenderQueue*, RenderQueue*>;
            using handle_pointer = typename std::conditional_t<IsConst, const handle_type*, handle_type*>;

            Iterator() noexcept = default;

            Iterator(
                render_queue_pointer render_queue_pointer,
                handle_pointer handle_pointer) noexcept :

                render_queue_pointer_{render_queue_pointer},
                handle_pointer_{handle_pointer}
            {}

            // A non-`const` `Iterator` can be implicitly converted into a `const` one
            operator Iterator<true> () const requires (not IsConst) { return {render_queue_pointer_, handle_pointer_}; }

            friend bool operator==(const Iterator& lhs, const Iterator& rhs) { return lhs.handle_pointer_ == rhs.handle_pointer_; }
            friend auto operator<=>(const Iterator& lhs, const Iterator& rhs) { return lhs.handle_pointer_ <=> rhs.handle_pointer_; }

            reference operator*() const { return reference{render_queue_pointer_, handle_pointer_}; }
            pointer operator->() const { return pointer{reference{render_queue_pointer_, handle_pointer_}}; }

            Iterator& operator++()                               { ++handle_pointer_; return *this; }
            Iterator  operator++(int)                            { Iterator copy{*this}; ++(*this); return copy; }
            Iterator& operator--()                               { --handle_pointer_; return *this; }
            Iterator  operator--(int)                            { Iterator copy{*this}; --(*this); return copy; }
            Iterator& operator+=(difference_type i)              { handle_pointer_ += i; return *this; }
            Iterator  operator+ (difference_type i) const        { Iterator copy{*this}; copy += i; return copy; }
            Iterator& operator-=(difference_type i)              { handle_pointer_ -= i; return *this; }
            Iterator  operator- (difference_type i) const        { Iterator copy{*this}; copy -= i; return copy; }
            difference_type operator-(const Iterator& rhs) const { return handle_pointer_ - rhs.handle_pointer_; }
            reference operator[](difference_type pos) const      { return reference{render_queue_pointer_, (handle_pointer_ + pos)}; }
        private:
            render_queue_pointer render_queue_pointer_ = nullptr;
            handle_pointer handle_pointer_ = nullptr;
        };

        using iterator = Iterator<false>;
        using const_iterator = Iterator<true>;
        using reference = ReferenceProxy<false>;
        using const_reference = ReferenceProxy<true>;
        using subrange = std::ranges::subrange<iterator>;
        using const_subrange = std::ranges::subrange<const_iterator>;

        friend bool operator==(const RenderQueue&, const RenderQueue&) = default;

        reference emplace(const Mesh& mesh, const Transform& transform, const Material& material)
        {
            return emplace(mesh, matrix4x4_cast(transform), material);
        }
        reference emplace(const Mesh& mesh, const Transform& transform, const Material& material, const MaterialPropertyBlock& material_prop_block)
        {
            return emplace(mesh, matrix4x4_cast(transform), material, material_prop_block);
        }
        reference emplace(const Mesh& mesh, const Transform& transform, const Material& material, size_t submesh_index)
        {
            return emplace(mesh, matrix4x4_cast(transform), material, submesh_index);
        }
        reference emplace(const Mesh& mesh, const Transform& transform, const Material& material, const MaterialPropertyBlock& material_prop_block, size_t submesh_index)
        {
            return emplace(mesh, matrix4x4_cast(transform), material, material_prop_block, submesh_index);
        }
        reference emplace(const Mesh& mesh, const Matrix4x4& transform, const Material& material)
        {
            materials_.push_back(material);
            material_property_blocks_.push_back(blank_property_block_);
            meshes_.push_back(mesh);
            maybe_submesh_indices_.emplace_back(std::nullopt);
            model_matrices_.push_back(transform);
            auto& handle = handles_.emplace_back(handles_.size());
            return ReferenceProxy<false>{this, &handle};
        }
        reference emplace(const Mesh& mesh, const Matrix4x4& transform, const Material& material, const MaterialPropertyBlock& material_prop_block)
        {
            materials_.push_back(material);
            material_property_blocks_.push_back(material_prop_block);
            meshes_.push_back(mesh);
            maybe_submesh_indices_.emplace_back(std::nullopt);
            model_matrices_.push_back(transform);
            auto& handle = handles_.emplace_back(handles_.size());
            return ReferenceProxy<false>{this, &handle};
        }
        reference emplace(const Mesh& mesh, const Matrix4x4& transform, const Material& material, size_t submesh_index)
        {
            materials_.push_back(material);
            material_property_blocks_.push_back(blank_property_block_);
            meshes_.push_back(mesh);
            maybe_submesh_indices_.emplace_back(submesh_index);
            model_matrices_.push_back(transform);
            auto& handle = handles_.emplace_back(handles_.size());
            return ReferenceProxy<false>{this, &handle};
        }
        reference emplace(const Mesh& mesh, const Matrix4x4& transform, const Material& material, const MaterialPropertyBlock& material_prop_block, size_t submesh_index)
        {
            materials_.push_back(material);
            material_property_blocks_.push_back(material_prop_block);
            meshes_.push_back(mesh);
            maybe_submesh_indices_.emplace_back(submesh_index);
            model_matrices_.push_back(transform);
            auto& handle = handles_.emplace_back(handles_.size());
            return ReferenceProxy<false>{this, &handle};
        }

        const_iterator begin() const { return {this, handles_.data()}; }
        iterator begin()             { return {this, handles_.data()}; }
        const_iterator end() const   { return {this, handles_.data() + handles_.size()}; }
        iterator end()               { return {this, handles_.data() + handles_.size()}; }

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
