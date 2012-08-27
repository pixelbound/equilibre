# Copyright (C) 2012 PiB <pixelbound@gmail.com>
# 
# EQuilibre is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

import struct
import sys
import os
from s3d import S3DArchive
from wld import WLDData

def stringhex(s):
    return " ".join(["%08x" % struct.unpack("I",s[i*4:i*4+4]) for i in range(0, len(s)//4)])

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
        print("%04x 0x%02x %s %s" % (frag.ID, frag.type, frag.name or '', stringhex(frag.data)))
