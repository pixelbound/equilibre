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
                print("Loaded fragment %d, type %x, name %s" % (i, fragmentID, fragmentName))
                fragmentData = f.read(fragmentSize - 4)
                wld.addFragment(Fragment(i, fragmentID, fragmentName, fragmentData))
        return wld
    
    def addFragment(self, frag):
        if not frag.ID in self.fragments:
            self.fragments[frag.ID] = frag
    
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
    def __init__(self, ID, type, namePtr, data):
        self.ID = ID
        self.type = type
        self.namePtr = namePtr
        self.data = data

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage: %s <WLD file>" % sys.argv[0])
    else:
        wld = WLDData.fromFile(sys.argv[1])
        print("%d fragments, %d loaded" % (wld.fragmentCount, len(wld.fragments)))
