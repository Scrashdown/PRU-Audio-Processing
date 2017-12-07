from scipy.io import wavfile as wf
import numpy as np
import matplotlib.pyplot as plt

N = 4

in_file = "../output/32bits_6chan.pcm"
out_file = "../output/32bits_6chan.wav"

# FIXME: there is currently a spike at the beginning of the recording, cut it off for now
offset = 6 * 4

# Get the raw data to a numpy u32 array
raw_u32 = np.fromfile(in_file, dtype=np.uint32, count=-1, sep='')[offset:]
number_samples = int(raw_u32.shape[0] / 6)
reshaped_arr = np.reshape(raw_u32, (number_samples, 6))

channel = reshaped_arr[:, 0]

# Convert the raw data to float32 between 1.0 and -1.0
temp = np.subtract(channel, (channel.max() + channel.min()) / 2)
norm_f32 = np.divide(temp, max(abs(temp)))

plt.figure()
plt.plot(norm_f32)
plt.show()

# Write data to wav file
wf.write(out_file, 16000 * 4, norm_f32)
