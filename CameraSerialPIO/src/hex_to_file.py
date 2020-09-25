#! /usr/bin/env python3
import binascii

filename = input('Filename (ending in .bmp): ')

lines = []
print('Paste hexdump, then press Enter twice.')
line = input()
while line:
    b = binascii.unhexlify(line)
    lines.append(b)
    line = input()

data = b''.join(lines)
with open(filename, 'wb') as f:
    f.write(data)
