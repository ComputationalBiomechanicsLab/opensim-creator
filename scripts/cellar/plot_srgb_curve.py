import matplotlib.pyplot as plt
import numpy as np

srgb_input = np.arange(0.0, 1.0, 0.01)
linear_output = np.power(srgb_input, 2.2)
line, = plt.plot(srgb_input, linear_output)
srgb_output = np.power(linear_output, 1/2.2)
line, = plt.plot(linear_output, srgb_output)
line, = plt.plot(srgb_input, srgb_input)
plt.show()
