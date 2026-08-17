#include "camera.h"

#include <liboscar/graphics/camera_clipping_planes.h>
#include <liboscar/graphics/camera_projection.h>
#include <liboscar/maths/angle.h>
#include <liboscar/maths/constants.h>
#include <liboscar/maths/geometric_functions.h>
#include <liboscar/maths/matrix4x4.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/quaternion.h>
#include <liboscar/maths/ray.h>
#include <liboscar/maths/rect.h>
#include <liboscar/maths/rect_functions.h>
#include <liboscar/maths/vector.h>
#include <liboscar/utilities/enum_helpers.h>

#include <cmath>
#include <optional>

using namespace osc;
using namespace osc::literals;

class osc::Camera::Impl final {
public:
    Impl() = default;

    friend bool operator==(const Impl&, const Impl&) = default;

    void reset()
    {
        *this = Impl{};
    }

    CameraProjection projection() const                     { return projection_; }
    void set_projection(CameraProjection camera_projection) { projection_ = camera_projection; }

    float orthographic_size() const                     { return orthographic_size_; }
    void set_orthographic_size(float orthographic_size) { orthographic_size_ = orthographic_size; }

    Radians vertical_field_of_view() const                          { return vertical_field_of_view_; }
    void set_vertical_field_of_view(Radians vertical_field_of_view) { vertical_field_of_view_ = vertical_field_of_view; }

    Radians horizontal_field_of_view(float aspect_ratio) const
    {
        return vertical_to_horizontal_field_of_view(vertical_field_of_view(), aspect_ratio);
    }

    CameraClippingPlanes clipping_planes() const                          { return clipping_planes_; }
    void set_clipping_planes(CameraClippingPlanes camera_clipping_planes) { clipping_planes_ = camera_clipping_planes; }

    float near_clipping_plane() const                       { return clipping_planes_.znear; }
    void set_near_clipping_plane(float near_clipping_plane) { clipping_planes_.znear = near_clipping_plane; }

    float far_clipping_plane() const                      { return clipping_planes_.zfar; }
    void set_far_clipping_plane(float far_clipping_plane) { clipping_planes_.zfar = far_clipping_plane; }

    Vector3 position() const                   { return position_; }
    void set_position(const Vector3& position) { position_ = position; }

    Quaternion rotation() const { return rotation_; }
    void set_rotation(const Quaternion& rotation) { rotation_ = normalize(rotation); }

    Vector3 direction() const                    { return rotation_ * Vector3{0.0f, 0.0f, -1.0f}; }
    void set_direction(const Vector3& direction)
    {
        const auto length_squared = length2(direction);
        if (length_squared <= epsilon_v<float>) {
            return;  // Ignore degenerate vector.
        }

        const auto backward = -(direction/sqrt(length_squared));  // +Z (-Z is forward)
        auto reference_up = up();

        // Ensure `backward` will not be parallel with `reference_up`.
        if (std::abs(dot(backward, reference_up)) > max_abs_dot_for_cross_v<float>) {
            reference_up = rotation_ * Vector3{1.0f, 0.0f, 0.0f};
        }

        const auto right   = normalize(cross(reference_up, backward));
        const auto up      = cross(backward, right);
        rotation_ = quaternion_from_xyz(right, up, backward);
    }

    Vector3 forward() const { return direction(); }
    void set_forward(const Vector3f& direction) { set_direction(direction); }

    Vector3 up() const             { return rotation_ * Vector3{0.0f, 1.0f, 0.0f}; }
    void set_up(const Vector3& up)
    {
        const auto length_squared = length2(up);
        if (length_squared <= epsilon_v<float>) {
            return;  // Ignore degenerate vector.
        }

        const auto nup = up/sqrt(length_squared);
        const auto current_direction = direction();

        // Ignore the new `up` if it's parallel with the current `direction`.
        if (std::abs(dot(current_direction, nup)) > max_abs_dot_for_cross_v<float>) {
            return;
        }

        const auto right = normalize(cross(current_direction, nup));
        const auto backward = cross(right, nup);  // +Z (-Z is forward)

        rotation_ = quaternion_from_xyz(right, nup, backward);
    }

    Ray principal_ray() const { return {position(), direction()}; }

