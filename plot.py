# type: ignore
import numpy as np
import matplotlib.pyplot as plt

data = np.genfromtxt('./build/left_channel.csv', delimiter=',')
fig, ax = plt.subplots(figsize=(10,1))
ax.plot(data)
ax.set_ylim(-1, 1)

plt.show()
