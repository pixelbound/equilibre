import struct
import io
import zlib
import collections

__all__ = ["S3DArchive"]
ArchiveHeader = collections.namedtuple("ArchiveHeader", ["directoryOffset", "magic", "unknownSize"])
DirectoryEntry = collections.namedtuple("DirectoryEntry", ["crc", "dataOffset", "inflatedSize"])
ArchiveFooter = collections.namedtuple("ArchiveFooter", ["magic", "date"])

class S3DArchive:
    """ Allows  extraction of S3D archives. """
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
        self.header = self._readStruct(self.file, "I4sI", ArchiveHeader)
        if self.header.magic != b"PFS ":
            raise Exception("The file is not a S3D archive")
        directoryEntry, entries = self._readEntries()
        try:
            self.footer = self._readStruct(self.file, "4sI", ArchiveFooter)
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
        entryCount = self._readStruct(self.file, "I")[0]
        directoryEntry, entries = None, []
        for i in range(0, entryCount):
            entry = self._readStruct(self.file, "III", DirectoryEntry)
            if entry.crc == Archive.DIRECTORY_CRC:
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
            deflatedSize, inflatedSize = self._readStruct(self.file, "II")
            deflatedData = self.file.read(deflatedSize)
            inflatedData = zlib.decompress(deflatedData, 15, inflatedSize)
            data[read : read + inflatedSize] = inflatedData
            read += inflatedSize
        return data
    
    def _unpackFileList(self, listData):
        with io.BytesIO(listData) as f:
            fileCount = self._readStruct(f, "I")[0]
            files = []
            for i in range(0, fileCount):
                nameSize = self._readStruct(f, "I")[0]
                name = f.read(nameSize)[0:-1].decode("latin1")
                files.append(name)
            return files
    
    def _readStruct(self, stream, pattern, cls=None):
        size = struct.calcsize(pattern)
        data = stream.read(size)
        if len(data) < size:
            raise EOFError("Tried to read %d bytes but only read %d" % (size, len(data)))
        args = struct.unpack(pattern, data)
        if cls is None:
            return args
        else:
            return cls(*args)
