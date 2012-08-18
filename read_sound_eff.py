import sys
import struct

def read_sound_eff(f):
    entries = []
    while True:
        data = f.read(84)
        if len(data) < 84:
            break
        entry = struct.unpack("12sIffff52s", data)
        entries.append(entry)
    return entries

if __name__ == "__main__":
    with open(sys.argv[1], "r") as f:
        entries = read_sound_eff(f)
    for entry in entries:
        print(repr(entry))
