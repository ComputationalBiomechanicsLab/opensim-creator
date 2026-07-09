import opynsim as opyn
import opynsim.examples
import opynsim.graphics

def test_can_initialize_blank_mesh():
    mesh = opyn.graphics.Mesh()
    assert mesh.vertices.size == 0

def test_can_set_mesh_vertices():
    import numpy as np

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
    import numpy as np

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
    import numpy as np

    model = opyn.examples.pendulum_model()
    state = model.initial_state(realized_to=opyn.STAGE_REPORT)
    render = opyn.graphics.render_model_in_state(model, state, background_color=opyn.graphics.Color(1.0, 0.0, 0.0, 1.0))
    top_left_pixel = render.pixels_rgba32()[0, 0]
    expected_pixel = np.array([255, 0, 0, 255], dtype=np.uint8)
    assert np.array_equal(top_left_pixel, expected_pixel)

def test_render_model_in_state_can_be_given_a_scene_cache():
    model = opyn.examples.pendulum_model()
    state = model.initial_state(realized_to=opyn.STAGE_REPORT)
    scene_cache = opyn.graphics.SceneCache()

    # i.e. it can be reused
    opyn.graphics.render_model_in_state(model, state, background_color=opyn.graphics.Color(1.0, 0.0, 0.0, 1.0), scene_cache=scene_cache)
    opyn.graphics.render_model_in_state(model, state, background_color=opyn.graphics.Color(0.0, 1.0, 0.0, 1.0), scene_cache=scene_cache)

def test_color_constructor_behaves_as_expected():
    assert opyn.graphics.Color()     == opyn.graphics.Color(0.0, 0.0, 0.0, 0.0)
    assert opyn.graphics.Color.clear == opyn.graphics.Color(0.0, 0.0, 0.0, 0.0)
    assert opyn.graphics.Color.white == opyn.graphics.Color(1.0, 1.0, 1.0, 1.0)
    assert opyn.graphics.Color.black == opyn.graphics.Color(0.0, 0.0, 0.0, 1.0)
    assert opyn.graphics.Color.red   == opyn.graphics.Color(1.0, 0.0, 0.0, 1.0)  # alpha should default to 1.0
    assert opyn.graphics.Color.red   == opyn.graphics.Color(1.0, 0.0, 0.0)
    assert opyn.graphics.Color.green == opyn.graphics.Color(0.0, 1.0, 0.0)
    assert opyn.graphics.Color.blue  == opyn.graphics.Color(0.0, 0.0, 1.0)

    assert opyn.graphics.Color.clear != opyn.graphics.Color.white
    assert opyn.graphics.Color.black != opyn.graphics.Color.white
    assert opyn.graphics.Color.red   != opyn.graphics.Color.green

    assert opyn.graphics.Color(b=0.5, r=1.0, g=0.25, a=0.1) == opyn.graphics.Color(1.0, 0.25, 0.5, 0.1)

    # And it's hash-able
    assert len({opyn.graphics.Color.red, opyn.graphics.Color.green, opyn.graphics.Color.red}) == 2

def test_color_can_be_stringified():
    assert repr(opyn.graphics.Color.green) == "Color(0, 1, 0, 1)"

def test_color_properties_work_as_expected():
    color = opyn.graphics.Color.green

    assert color.r == 0.0
    color.r = 0.25
    assert color.r == 0.25

    assert color.g == 1.0
    color.g = 0.5
    assert color.g == 0.5

    assert color.b == 0.0
    color.b = 0.75
    assert color.b == 0.75

    assert color.a == 1.0
    color.a = 0.5
    assert color.a == 0.5

def test_camera_properties_work_as_expected():
    import numpy as np

    camera = opyn.graphics.Camera()
    assert np.array_equal(camera.position, np.array([0.0, 0.0, 0.0]))
    assert np.array_equal(camera.direction, np.array([0.0, 0.0, -1.0]))
    assert np.array_equal(camera.up, np.array([0.0, 1.0, 0.0]))

    camera.position = np.array([1.0, 0.0, 0.0])
    assert np.array_equal(camera.position, np.array([1.0, 0.0, 0.0]))

    camera.direction = np.array([0.0, -1.0, 0.0])
    assert np.array_equal(camera.direction, np.array([0.0, -1.0, 0.0]))

    camera.up = np.array([1.0, 0.0, 0.0])
    assert np.array_equal(camera.up, np.array([1.0, 0.0, 0.0]))
