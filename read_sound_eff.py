import sys
import binascii
import struct

def read_sound_eff(f):
    entries = []
    while True:
        data = f.read(84)
        if len(data) < 84:
            break
        entry = struct.unpack("4I4f13I", data)
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
        # field8-field11
        formats.extend(["%d"] * 4)
        fields.extend(entry[8:12])
        # field12-field15
        formats.extend(["%04d", "%04d", "%08x", "%04d"])
        fields.extend(entry[12:16])
        # field16-field21
        formats.extend(["%05d", "%05d", "%d", "%04d", "%d"])
        fields.extend(entry[16:21])
        # x, y, z, r
        formats.extend(["%.2f"] * 4)
        fields.extend(entry[4:8])
        print(" ".join([fmt % field for (fmt, field) in zip(formats, fields)]))
