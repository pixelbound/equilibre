import sys
import struct

Key = [0x95, 0x3A, 0xC5, 0x2A, 0x95, 0x7A, 0x95, 0x6A]
def decodeString(stringData):
    return bytes(b ^ Key[i % len(Key)] for i, b in enumerate(stringData))

class WLDData:
    def __init__(self, magic, version, fragmentCount, header3, header4, stringHashSize, header6):
        self.magic = magic
        self.version = version
        self.fragmentCount = fragmentCount
        self.header3 = header3
        self.header4 = header4
        self.stringHashSize = stringHashSize
        self.header6 = header6
        self.strings = None
        self.fragments = {}

    @classmethod 
    def fromFile(cls, path):
        with open(path, "rb") as f:
            headerSize = 4 * 7
            header = f.read(headerSize)
            wld = cls(*struct.unpack("IIIIIII", header))
            stringData = f.read(wld.stringHashSize)
            wld.loadStrings(stringData)
            for i in range(0, wld.fragmentCount + 1):
                fragmentHeaderSize = 4 * 3
                fragmentHeader = f.read(fragmentHeaderSize)
                if len(fragmentHeader) < fragmentHeaderSize:
                    break
                fragmentSize, fragmentID, fragmentNamePtr = struct.unpack("IIi", fragmentHeader)
                if fragmentNamePtr < 0:
                    fragmentName = wld.lookupString(-fragmentNamePtr)
                else:
                    fragmentName = None
                fragmentData = f.read(fragmentSize - 4)
                wld.addFragment(i, fragmentID, fragmentName, fragmentData)
        return wld
    
    def addFragment(self, fragID, type, name, data):
        if not fragID in self.fragments:
            frag = Fragment.decode(fragID, type, name, data)
            frag.unpack(self)
            self.fragments[fragID] = frag
    
    def loadStrings(self, stringData):
        self.strings = decodeString(stringData)
    
    def lookupString(self, start):
        if self.strings is None:
            return None
        elif (start >= 0) and (start < len(self.strings)):
            end = self.strings.find(b"\0", start)
            if end >= 0:
                return self.strings[start:end].decode("utf-8")
        return None
    
    def lookupReference(self, ref):
        if ref < 0:
            # reference by name
            return self.lookupString(-ref - 1)
        elif (ref > 0) and (ref <= len(self.fragments)):
            # reference by index
            return self.fragments[ref - 1]
        else:
            return None
    
    def fragmentsByType(self, type):
        return filter(lambda f: f.type == type, self.fragments.values())

class Fragment:
    def __init__(self, ID, type, name, data):
        self.ID = ID
        self.type = type
        self.name = name
        self.data = data
        self.pos = 0
    
    @classmethod
    def decode(cls, ID, type, name, data):
        module = sys.modules[__name__]
        className = "Fragment%02x" % type
        if hasattr(module, className):
            classObj = getattr(module, className)
        else:
            classObj = cls
        return classObj(ID, type, name, data)
    
    def unpack(self, wld):
        pass
    
    def unpackReference(self, wld, name="Reference"):
        pattern = "i"
        size = struct.calcsize(pattern)
        value = struct.unpack(pattern, self.data[self.pos: self.pos + size])[0]
        setattr(self, name, wld.lookupReference(value))
        self.pos += size
    
    def unpackFields(self, fields):
        patterns = "".join(pattern for (name, pattern) in fields)
        size = struct.calcsize(patterns)
        values = struct.unpack(patterns, self.data[self.pos: self.pos + size])
        for (name, pattern), value in zip(fields, values):
            setattr(self, name, value)
        self.pos += size
    
    def unpackField(self, name, pattern):
        size = struct.calcsize(pattern)
        value = struct.unpack(pattern, self.data[self.pos: self.pos + size])[0]
        setattr(self, name, value)
        self.pos += size
    
    def unpackArray(self, name, pattern, n, fun=None, *args):
        size = struct.calcsize(pattern)
        array = []
        for i in range(0, n * size, size):
            params = struct.unpack(pattern, self.data[self.pos + i : self.pos + i + size])
            if fun:
                array.append(fun(params, *args))
            else:
                array.append(params)
        setattr(self, name, array)
        self.pos += n * size
    
    def __repr__(self):
        return "Fragment(%d, 0x%02x, %s)" % (self.ID, self.type, repr(self.name))

class Fragment03(Fragment):
    """ This type of fragment contains the name of a texture file. """
    def unpack(self, wld):
        self.unpackField("Flags", "I")
        self.unpackField("_NameLength", "H")
        nameData = self.data[self.pos: self.pos + self._NameLength]
        self.FileName = decodeString(nameData)[0:-1].decode("utf-8")
        self.pos += self._NameLength

class Fragment04(Fragment):
    """ This type of fragment describes a list of texture file names. """
    def unpack(self, wld):
        self.unpackField("Flags", "I")
        self.unpackField("Size1", "I")
        if self.Flags & 0b100:
            self.unpackField("Param1", "I")
        if self.Flags & 0b1000:
            self.unpackField("Param2", "I")
        self.unpackArray("Files", "i", self.Size1, lambda params: params[0])
        self.Files = [wld.lookupReference(f) for f in self.Files]

class Fragment05(Fragment):
    """ This type of fragment refers to a 04 fragment. """
    def unpack(self, wld):
        self.unpackReference(wld)
        self.unpackField("Flags", "I")

class Fragment21(Fragment):
    """ This type of fragment describes a BSP tree for a zone divided into regions. """
    def unpack(self, wld):
        self.unpackField("Size1", "I")
        self.unpackArray("nodes", "ffffiII", self.Size1)

