from scipy.io import wavfile as wf
import numpy as np
import matplotlib.pyplot as plt

N = 4

filename = "interface"

in_file = "../output/" + filename + ".pcm"

# FIXME: there is currently a spike at the beginning of the recording, cut it off for now
offset = 6 * 4

# Get the raw data to a numpy u32 array
raw_u32 = np.fromfile(in_file, dtype=np.uint32, count=-1, sep='')[offset:]
number_samples = int(raw_u32.shape[0] / 6)
reshaped_arr = np.reshape(raw_u32, (number_samples, 6))

# Extract each channel in its own WAV file
for i in range(0, 6):
    channel = reshaped_arr[:, i]
    # Convert the raw data to float32 between 1.0 and -1.0
    temp = np.subtract(channel, (channel.max() + channel.min()) / 2)
    norm_f32 = np.divide(temp, max(abs(temp)))
    print(channel.max())
    print(channel.min())
    wf.write("../output/interface_chan" + str(i + 1) + ".wav", 16000 * 4, norm_f32)
