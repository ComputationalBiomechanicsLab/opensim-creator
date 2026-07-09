#include "camera_v2.h"

#include <liboscar/graphics/camera_clipping_planes.h>
#include <liboscar/graphics/camera_projection.h>
#include <liboscar/maths/angle.h>
#include <liboscar/maths/matrix4x4.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/vector.h>
#include <liboscar/utilities/enum_helpers.h>

#include <optional>

using namespace osc;
using namespace osc::literals;

class osc::CameraV2::Impl final {
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

    Vector3 direction() const                    { return direction_; }
    void set_direction(const Vector3& direction) { direction_ = direction; }

    Vector3 up() const             { return up_; }
    void set_up(const Vector3& up) { up_ = up; }

    Matrix4x4 view_matrix() const
    {
        if (maybe_view_matrix_override_) {
            return *maybe_view_matrix_override_;
        }
        return look_at(position(), position() + direction(), up());
    }

    Matrix4x4 inverse_view_matrix() const
    {
        return inverse(view_matrix());
    }

    std::optional<Matrix4x4> view_matrix_override() const                        { return maybe_view_matrix_override_; }
    void set_view_matrix_override(std::optional<Matrix4x4> view_matrix_override) { maybe_view_matrix_override_ = view_matrix_override; }

    Matrix4x4 projection_matrix(float aspect_ratio) const
    {
        if (maybe_projection_matrix_override_) {
            return *maybe_projection_matrix_override_;
        }
        if (projection() == CameraProjection::Perspective) {
            return perspective(
                vertical_field_of_view_,
                aspect_ratio,
                clipping_planes_.znear,
                clipping_planes_.zfar
            );
        }
        // else: orthographic
        static_assert(osc::num_options<CameraProjection>() == 2);
        const float height = orthographic_size_;
        const float width = height * aspect_ratio;

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

private:
    CameraProjection projection_ = CameraProjection::Default;
    float orthographic_size_ = 2.0f;
    Radians vertical_field_of_view_ = 90_deg;
    CameraClippingPlanes clipping_planes_{0.1f, 100.0f};
    Vector3 position_;
    Vector3 direction_ = {0.0f, 0.0f, -1.0f};
    Vector3 up_ = {0.0f, 1.0f, 0.0f};
    std::optional<Matrix4x4> maybe_view_matrix_override_;
    std::optional<Matrix4x4> maybe_projection_matrix_override_;
};

osc::CameraV2::CameraV2() : impl_{make_cowv<Impl>()} {}
bool osc::operator==(const CameraV2& lhs, const CameraV2& rhs) { return lhs.impl_ == rhs.impl_ or *lhs.impl_ == *rhs.impl_; }

void osc::CameraV2::reset() { impl_.upd()->reset(); }

CameraProjection osc::CameraV2::projection() const              { return impl_->projection(); }
void osc::CameraV2::set_projection(CameraProjection projection) { impl_.upd()->set_projection(projection); }

float osc::CameraV2::orthographic_size() const                     { return impl_->orthographic_size(); }
void osc::CameraV2::set_orthographic_size(float orthographic_size) { impl_.upd()->set_orthographic_size(orthographic_size); }

Radians osc::CameraV2::vertical_field_of_view() const                          { return impl_->vertical_field_of_view(); }
void osc::CameraV2::set_vertical_field_of_view(Radians vertical_field_of_view) { impl_.upd()->set_vertical_field_of_view(vertical_field_of_view); }

Radians osc::CameraV2::horizontal_field_of_view(float aspect_ratio) const { return impl_->horizontal_field_of_view(aspect_ratio); }

CameraClippingPlanes osc::CameraV2::clipping_planes() const                   { return impl_->clipping_planes(); }
void osc::CameraV2::set_clipping_planes(CameraClippingPlanes clipping_planes) { impl_.upd()->set_clipping_planes(clipping_planes); }

float osc::CameraV2::near_clipping_plane() const                       { return impl_->near_clipping_plane(); }
void osc::CameraV2::set_near_clipping_plane(float near_clipping_plane) { impl_.upd()->set_near_clipping_plane(near_clipping_plane); }

float osc::CameraV2::far_clipping_plane() const                      { return impl_->far_clipping_plane(); }
void osc::CameraV2::set_far_clipping_plane(float far_clipping_plane) { impl_.upd()->set_far_clipping_plane(far_clipping_plane); }

Vector3 osc::CameraV2::position() const                   { return impl_->position(); }
void osc::CameraV2::set_position(const Vector3& position) { impl_.upd()->set_position(position); }

Vector3 osc::CameraV2::direction() const                    { return impl_->direction(); }
void osc::CameraV2::set_direction(const Vector3& direction) { impl_.upd()->set_direction(direction); }

Vector3 osc::CameraV2::up() const             { return impl_->up(); }
void osc::CameraV2::set_up(const Vector3& up) { impl_.upd()->set_up(up); }

Matrix4x4 osc::CameraV2::view_matrix() const                                    { return impl_->view_matrix(); }
Matrix4x4 osc::CameraV2::inverse_view_matrix() const                            { return impl_->inverse_view_matrix(); }
std::optional<Matrix4x4> osc::CameraV2::view_matrix_override() const            { return impl_->view_matrix_override(); }
void osc::CameraV2::set_view_matrix_override(std::optional<Matrix4x4> override) { impl_.upd()->set_view_matrix_override(override); }

Matrix4x4 osc::CameraV2::projection_matrix(float aspect_ratio) const                  { return impl_->projection_matrix(aspect_ratio); }
std::optional<Matrix4x4> osc::CameraV2::projection_matrix_override() const            { return impl_->projection_matrix_override(); }
void osc::CameraV2::set_projection_matrix_override(std::optional<Matrix4x4> override) { impl_.upd()->set_projection_matrix_override(override); }

Matrix4x4 osc::CameraV2::view_projection_matrix(float aspect_ratio) const         { return impl_->view_projection_matrix(aspect_ratio); }
Matrix4x4 osc::CameraV2::inverse_view_projection_matrix(float aspect_ratio) const { return impl_->inverse_view_projection_matrix(aspect_ratio); }
