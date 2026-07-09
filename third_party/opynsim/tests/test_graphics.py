import numpy as np
import opynsim as opyn
import opynsim.examples
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
    ], dtype=np.float64)

    assert vertices.dtype == np.float64
    mesh.vertices = vertices
    assert mesh.vertices.dtype == np.float32, "The Mesh class always returns vertices as float32"
    assert np.array_equal(mesh.vertices, vertices)

def test_can_set_mesh_faces():
    mesh = opyn.graphics.Mesh()
    vertices = np.array([
        [-1.0, -1.0, 0.0],
        [1.0, -1.0, 0.0],
        [0.0, 1.0, 0.0],
    ], dtype=np.float64)
    faces = np.array([0, 1, 2], dtype=np.int64)

    assert vertices.dtype == np.float64
    assert faces.dtype == np.int64
    mesh.vertices = vertices
    mesh.faces = faces
    assert mesh.faces.dtype == np.int32, "The mesh class always returns faces/indices as int32 - even if something else was supplied"
    assert np.array_equal(mesh.faces, faces)

def test_render_model_in_state_has_expected_background_color():
    model = opyn.examples.pendulum_model()
    state = model.initial_state(realized_to=opyn.STAGE_REPORT)
    render = opyn.graphics.render_model_in_state(model, state, background_color=[1.0, 0.0, 0.0, 1.0])
    top_left_pixel = render.pixels_rgba32()[0, 0]
    expected_pixel = np.array([255, 0, 0, 255], dtype=np.uint8)
    assert np.array_equal(top_left_pixel, expected_pixel)

def test_render_model_in_state_can_be_given_a_scene_cache():
    model = opyn.examples.pendulum_model()
    state = model.initial_state(realized_to=opyn.STAGE_REPORT)
    scene_cache = opyn.graphics.SceneCache()

    # i.e. it can be reused
    opyn.graphics.render_model_in_state(model, state, background_color=[1.0, 0.0, 0.0, 1.0], scene_cache=scene_cache)
    opyn.graphics.render_model_in_state(model, state, background_color=[0.0, 1.0, 0.0, 1.0], scene_cache=scene_cache)
