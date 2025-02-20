import numpy as np
from opensimcreator import tps

def test_call_solve_coefficients_on_empty_numpy_array_raises_exception():
	tps.inspect(np.array([
		[1,2,3],
		[4,5,6],
		[7,8,9],
		[10,11,12],
	]))
	assert True
