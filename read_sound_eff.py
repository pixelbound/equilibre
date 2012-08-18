import sys
import binascii
import struct

def read_sound_eff(f):
    entries = []
    while True:
        data = f.read(84)
        if len(data) < 84:
            break
        entry = struct.unpack("IIIIffff52s", data)
        entries.append(entry)
    return entries

if __name__ == "__main__":
    with open(sys.argv[1], "r") as f:
        entries = read_sound_eff(f)
    for entry in entries:
        flags, field2, field3, entryID, x, y, z, r, field4 = entry
        print("%08x %d %d %d %s %f %f %f %f" % (flags, field2, field3, entryID, binascii.hexlify(field4), x, y, z, r))
        #print(binascii.hexlify(field4))
