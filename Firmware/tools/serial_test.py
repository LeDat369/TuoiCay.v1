#!/usr/bin/env python3
# simple serial test script for pump on/off timeout
import time
import serial
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--port', '-p', default='COM4')
parser.add_argument('--baud', '-b', type=int, default=115200)
parser.add_argument('--onfor', '-s', type=int, default=5)
args = parser.parse_args()

ser = serial.Serial(args.port, args.baud, timeout=1)
print('Opened', args.port, args.baud)
# drain
time.sleep(0.1)
ser.reset_input_buffer()

def send(cmd):
    ser.write((cmd + '\n').encode('utf-8'))
    time.sleep(0.05)
    out = b''
    # read available lines for 1s
    t0 = time.time()
    while time.time() - t0 < 1.0:
        line = ser.readline()
        if not line:
            break
        out += line
    return out.decode('utf-8', errors='replace')

print(send('pump setmax 10'))
print('Requesting pump onfor', args.onfor, 's')
print(send(f'pump onfor {args.onfor}'))

start = time.time()
# poll status every 0.5s until pump status is false or timeout
while True:
    time.sleep(0.5)
    resp = send('pump status')
    print(time.time()-start, resp.strip())
    if 'pump_is_on=0' in resp or time.time() - start > (args.onfor + 10):
        break

print('Test finished')
ser.close()
