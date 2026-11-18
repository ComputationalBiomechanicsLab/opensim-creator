#include "mw_document.h"

#include <liboscar/graphics/geometries/cylinder_geometry.h>
#include <liboscar/graphics/geometries/sphere_geometry.h>

osc::MwDocument::MwDocument() :
    source_mesh{SphereGeometry{{.num_width_segments = 16, .num_height_segments = 16}}.mesh()},
    destination_mesh{CylinderGeometry{{.height = 2.0f, .num_radial_segments = 16}}.mesh()}
{}
