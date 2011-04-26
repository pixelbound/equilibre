import struct
import io
import zlib
import collections
import sys
import os

__all__ = ["S3DArchive", "readStruct"]
ArchiveHeader = collections.namedtuple("ArchiveHeader", ["directoryOffset", "magic", "unknownSize"])
DirectoryEntry = collections.namedtuple("DirectoryEntry", ["crc", "dataOffset", "inflatedSize"])
ArchiveFooter = collections.namedtuple("ArchiveFooter", ["magic", "date"])

def readStruct(stream, pattern, cls=None):
    size = struct.calcsize(pattern)
    data = stream.read(size)
    if len(data) < size:
        raise EOFError("Tried to read %d bytes but only read %d" % (size, len(data)))
    args = struct.unpack(pattern, data)
    if cls is None:
        return args
    else:
        return cls(*args)

class S3DArchive:
    """ Allows extraction of S3D archives. """
    DIRECTORY_CRC = 0x61580AC9
    
    def __init__(self, path):
        self.path = path
        self.file = None
        self.header = None
        self.footer = None
        self.entries = {}
        self.files = []
        self._openArchive()
    
    def _openArchive(self):
        self.file = open(self.path, "rb")
        # read the file header, entry list and optional file footer
        self.header = readStruct(self.file, "I4sI", ArchiveHeader)
        if self.header.magic != b"PFS ":
            raise Exception("The file is not a S3D archive")
        directoryEntry, entries = self._readEntries()
        try:
            self.footer = readStruct(self.file, "4sI", ArchiveFooter)
        except Exception:
            self.footer = None
        # extract the file name list and map each file name to an entry
        if directoryEntry is not None:
            listData = self._unpackFileEntry(directoryEntry)
            self.files = self._unpackFileList(listData)
            entries.sort(key=lambda e: e.dataOffset)
            self.entries = dict(zip(self.files, entries))
    
    def unpackFile(self, name):
        return self._unpackFileEntry(self.entries.get(name))
    
    def openFile(self, name):
        data = self._unpackFileEntry(self.entries.get(name))
        if data is None:
            raise Exception("The file '%s' was not found in the archive" % name)
        return io.BytesIO(data)
    
    def close(self):
        if self.file:
            self.file.close()
            self.file = None
    
    def __enter__(self):
        return self
    
    def __exit__(self, e, val, tb):
        self.close()
    
    def _readEntries(self):
        self.file.seek(self.header.directoryOffset)
        entryCount = readStruct(self.file, "I")[0]
        directoryEntry, entries = None, []
        for i in range(0, entryCount):
            entry = readStruct(self.file, "III", DirectoryEntry)
            if entry.crc == S3DArchive.DIRECTORY_CRC:
                directoryEntry = entry
            else:
                entries.append(entry)
        return directoryEntry, entries
    
    def _unpackFileEntry(self, entry):
        if entry is None:
            return None
        self.file.seek(entry.dataOffset)
        read, data = 0, bytearray(entry.inflatedSize)
        while read < len(data):
            deflatedSize, inflatedSize = readStruct(self.file, "II")
            deflatedData = self.file.read(deflatedSize)
            inflatedData = zlib.decompress(deflatedData, 15, inflatedSize)
            data[read : read + inflatedSize] = inflatedData
            read += inflatedSize
        return data
    
    def _unpackFileList(self, listData):
        with io.BytesIO(listData) as f:
            fileCount = readStruct(f, "I")[0]
            files = []
            for i in range(0, fileCount):
                nameSize = readStruct(f, "I")[0]
                name = f.read(nameSize)[0:-1].decode("latin1")
                files.append(name)
            return files

if __name__ == "__main__":
    def listFiles(path):
        with S3DArchive(path) as a:
            for fileName in a.files:
                print(fileName)
    
    def extractFiles(path, destPath):
        os.makedirs(destPath, exist_ok=True)
        with S3DArchive(path) as a:
            for fileName in a.files:
                filePath = os.path.join(destPath, fileName)
                with open(filePath, "wb") as f:
                    f.write(a.unpackFile(fileName))
                
    if len(sys.argv) < 2:
        print("usage: %s <.S3D file>" % sys.argv[0])
        print("       [option] <.S3D file>")
    elif len(sys.argv) == 2:
        listFiles(sys.argv[1])
    else:
        mode = sys.argv[1]
        path = sys.argv[2]
        if mode.find("t") >= 0:
            listFiles(path)
        elif mode.find("x") >= 0:
            extractFiles(path, os.path.join(os.getcwd(), "%s.d" % os.path.basename(path)))
