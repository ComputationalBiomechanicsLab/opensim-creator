import numpy as np
from opensimcreator import tps

def test_solve_coefficients_works_in_basic_case():
	tps.solve_coefficients(
		source_landmarks = np.array([
			[1,2,3],
			[4,5,6],
			[7,8,9],
			[10,11,12],
		]),
		destination_landmarks = np.array([
			[13, 14, 15],
		])
	)