    Matrix4x4 view_matrix() const
    {
        if (maybe_view_matrix_override_) {
            return *maybe_view_matrix_override_;
        }

        const Quaternion inv_rotation = inverse(rotation_);
        auto rv = matrix4x4_cast(inv_rotation);
        rv[3] = Vector4{inv_rotation * -position_, 1.0f};
        return rv;
    }

    Matrix4x4 inverse_view_matrix() const
    {
        if (maybe_view_matrix_override_) {
            return inverse(*maybe_view_matrix_override_);
        }
        auto rv = matrix4x4_cast(rotation_);
        rv[3] = Vector4{position_, 1.0f};
        return rv;
    }

    std::optional<Matrix4x4> view_matrix_override() const                        { return maybe_view_matrix_override_; }
    void set_view_matrix_override(std::optional<Matrix4x4> view_matrix_override) { maybe_view_matrix_override_ = view_matrix_override; }

    Matrix4x4 projection_matrix(float aspect_ratio) const
    {
        if (maybe_projection_matrix_override_) {
            return *maybe_projection_matrix_override_;
        }

        // Guard against zero/negative/NaN aspect ratios.
        const float safe_aspect_ratio = aspect_ratio > 0.0f ? aspect_ratio : 1.0f;

        if (projection() == CameraProjection::Perspective) {
            return perspective(
                vertical_field_of_view_,
                safe_aspect_ratio,
                clipping_planes_.znear,
                clipping_planes_.zfar
            );
        }
        // else: orthographic
        static_assert(osc::num_options<CameraProjection>() == 2);
        const float height = orthographic_size_;
        const float width = height * safe_aspect_ratio;

        const float right = 0.5f * width;
        const float left = -right;
        const float top = 0.5f * height;
        const float bottom = -top;

        return ortho(left, right, bottom, top, clipping_planes_.znear, clipping_planes_.zfar);
    }
    std::optional<Matrix4x4> projection_matrix_override() const                              { return maybe_projection_matrix_override_; }
    void set_projection_matrix_override(std::optional<Matrix4x4> projection_matrix_override) { maybe_projection_matrix_override_ = projection_matrix_override; }

    Matrix4x4 view_projection_matrix(float aspect_ratio) const
    {
        return projection_matrix(aspect_ratio) * view_matrix();
    }

    Matrix4x4 inverse_view_projection_matrix(float aspect_ratio) const
    {
        return inverse(view_projection_matrix(aspect_ratio));
    }

    Vector2 world_to_ui(const Vector3& world_position, const Rect& ui_rect) const
    {
        return project_onto_viewport_rect(
            world_position,
            view_matrix(),
            projection_matrix(aspect_ratio_of(ui_rect)),
            ui_rect
        );
    }

    Ray ui_to_world(const Vector2& ui_position, const Rect& ui_rect) const
    {
        const Vector2 normalized_tl_pos = (ui_position - ui_rect.ypd_top_left()) / ui_rect.dimensions();
        const Vector3 origin = topleft_normalized_point_to_world_znear(
            normalized_tl_pos,
            view_matrix(),
            projection_matrix(aspect_ratio_of(ui_rect))
        );

        static_assert(num_options<CameraProjection>() == 2);
        if (projection() == CameraProjection::Orthographic) {
            return Ray{origin, direction()};
        } else {
            return Ray{origin, normalize(origin - position())};
        }
    }

    float view_volume_height_at_depth(float depth) const
    {
        static_assert(num_options<CameraProjection>() == 2);
        if (projection_ == CameraProjection::Orthographic) {
            return orthographic_size_;
        }
        // Else: perspective projection
        return 2.0f * depth * tan(0.5f * vertical_field_of_view_);
    }

private:
    // Transform
    Vector3 position_;
    Quaternion rotation_;
    std::optional<Matrix4x4> maybe_view_matrix_override_;

    // Projection
    CameraProjection projection_ = CameraProjection::Default;
    float orthographic_size_ = 2.0f;
    Radians vertical_field_of_view_ = 90_deg;
    CameraClippingPlanes clipping_planes_{0.1f, 100.0f};
    std::optional<Matrix4x4> maybe_projection_matrix_override_;
};

osc::Camera::Camera() : impl_{make_cowv<Impl>()} {}
bool osc::operator==(const Camera& lhs, const Camera& rhs) { return lhs.impl_ == rhs.impl_ or *lhs.impl_ == *rhs.impl_; }

