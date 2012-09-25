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
import re
import os
import sys
from s3d import S3DArchive, readStruct
from wld import WLDData

def list_palettes(dir_path):
    files = [f for f in os.listdir(dir_path) if f.endswith(".s3d")]
    files.sort()

    print("s3d,wld,palettes,maxMatsPerPalette,totalMats")
    for file in files:
        path = os.path.join(dir_path, file)
        with S3DArchive(path) as a:
            for entry in a.files:
                if not entry.endswith(".wld"):
                    continue
                try:
                    wld = WLDData.fromArchive(a, entry)
                except Exception as e:
                    continue
                palettes = list(wld.fragmentsByType(0x31))
                if palettes:
                    max_mats = max(len(pal.Textures) for pal in palettes)
                    total_mats = sum(len(pal.Textures) for pal in palettes)
                    print("%s,%s,%d,%d,%d" % (file, entry, len(palettes), max_mats, total_mats))

if __name__ == "__main__":
    if len(sys.argv) > 1:
        dir_path = sys.argv[1]
    else:
        dir_path = os.path.join(os.getcwd(), "Data")
    list_palettes(dir_path)
