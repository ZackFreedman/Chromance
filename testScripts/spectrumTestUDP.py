
"""
Notebook for streaming data from a microphone in realtime

audio is captured using pyaudio
then converted from binary data to ints using struct
then displayed using matplotlib

if you don't have pyaudio, then run

>>> pip install pyaudio

note: with 2048 samples per chunk, I'm getting 20FPS
"""

import pyaudio
import os
import struct
import numpy as np
import matplotlib.pyplot as plt
import time
from tkinter import TclError
from scipy.fftpack import fft
import socket
import time
import random

UDP_IP = "192.168.5.50"
UDP_PORT = 8888

fd = open("out.bin", "wb")

b = bytearray()

sock = socket.socket(socket.AF_INET, # Internet
                    socket.SOCK_DGRAM) # UDP

# for j in range(0, 457):
buff = bytearray(457*3)

# use this backend to display in separate Tk window
# matplotlib tk

def clear():
    buff.clear()
    for i in range(0, 457):
        buff.append(0) # R
        buff.append(0) # G
        buff.append(0) # B
    
def setSegment(idx, r , g , b):
    global buff
    segmentIdx = idx*36
    for i in range(0, 12):
        buff[segmentIdx + (i*3)] = r # R
        buff[segmentIdx + (i*3) + 1] = g # G
        buff[segmentIdx + (i*3) + 2] = b # B
    
# constants
CHUNK =  4096 *2      # samples per frame
FORMAT = pyaudio.paInt16     # audio format (bytes per sample?)
CHANNELS = 1             # single channel for microphone
RATE = 48000                 # samples per second

# create matplotlib figure and axes
fig, (ax1, ax2) = plt.subplots(2, figsize=(15, 7))

# pyaudio class instance
p = pyaudio.PyAudio()

# stream object to get data from microphone
stream = p.open(
    format=FORMAT,
    channels=CHANNELS,
    rate=RATE,
    input=True,
    frames_per_buffer=CHUNK
)

# variable for plotting
x = np.arange(0, 2 * CHUNK, 2)       # samples (waveform)
xf = np.linspace(0, RATE, CHUNK)     # frequencies (spectrum)

# create a line object with random data
line, = ax1.plot(x, np.random.rand(CHUNK), '-', lw=2)

# create semilogx line for spectrum
line_fft, = ax2.semilogx(xf, np.random.rand(CHUNK), '-', lw=2)

# format waveform axes
ax1.set_title('AUDIO WAVEFORM')
ax1.set_xlabel('samples')
ax1.set_ylabel('volume')
ax1.set_ylim(0, 255)
ax1.set_xlim(0, 2 * CHUNK)
plt.setp(ax1, xticks=[0, CHUNK, 2 * CHUNK], yticks=[0, 128, 255])

# format spectrum axes
ax2.set_xlim(20, RATE / 2)

print('stream started')

# for measuring frame rate
frame_count = 0
start_time = time.time()
# plt.show()
while True:
    
    # binary data
    data = stream.read(CHUNK)  
    
    # convert data to integers, make np array, then offset it by 127
    data_int = struct.unpack(str(2 * CHUNK) + 'B', data)
    
    # create np array and offset by 128
    data_np = np.array(data_int, dtype='b')[::2] + 128
    
    line.set_ydata(data_np)
    
    # compute FFT and update line
    yf = fft(data_int)
    line_fft.set_ydata(np.abs(yf[0:CHUNK])  / (128 * CHUNK))
    outData = np.abs(yf[0:CHUNK])  / (128 * CHUNK)
    clear()
    setSegment(4, int(outData[80]*255%255), 0, 0)
    setSegment(6, int(outData[150]*255%255), 0, 0)
    setSegment(1, int(outData[400]*255%255), 0, 0)
    setSegment(17, int(outData[800]*255%255), 0, 0)
    setSegment(26, int(outData[1000]*255%255), 0, 0)
    setSegment(30, int(outData[2000]*255%255), 0, 0)
    sock.sendto(buff, (UDP_IP, UDP_PORT))
    # update figure canvas
    try:
        fig.canvas.draw()
        fig.canvas.flush_events()
        frame_count += 1
        
    except TclError:
        
        # calculate average frame rate
        frame_rate = frame_count / (time.time() - start_time)
        
        print('stream stopped')
        print('average frame rate = {:.0f} FPS'.format(frame_rate))
        break

    
    # max = 0;
    # maxIdx = 0;
    # tmpArr = np.abs(yf[0:CHUNK])  / (128 * CHUNK)
    # for i in range(0,tmpArr.size):
    #     tmp = tmpArr[i]
    #     if ( tmp > max):
    #         max = tmp;
    #         maxIdx = i;
    # print(data[30])
    # update figure canvas