void osc::Camera::reset() { impl_.upd()->reset(); }

CameraProjection osc::Camera::projection() const              { return impl_->projection(); }
void osc::Camera::set_projection(CameraProjection projection) { impl_.upd()->set_projection(projection); }

float osc::Camera::orthographic_size() const                     { return impl_->orthographic_size(); }
void osc::Camera::set_orthographic_size(float orthographic_size) { impl_.upd()->set_orthographic_size(orthographic_size); }

Radians osc::Camera::vertical_field_of_view() const                          { return impl_->vertical_field_of_view(); }
void osc::Camera::set_vertical_field_of_view(Radians vertical_field_of_view) { impl_.upd()->set_vertical_field_of_view(vertical_field_of_view); }

Radians osc::Camera::horizontal_field_of_view(float aspect_ratio) const { return impl_->horizontal_field_of_view(aspect_ratio); }

CameraClippingPlanes osc::Camera::clipping_planes() const                   { return impl_->clipping_planes(); }
void osc::Camera::set_clipping_planes(CameraClippingPlanes clipping_planes) { impl_.upd()->set_clipping_planes(clipping_planes); }

float osc::Camera::near_clipping_plane() const                       { return impl_->near_clipping_plane(); }
void osc::Camera::set_near_clipping_plane(float near_clipping_plane) { impl_.upd()->set_near_clipping_plane(near_clipping_plane); }

float osc::Camera::far_clipping_plane() const                      { return impl_->far_clipping_plane(); }
void osc::Camera::set_far_clipping_plane(float far_clipping_plane) { impl_.upd()->set_far_clipping_plane(far_clipping_plane); }

Vector3 osc::Camera::position() const                   { return impl_->position(); }
void osc::Camera::set_position(const Vector3& position) { impl_.upd()->set_position(position); }

Quaternion osc::Camera::rotation() const { return impl_->rotation(); }
void osc::Camera::set_rotation(const Quaternion& rotation) { impl_.upd()->set_rotation(rotation); }

Vector3 osc::Camera::direction() const                    { return impl_->direction(); }
void osc::Camera::set_direction(const Vector3& direction) { impl_.upd()->set_direction(direction); }

Vector3 osc::Camera::forward() const                    { return impl_->forward(); }
void osc::Camera::set_forward(const Vector3& direction) { impl_.upd()->set_forward(direction); }

Vector3 osc::Camera::up() const             { return impl_->up(); }
void osc::Camera::set_up(const Vector3& up) { impl_.upd()->set_up(up); }

Ray osc::Camera::principal_ray() const { return impl_->principal_ray(); }

Matrix4x4 osc::Camera::view_matrix() const                                    { return impl_->view_matrix(); }
Matrix4x4 osc::Camera::inverse_view_matrix() const                            { return impl_->inverse_view_matrix(); }
std::optional<Matrix4x4> osc::Camera::view_matrix_override() const            { return impl_->view_matrix_override(); }
void osc::Camera::set_view_matrix_override(std::optional<Matrix4x4> override) { impl_.upd()->set_view_matrix_override(override); }

Matrix4x4 osc::Camera::projection_matrix(float aspect_ratio) const                  { return impl_->projection_matrix(aspect_ratio); }
std::optional<Matrix4x4> osc::Camera::projection_matrix_override() const            { return impl_->projection_matrix_override(); }
void osc::Camera::set_projection_matrix_override(std::optional<Matrix4x4> override) { impl_.upd()->set_projection_matrix_override(override); }

Matrix4x4 osc::Camera::view_projection_matrix(float aspect_ratio) const         { return impl_->view_projection_matrix(aspect_ratio); }
Matrix4x4 osc::Camera::inverse_view_projection_matrix(float aspect_ratio) const { return impl_->inverse_view_projection_matrix(aspect_ratio); }

Vector2 osc::Camera::world_to_ui(const Vector3& world_position, const Rect& ui_rect) const { return impl_->world_to_ui(world_position, ui_rect); }
Ray osc::Camera::ui_to_world(const Vector2& ui_position, const Rect& ui_rect) const { return impl_->ui_to_world(ui_position, ui_rect); }

float osc::Camera::view_volume_height_at_depth(float depth) const { return impl_->view_volume_height_at_depth(depth); }
