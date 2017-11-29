from scipy.io import wavfile as wf
import numpy as np
import matplotlib.pyplot as plt

in_file = "../output/out.pcm"
out_file = "../output/out.wav"

# FIXME: there is currently a spike at the beginning of the recording, cut it off for now
offset = 20000

# Get the raw data to a numpy u32 array
raw_u32 = np.fromfile(in_file, dtype=np.uint32, count=-1, sep='')
raw_u32 = raw_u32[offset:]

# Convert the raw data to float32 between 1.0 and -1.0
values_range = raw_u32.max() - raw_u32.min()
norm_f32 = np.subtract(raw_u32, (raw_u32.max() + raw_u32.min()) / 2)
norm_f32 = np.divide(norm_f32, max(abs(norm_f32))).astype(np.float32)

plt.figure()
plt.plot(norm_f32)
plt.show()

# Write data to wav file
wf.write(out_file, 19297, norm_f32)
