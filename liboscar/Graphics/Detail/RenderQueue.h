#pragma once

#include <liboscar/Graphics/Detail/MaybeIndex.h>
#include <liboscar/Graphics/Material.h>
#include <liboscar/Graphics/MaterialPropertyBlock.h>
#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Maths/AABBFunctions.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/Matrix3x3.h>
#include <liboscar/Maths/Matrix4x4.h>
#include <liboscar/Maths/Transform.h>
#include <liboscar/Maths/TransformFunctions.h>
#include <liboscar/Maths/Vector3.h>

#include <cstddef>
#include <iterator>
#include <optional>
#include <type_traits>
#include <vector>

namespace osc::detail
{
    // Represents what's queued up whenever a caller calls `graphics::draw`
    class RenderQueue final {
    private:
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
            ReferenceProxy& operator=(const ReferenceProxy& rhs) noexcept { *handle_pointer_ = *rhs.handle_pointer_; }
            // `const` required to satisfy the `std::indirectly_assignable` concept.
            const ReferenceProxy& operator=(const ReferenceProxy& rhs) const noexcept { *handle_pointer_ = *rhs.handle_pointer_; }

            // A non-`const` `ReferenceProxy` can be implicitly converted into a `const` one
            constexpr operator ReferenceProxy<true> () const requires (not IsConst) { return {render_queue_pointer_, handle_pointer_}; }

            auto& material() const { return render_queue_pointer_->materials_[*handle_pointer_]; }
            auto& material_property_block() const { return render_queue_pointer_->material_property_blocks_[*handle_pointer_]; }
            auto& mesh() const { return render_queue_pointer_->meshes_[*handle_pointer_]; }
            std::optional<size_t> submesh_index() const
            {
                const auto maybe = render_queue_pointer_->maybe_submesh_indices_[*handle_pointer_];
                return maybe ? std::make_optional(*maybe) : std::nullopt;
            }
            auto& transform() const { return render_queue_pointer_->transforms_[*handle_pointer_]; }
            auto& world_space_centroid() const { return render_queue_pointer_->world_space_centroids_[*handle_pointer_]; }

            bool is_opaque() const { return not material().is_transparent(); }
            bool is_depth_tested() const { return material().is_depth_tested(); }
            const Matrix4x4& model_matrix4x4() const { return transform(); }
            Matrix3x3 normal_matrix() const { return ::normal_matrix(transform()); }
            Matrix4x4 normal_matrix4x4() const { return ::normal_matrix4x4(transform()); }
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

            Iterator() = default;

            Iterator(
                render_queue_pointer render_queue_pointer,
                handle_pointer handle_pointer) :

                render_queue_pointer_{render_queue_pointer},
                handle_pointer_{handle_pointer}
            {}

            // A non-`const` `Iterator` can be implicitly converted into a `const` one
            operator Iterator<true> () const requires (not IsConst) { return {render_queue_pointer_, handle_pointer_}; }

            friend bool operator==(const Iterator& lhs, const Iterator& rhs) { return lhs.handle_pointer_ == rhs.handle_pointer_; }
            friend bool operator!=(const Iterator& lhs, const Iterator& rhs) { return lhs.handle_pointer_ != rhs.handle_pointer_; }

            reference operator*() const { return {render_queue_pointer_, handle_pointer_}; }
            pointer operator->() const { return pointer{reference{render_queue_pointer_, handle_pointer_}}; }

            Iterator& operator++()                               { ++handle_pointer_; return *this; }
            Iterator  operator++(int)                            { Iterator copy{*this}; ++(*this); return copy; }
            Iterator& operator--()                               { --handle_pointer_; return *this; }
            Iterator  operator--(int)                            { Iterator copy{*this}; --(*this); return copy; }
            Iterator& operator+=(difference_type i)              { handle_pointer_ += i; }
            Iterator  operator+ (difference_type i) const        { Iterator copy{*this}; copy += i; return copy; }
            Iterator& operator-=(difference_type i)              { handle_pointer_ -= i; }
            Iterator  operator- (difference_type i) const        { Iterator copy{*this}; copy -= i; return copy; }
            difference_type operator-(const Iterator& rhs) const { return handle_pointer_ - rhs.handle_pointer_; }
            reference operator[](difference_type pos) const      { return {render_queue_pointer_, (handle_pointer_ + pos)}; }
        private:
            render_queue_pointer render_queue_pointer_ = nullptr;
            handle_pointer handle_pointer_ = nullptr;
        };

        using iterator = Iterator<false>;
        using const_iterator = Iterator<true>;
        using reference = ReferenceProxy<false>;
        using const_reference = ReferenceProxy<true>;

        reference emplace(
            const Mesh& mesh,
            const Transform& transform,
            const Material& material,
            const std::optional<MaterialPropertyBlock>& maybe_prop_block,
            std::optional<size_t> maybe_submesh_index)
        {
            return emplace(mesh, matrix4x4_cast(transform), material, maybe_prop_block, maybe_submesh_index);
        }

        reference emplace(
            const Mesh& mesh,
            const Matrix4x4& transform,
            const Material& material,
            const std::optional<MaterialPropertyBlock>& maybe_prop_block,
            std::optional<size_t> maybe_submesh_index)
        {
            materials_.push_back(material);
            material_property_blocks_.push_back(maybe_prop_block ? *maybe_prop_block : MaterialPropertyBlock{});
            meshes_.push_back(mesh);
            maybe_submesh_indices_.emplace_back(maybe_submesh_index);
            transforms_.push_back(transform);
            world_space_centroids_.push_back(transform_point(transform, centroid_of(mesh.bounds())));
            auto& handle = handles_.emplace_back(handles_.size());
            return ReferenceProxy<false>{this, &handle};
        }

        const_iterator begin() const { return {this, handles_.data()}; }
        iterator begin()             { return {this, handles_.data()}; }
        const_iterator end() const   { return {this, handles_.data() + handles_.size()}; }
        iterator end()               { return {this, handles_.data() + handles_.size()}; }

    private:
        std::vector<Material> materials_;
        std::vector<MaterialPropertyBlock> material_property_blocks_;
        std::vector<Mesh> meshes_;
        std::vector<MaybeIndex> maybe_submesh_indices_;
        std::vector<Matrix4x4> transforms_;
        std::vector<Vector3> world_space_centroids_;
        std::vector<handle_type> handles_;
    };
}
