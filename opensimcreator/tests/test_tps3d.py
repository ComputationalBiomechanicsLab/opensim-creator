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
