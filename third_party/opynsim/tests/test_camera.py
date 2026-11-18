import opynsim as opyn
import pytest

def test_camera_properties_work_as_expected():
    import numpy as np

    camera = opyn.Camera()
    assert np.array_equal(camera.position, np.array([0.0, 0.0, 0.0]))
    assert np.array_equal(camera.direction, np.array([0.0, 0.0, -1.0]))
    assert np.array_equal(camera.up, np.array([0.0, 1.0, 0.0]))

    camera.position = np.array([1.0, 0.0, 0.0])
    assert np.array_equal(camera.position, np.array([1.0, 0.0, 0.0]))

    camera.direction = np.array([0.0, -1.0, 0.0])
    assert np.array_equal(camera.direction, np.array([0.0, -1.0, 0.0]))

    # The camera's `direction` should normalize on being set.
    camera.direction = np.array([0.0, -2.0, 0.0])  # length==2
    assert np.array_equal(camera.direction, np.array([0.0, -1.0, 0.0]))

    camera.up = np.array([1.0, 0.0, 0.0])
    assert np.array_equal(camera.up, np.array([1.0, 0.0, 0.0]))

    # The camera's `up` should normalize on being set.
    camera.up = np.array([3.0, 0.0, 0.0])  # length==3
    assert np.array_equal(camera.up, np.array([1.0, 0.0, 0.0]))

    # Vertical FoV defaults to close to 90 degrees (in radians)
    assert np.isclose(camera.vertical_field_of_view, np.deg2rad(90), rtol=1e-5, atol=1e-5)
    camera.vertical_field_of_view = np.deg2rad(45)
    assert np.isclose(camera.vertical_field_of_view, np.deg2rad(45), rtol=1e-5, atol=1e-5)

def test_camera_has_human_readable_repr():
    # It's useful for a camera to have a human-readable/copyable `repr` because
    # it lets people see what they should set their camera to (e.g. when rendering
    # to a texture).
    s = repr(opyn.Camera())
    assert "Camera(" in s
    assert "position=(" in s
    assert "direction=" in s

def test_can_construct_camera_from_position_direction_up():
    import numpy as np

    position = np.array([1.0, 2.0, 3.0])
    direction = np.array([-1.0, 0.0, 0.0])
    up = np.array([0.0, 1.0, 0.0])
    camera = opyn.Camera(
        position=position,
        direction=direction,
        up=up
    )

    assert np.array_equal(camera.position, position)
    tolerance = 0.0000001  # The camera doesn't store direction/up.
    np.testing.assert_allclose(camera.direction, direction, atol=tolerance)
    np.testing.assert_allclose(camera.up, up, atol=tolerance)

def test_look_at_constructs_expected_camera():
    import numpy as np

    position = np.array([1.0, 2.0, 3.0])
    target = np.array([-1.0, -1.0, -1.0])
    up = np.array([0.0, 1.0, 0.0])
    camera = opyn.Camera.look_at(position=position, target=target, up=up)

    assert np.array_equal(camera.position, position)
    tolerance = 0.0000001  # The camera doesn't store direction/up.
    expected_direction = (target - position) / np.linalg.norm(target - position)
    np.testing.assert_allclose(camera.direction, expected_direction, atol=tolerance)

def test_can_construct_camera_from_python_tuples():
    import numpy as np

    camera = opyn.Camera(
        position=(1, 2, 3),
        direction=(0, 0, 1),
        up=(0, 1, 0)
    )
    tolerance = 0.0000001  # The camera doesn't store direction/up.
    np.testing.assert_allclose(camera.position, np.array([1, 2, 3]), atol=tolerance)
    np.testing.assert_allclose(camera.direction, np.array([0, 0, 1]), atol=tolerance)
    np.testing.assert_allclose(camera.up, np.array([0, 1, 0]), atol=tolerance)

def test_can_assign_camera_position_with_python_tuple():
    import numpy as np

    camera = opyn.Camera()
    camera.position = (-2, 7, 57)
    assert np.array_equal(camera.position, np.array([-2, 7, 57]))

def test_can_assign_camera_position_with_python_list():
    import numpy as np

    camera = opyn.Camera()
    camera.position = [2, 2, 4]
    assert np.array_equal(camera.position, np.array([2, 2, 4]))

def test_can_construct_camera_from_python_list():
    import numpy as np

    camera = opyn.Camera(
        position=[3, 2, 1],
        direction=[1, 0, 0],
        up=[0, 0, 1]
    )
    tolerance = 0.0000001  # The camera doesn't store direction/up.
    np.testing.assert_allclose(camera.position, np.array([3, 2, 1]), atol=tolerance)
    np.testing.assert_allclose(camera.direction, np.array([1, 0, 0]), atol=tolerance)
    np.testing.assert_allclose(camera.up, np.array([0, 0, 1]), atol=tolerance)

def test_constructing_camera_from_non_numeric_list_raises_exception():
    with pytest.raises(Exception):
        opyn.Camera(
            position=[3, 2, "bacon"],
            direction=[1, 0, 0],
            up=[0, 0, 1],
        )
    with pytest.raises(Exception):
        opyn.Camera(
            position=[3, 2, 1],
            direction=[1, "lettuce", 0],
            up=[0, 0, 1],
        )
    with pytest.raises(Exception):
        opyn.Camera(
            position=[3, 2, 1],
            direction=[1, 0, 0],
            up=["tomato", 0, 1],
        )

def test_can_construct_camera_with_vertical_field_of_view_argument():
    import numpy as np

    camera = opyn.Camera(
        vertical_field_of_view=np.deg2rad(45)
    )
    assert np.isclose(camera.vertical_field_of_view, np.deg2rad(45))
