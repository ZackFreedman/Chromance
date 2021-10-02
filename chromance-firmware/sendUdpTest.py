import socket
import time

UDP_IP = "192.168.5.50"
UDP_PORT = 8888

fd = open("out.bin", "wb")

b = bytearray()

sock = socket.socket(socket.AF_INET, # Internet
                    socket.SOCK_DGRAM) # UDP

# for j in range(0, 457):
b = bytearray()
j=0
for i in range(0, j):
    b.append(10) # R
    b.append(j%50) # G
    b.append(00) # B

for i in range(j, 457):
    b.append(10) # R
    b.append(0) # G
    b.append(25) # B

sock.sendto(b, (UDP_IP, UDP_PORT))
time.sleep(0.01)
# fd.write(b)
# fd.flush()
# fd.close()