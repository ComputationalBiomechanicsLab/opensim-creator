#include "Transform.hpp"

#include "src/Bindings/GlmHelpers.hpp"
#include "src/Maths/Geometry.hpp"

#include <iostream>

std::ostream& osc::operator<<(std::ostream& o, Transform const& t)
{
    return o << "Transform(position = " << t.position << ", rotation = " << t.rotation << ", scale = " << t.scale << ')';
}

glm::vec3 osc::operator*(Transform const& t, glm::vec3 const& p) noexcept
{
    return TransformPoint(t, p);
}

osc::Transform& osc::operator+=(Transform& t, Transform const& o) noexcept
{
    t.position += o.position;
    t.rotation += o.rotation;
    t.scale += o.scale;
    return t;
}

osc::Transform& osc::operator/=(Transform& t, float s) noexcept
{
    t.position /= s;
    t.rotation /= s;
    t.scale /= s;
    return t;
}
