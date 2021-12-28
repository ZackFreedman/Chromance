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

def clear():
    buff.clear()
    for i in range(0, 457):
        buff.append(0) # R
        buff.append(0) # G
        buff.append(0) # B
    
def setSegment(idx, r , g , b):
    global buff
    segmentIdx = idx*36
    for i in range(0, random.randint(0,12)):
        buff[segmentIdx + (i*3)] = r # R
        buff[segmentIdx + (i*3) + 1] = g # G
        buff[segmentIdx + (i*3) + 2] = b # B
    
for i in range(0, 2000):
    clear()
    setSegment(4, 10, 40, 0)
    setSegment(6, 10, 40, 0)
    setSegment(1, 10, 40, 0)
    setSegment(17, 10, 40, 0)
    setSegment(26, 10, 40, 0)
    setSegment(30, 10, 40, 0)
    setSegment(29, 10, 40, 0)
    sock.sendto(buff, (UDP_IP, UDP_PORT))
    time.sleep(0.02)
    print(i)
    # print(buff.__len__())
# fd.write(b)
# fd.flush()
# fd.close()