class Fragment22(Fragment):
    """ This type of fragment describes the node of a BSP tree. """
    HeaderValues = [
        ("Flags", "I"), ("Fragment1", "i"), ("Size1", "I"), ("Size2", "I"), ("Params1", "I"),
        ("Size3", "I"), ("Size4", "I"), ("Param2", "I"), ("Size5", "I"), ("Size6", "I")
    ]
    
    def unpack(self, wld):
        self.unpackFields(Fragment22.HeaderValues)
        self.unpackArray("Data1", "B" * 12, self.Size1)
        self.unpackArray("Data2", "B" * 8, self.Size2)
        #TODO Data3
        #TODO Data4
        self.unpackArray("Data5", "I" * 7, self.Size5)
        self.NearbyRegions = []
        for i in range(0, self.Size6):
            self.unpackField("_NearbyRegionSize", "H")
            regionData = self.data[self.pos: self.pos + self._NearbyRegionSize]
            self.NearbyRegions.append(self.decodeRegionList(regionData))
            self.pos += self._NearbyRegionSize
    
    def decodeRegionList(self, data):
        RID = 0
        pos = 0
        regions = set()
        while pos < len(data):
            b = data[pos]
            if b < 0x3f:
                RID += b
                pos += 1
            elif b == 0x3f:
                lo = data[pos + 1]
                hi = data[pos + 2]
                skip = ((hi << 8) + lo)
                RID += skip
                pos += 3
            elif b < 0x80:
                skip = (b & 0b111000) >> 3
                mark = (b & 0b111)
                RID += skip
                for i in range(0, mark):
                    regions.add(RID + i)
                RID += mark
                pos += 1
            elif b < 0xc0:
                mark = (b & 0b111000) >> 3
                skip = (b & 0b111)
                for i in range(0, mark):
                    regions.add(RID + i)
                RID += mark
                RID += skip
                pos += 1
            elif b < 0xff:
                mark = b - 0xc0
                for i in range(0, mark):
                    regions.add(RID + i)
                RID += mark
                pos += 1
            elif b == 0xff:
                lo = data[pos + 1]
                hi = data[pos + 2]
                mark = ((hi << 8) + lo)
                for i in range(0, mark):
                    regions.add(RID + i)
                RID += mark
                pos += 3
            else:
                break
        return regions

class Fragment29(Fragment):
    """ This type of fragment describes the properties of a list of regions. """
    def unpack(self, wld):
        self.unpackField("Flags", "I")
        self.unpackField("Size1", "I")
        self.unpackArray("regions", "I", self.Size1)
        self.unpackField("Size2", "I")
        self.unpackArray("Data2", "B", self.Size2)

class Fragment30(Fragment):
    """ This type of fragment describes a texture, and refers to a 05 fragment. """
    def unpack(self, wld):
        #self.unpackReference(wld)
        self.unpackField("Flags", "I")
        self.unpackField("Param1", "I")
        self.unpackField("Param2", "I")
        self.unpackField("Param3_0", "f")
        self.unpackField("Param3_1", "f")
        #if self.Flags & 0b1:
        self.unpackReference(wld)
        self.unpackField("Param4", "f")
            
class Fragment31(Fragment):
    """ This type of fragment describes a list of textures. """
    def unpack(self, wld):
        self.unpackField("Flags", "I")
        self.unpackField("Size1", "I")
        self.unpackArray("Textures", "i", self.Size1, lambda params: params[0])
        self.Textures = [wld.lookupReference(f) for f in self.Textures]

class Fragment36(Fragment):
    """ This type of fragment describes a mesh. """
    HeaderValues = [
        #("Flags", "I"), ("Fragment1", "I"), ("Fragment2", "I"), ("Fragment3", "I"), ("Fragment4", "I"),
        ("CenterX", "f"), ("CenterY", "f"), ("CenterZ", "f"), ("Param2_0", "I"), ("Param2_1", "I"), ("Param2_2", "I"),
        ("MaxDist", "f"), ("MinX", "f"), ("MinY", "f"), ("MinZ", "f"), ("MaxX", "f"), ("MaxY", "f"), ("MaxZ", "f"),
        ("VertexCount", "H"), ("TexCoordsCount", "H"), ("NormalCount", "H"), ("ColorCount", "H"), ("PolygonCount", "H"),
        ("VertexPieceCount", "H"), ("PolygonTexCount", "H"), ("VertexTexCount", "H"), ("Size9", "H"), ("Scale", "H")
    ]
    
    def unpack(self, wld):
        self.unpackField("Flags", "I")
        for i in range(1, 5):
            self.unpackReference(wld, "Fragment%d" % i)
        self.unpackFields(Fragment36.HeaderValues)
        scale = 1.0 / float(1 << self.Scale)
        self.unpackArray("vertices", "hhh", self.VertexCount, self.unpackVertex, scale)
        self.unpackArray("texCoords", "hh", self.TexCoordsCount)
        self.unpackArray("normals", "bbb", self.NormalCount, self.unpackNormal)
        self.unpackArray("colors", "BBBB", self.ColorCount)
        self.unpackArray("polygons", "HHHH", self.PolygonCount)
        self.unpackArray("vertexPieces", "HH", self.VertexPieceCount)
        self.unpackArray("polygonsByTex", "HH", self.PolygonTexCount)
        self.unpackArray("verticesByTex", "HH", self.VertexTexCount)
    
    def unpackVertex(self, params, scale):
        return ((params[0] * scale) + self.CenterX,
                (params[1] * scale) + self.CenterY,
                (params[2] * scale) + self.CenterZ)
    
    def unpackNormal(self, params):
        return tuple([float(p) / 127.0 for p in params])

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage: %s <WLD file>" % sys.argv[0])
    else:
        wld = WLDData.fromFile(sys.argv[1])
        print("%d fragments, %d loaded" % (wld.fragmentCount, len(wld.fragments)))
