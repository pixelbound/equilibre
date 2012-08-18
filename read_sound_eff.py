import sys
import binascii
import struct

def read_sound_eff(f):
    entries = []
    while True:
        data = f.read(84)
        if len(data) < 84:
            break
        entry = struct.unpack("4I4f8I20s", data)
        entries.append(entry)
    return entries

if __name__ == "__main__":
    with open(sys.argv[1], "r") as f:
        entries = read_sound_eff(f)
    for entry in entries:
        formats, fields = [], []
        # flags, field2, field3, entryID
        formats.extend(["%08x", "%d", "%d", "%d"])
        fields.extend(entry[0:4])
        # field4-field11
        formats.extend(["%d"] * 4)
        formats.extend(["%04d"] * 2)
        formats.extend(["%08x"] * 1)
        formats.extend(["%04d"] * 1)
        fields.extend(entry[8:16])
        # x, y, z, r
        formats.extend(["%f"] * 4)
        fields.extend(entry[4:8])
        print(" ".join([fmt % field for (fmt, field) in zip(formats, fields)]))
