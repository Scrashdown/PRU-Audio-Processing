from scipy.io import wavfile as wf
import numpy as np
import matplotlib.pyplot as plt

in_file = "../output/16bits_8chan_long.pcm"
out_file = "../output/16bits_8chan_long.wav"

# FIXME: there is currently a spike at the beginning of the recording, cut it off for now
offset = 20000

# Get the raw data to a numpy u16 array
raw_u16 = np.fromfile(in_file, dtype=np.uint16, count=-1, sep='')
number_samples = int(raw_u16.shape[0] / 8)
reshaped_arr = np.reshape(raw_u16, (number_samples, 8))

first_channel = reshaped_arr[:, 0]

# Convert the raw data to float16 between 1.0 and -1.0
norm_f32 = np.subtract(first_channel, (first_channel.max() + first_channel.min()) / 2)
norm_f32 = np.divide(norm_f32, max(abs(norm_f32)))

plt.figure()
plt.plot(norm_f32)
plt.show()

# Write data to wav file
wf.write(out_file, 16000, norm_f32)
