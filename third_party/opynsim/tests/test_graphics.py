import numpy as np
import opynsim as opyn
import opynsim.graphics

def test_can_initialize_blank_mesh():
    mesh = opyn.graphics.Mesh()
    assert mesh.vertices.size == 0

def test_can_set_mesh_vertices():
    mesh = opyn.graphics.Mesh()
    vertices = np.array([
        [-1.0, -1.0, 0.0],
        [ 1.0, -1.0, 0.0],
        [ 0.0,  1.0, 0.0],
    ])
    assert vertices.dtype == np.float64
    mesh.vertices = vertices
    assert mesh.vertices.dtype == np.float32, "The Mesh class always returns vertices as float32"
    assert np.array_equal(mesh.vertices, vertices)
