import struct
import sys
import os
from s3d import S3DArchive
from wld import WLDData

def stringhex(s):
    return " ".join(["%02x%02x%02x%02x" % tuple(s[i*4:i*4+4]) for i in range(0, len(s)//4)])

if len(sys.argv) < 2:
    print("usage: %s <.S3D file> [WLD name]" % sys.argv[0])
s3dPath = sys.argv[1]
if len(sys.argv) < 3:
    wldName = os.path.basename(s3dPath).replace(".s3d", ".wld")
else:
    wldName = sys.argv[2]
with S3DArchive(s3dPath) as a:
    wld = WLDData.fromArchive(a, wldName)
    for frag in wld.fragments.values():
        print("%05d 0x%02x %s %s" % (frag.ID, frag.type, frag.name or '', stringhex(frag.data)))
