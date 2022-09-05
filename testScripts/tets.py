import pyaudio
import struct
import matplotlib.pyplot as plt
import numpy as np
from scipy import signal

p = pyaudio.PyAudio()
FORMAT = pyaudio.paInt16
CHANNELS = 1
RATE = 20000
CHUNK = int(RATE/20)
stream = p.open(format=FORMAT, channels=CHANNELS, rate=RATE, input=True, frames_per_buffer=CHUNK)



plt.ylabel('Frequency [Hz]')
plt.xlabel('Time [sec]')

while True:
    data = stream.read(CHUNK)
    data = np.array(struct.unpack(str(2 * CHUNK) + 'B', data), dtype='b')
    f, t, Sxx = signal.spectrogram(data, fs=CHUNK)
    dBS = 10 * np.log10(Sxx)
    plt.pcolormesh(t, f, dBS)
    plt.pause(0.005)