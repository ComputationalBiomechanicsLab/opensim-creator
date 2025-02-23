import numpy as np
from opensimcreator import tps3d
import pytest

def test_solve_coefficients_raises_exception_if_given_empty_source_and_destination_landmarks():
    source_landmarks = np.array([])
    destination_landmarks = np.array([[1, 0, 0]])

    with pytest.raises(Exception):
        # Should raise, because at least one landmark pair is required
        # in order to solve the coefficients.
        tps3d.solve_coefficients(source_landmarks, destination_landmarks)

def test_solve_coefficients_raises_exception_if_number_of_source_landmarks_does_not_match_number_of_destination_landmarks():
    source_landmarks = np.array([
        [1,2,3],
        [4,5,6],
        [7,8,9],
        [10,11,12],
    ])
    destination_landmarks = np.array([
        [13, 14, 15],
    ])

    with pytest.raises(Exception):
        # Should raise, because there's a different number of
        # source/destination landmarks.
        tps3d.solve_coefficients(source_landmarks, destination_landmarks)

def test_solve_coefficients_works_if_the_number_of_source_landmarks_matches_the_number_of_destination_landmarks_and_is_nonzero():
    source_landmarks = np.array([[1,2,3]])
    destination_landmarks = np.array([[4,5,6]])

    # Should work, although the inputs are minimal.
    tps3d.solve_coefficients(source_landmarks, destination_landmarks)

def test_solve_coefficients_accepts_named_arguments():
    slms = np.array([[1,2,3]])
    dlms = np.array([[4,5,6]])

    # I.e. the function argument names are named+fixed, so that downstream code
    # can write in a more readable style.
    tps3d.solve_coefficients(source_landmarks=slms, destination_landmarks=dlms)

def test_solve_coefficients_returns_object_with_warp_point_method():
    source_landmarks = np.array([[1,2,3]])
    destination_landmarks = np.array([[4,5,6]])

    coefs = tps3d.solve_coefficients(source_landmarks, destination_landmarks)
    warped_point = coefs.warp_point(np.array([7,8,9]))

    assert isinstance(warped_point, np.ndarray)

def test_coefficients_repr_shows_user_a1_a2_and_a3():
    # Test that the user is able to, at a quick glance in the REPL or
    # a log/debug/error message, see the three affine coefficients of
    # the TPS warp.

    # Create an identity-like mapping
    coefs = tps3d.solve_coefficients(
        source_landmarks=np.array([
            [0, 1, 0],
            [1, 0, 0],
            [0, 0, 1],
        ]),
        destination_landmarks=np.array([
            [0, 1, 0],
            [1, 0, 0],
            [0, 0, 1],
        ])
    )

    assert 'a1 = ' in repr(coefs)
    assert 'a2 = ' in repr(coefs)
    assert 'a3 = ' in repr(coefs)
    assert 'a4 = ' in repr(coefs)

def test_coefficients_has_a1_a2_a3_readable_props():
    coefs = tps3d.solve_coefficients(
        source_landmarks=np.array([
            [0, 1, 0],
            [1, 0, 0],
            [0, 0, 1],
        ]),
        destination_landmarks=np.array([
            [0, 1, 0],
            [1, 0, 0],
            [0, 0, 1],
        ])
    )

    assert isinstance(coefs.a1, np.ndarray)
    assert isinstance(coefs.a2, np.ndarray)
    assert isinstance(coefs.a3, np.ndarray)
    assert isinstance(coefs.a4, np.ndarray)

def test_solve_coefficients_returns_object_that_performs_expected_warp():
    # Create an identity-like mapping
    coefs = tps3d.solve_coefficients(
        source_landmarks=np.array([
            [0, 1, 0],
            [1, 0, 0],
            [0, 0, 1],
        ]),
        destination_landmarks=np.array([
            [0, 1, 0],
            [1, 0, 0],
            [0, 0, 1],
        ])
    )

    # Warp a datapoint that's exactly on top of a source landmark
    warped = coefs.warp_point(np.array([0, 1, 0]))

    assert warped[0] == pytest.approx(0, nan_ok=False)
    assert warped[1] == pytest.approx(1, nan_ok=False)
    assert warped[2] == pytest.approx(0, nan_ok=False)
