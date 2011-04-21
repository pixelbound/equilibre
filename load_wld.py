import sys
import struct

class WLDData:
    Key = [0x95, 0x3A, 0xC5, 0x2A, 0x95, 0x7A, 0x95, 0x6A]
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
            frag.unpack()
            self.fragments[fragID] = frag
    
    def loadStrings(self, stringData):
        key, keyLen = WLDData.Key, len(WLDData.Key)
        self.strings = bytes(b ^ key[i % keyLen] for i, b in enumerate(stringData))
    
    def lookupString(self, start):
        if self.strings is None:
            return None
        elif (start >= 0) and (start < len(self.strings)):
            end = self.strings.find(b"\0", start)
            if end >= 0:
                return self.strings[start:end].decode("utf-8")
        return None

class Fragment:
    def __init__(self, ID, type, name, data):
        self.ID = ID
        self.type = type
        self.name = name
        self.data = data
    
    @classmethod
    def decode(cls, ID, type, name, data):
        module = sys.modules[__name__]
        className = "Fragment%02x" % type
        if hasattr(module, className):
            classObj = getattr(module, className)
        else:
            classObj = cls
        return classObj(ID, type, name, data)
    
    def unpack(self):
        if not hasattr(self.__class__, "HeaderSize") or not hasattr(self.__class__, "HeaderValues"):
            return
        types = "".join(type for (name, type) in self.__class__.HeaderValues)
        params = struct.unpack(types, self.data[0:self.__class__.HeaderSize])
        for (name, type), value in zip(self.HeaderValues, params):
            setattr(self, name, value)
    
    def __repr__(self):
        return "Fragment(%d, 0x%02x, %s)" % (self.ID, self.type, repr(self.name))

class Fragment36(Fragment):
    HeaderSize = 92
    HeaderValues = [
        ("Flags", "I"), ("Fragment1", "I"), ("Fragment2", "I"), ("Fragment3", "I"), ("Fragment4", "I"),
        ("CenterX", "f"), ("CenterY", "f"), ("CenterZ", "f"), ("Param2_0", "I"), ("Param2_1", "I"), ("Param2_2", "I"),
        ("MaxDist", "f"), ("MinX", "f"), ("MinY", "f"), ("MinZ", "f"), ("MaxX", "f"), ("MaxY", "f"), ("MaxZ", "f"),
        ("VertexCount", "H"), ("TexCoordsCount", "H"), ("NormalCount", "H"), ("ColorCount", "H"), ("PolygonCount", "H"),
        ("VertexPieceCount", "H"), ("PolygonTexCount", "H"), ("VertexTexCount", "H"), ("Size9", "H"), ("Scale", "H")
    ]
    def unpack(self):
        super(Fragment36, self).unpack()
        pos = self.HeaderSize
        end = pos + self.VertexCount * 6
        scale = 1.0 / float(1 << self.Scale)
        self.vertices = self.unpackArray(self.data[pos:end], "hhh", self.VertexCount, 6, self.unpackVertex, scale)
        pos = end + 1
        end = pos + self.TexCoordsCount * 4
        self.texCoords = self.unpackArray(self.data[pos:end], "hh", self.TexCoordsCount, 4)
        pos = end + 1
        end = pos + self.NormalCount * 3
        self.normals = self.unpackArray(self.data[pos:end], "bbb", self.NormalCount, 3, self.unpackNormal)
        pos = end + 1
        end = pos + self.ColorCount * 4
        self.colors = self.unpackArray(self.data[pos:end], "BBBB", self.ColorCount, 4)
        pos = end + 1
        end = pos + self.PolygonCount * 8
        self.polygons = self.unpackArray(self.data[pos:end], "HHHH", self.PolygonCount, 8)
    
    def unpackArray(self, data, pattern, n, size, fun=None, *args):
        array = []
        for i in range(0, n * size, size):
            params = struct.unpack(pattern, data[i:i+size])
            if fun:
                array.append(fun(params, *args))
            else:
                array.append(params)
        return array
    
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
