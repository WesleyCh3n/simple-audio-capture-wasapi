# type: ignore
import numpy as np
import matplotlib.pyplot as plt

# data = np.genfromtxt('./build/left_channel.csv', delimiter=',')
# fig, ax = plt.subplots(figsize=(10,1))
# ax.plot(data)
# ax.set_ylim(-1, 1)
#
# plt.show()

# freq = np.arange((480 / 2) + 1) / (float(480) / 44100)

import pandas as pd


fig, ax = plt.subplots()
df = pd.read_csv("./build/bin.csv");
# df = pd.read_csv("./build/db.csv");
df.plot(ax=ax)
ax.legend(bbox_to_anchor=(1.0, 1.0))

plt.show()
