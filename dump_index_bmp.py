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
from s3d import S3DArchive, readStruct

def loadIndexedBitmap(stream):
    # read file header
    fileHeader = readStruct(stream, "<2sIHHI")
    if fileHeader[0] != b"BM":
        return 0, 0, None
    bmpHeaderSize = readStruct(stream, "I")[0]
    bmpHeader = stream.read(bmpHeaderSize - 4)
    (biWidth, biHeight, biPlanes, biBitCount, biCompression, 
        biSizeImage, biXPelsPerMeter, biYPelsPerMeter, biClrUsed, 
        biClrImportant) = struct.unpack("iiHHIIiiII", bmpHeader[0:36])
    if biBitCount != 8:
        return 0, 0, None
    
    # read color table
    if biClrUsed == 0:
        biClrUsed = 256
    colorTableData = stream.read(biClrUsed * 4)
    pixelCount = biWidth * biHeight
    pixelData = stream.read(pixelCount)
    
    colorUse = [0 for i in range(0, 256)]
    for p in pixelData:
        colorUse[p] += 1
    
    colors = []
    for i, use in zip(range(0, biClrUsed * 4, 4), colorUse):
        b, g, r, x = colorTableData[i : i + 4]
        colors.append((r, g, b, x, use))
    
    pixels = []
    pos = 0
    for i in range(0, biHeight):
        row = list(pixelData[pos:pos+biWidth])
        pixels.append(row)
        pos += biWidth
    return biWidth, biHeight, pixels, colors

def pix(p):
    return "%02x" % p

if __name__ == "__main__":
    with open(sys.argv[1], "rb") as f:
        w, h, pixels, colors = loadIndexedBitmap(f)
        print("\n".join([" ".join(map(pix, row)) for row in pixels]))
        print()
        print("\n".join(["%x -> %02x%02x%02x%02x (used %d times)" % (i, r, g, b, x, u) for i, (r, g, b, x, u) in enumerate(colors)]))
