#ifndef OPENEQ_STREAM_READER_H
#define OPENEQ_STREAM_READER_H

#include <QObject>
#include "OpenEQ/Render/Platform.h"

class QIODevice;

class GAME_DLL StreamReader
{
public:
    StreamReader(QIODevice *stream);

    QIODevice *stream() const;

    virtual bool unpackField(char type, void *field);
    bool unpackFields(const char *types, ...);
    bool unpackStruct(const char *types, void *first);
    bool unpackArray(const char *types, uint32_t count, void *first);
    /*!
      \def Return the in-memory size of the structure.
      */
    uint32_t structSize(const char *types) const;
    bool readString(uint32_t size, QString *dest);

protected:
    virtual uint32_t fieldSize(char c) const;
    bool readUint8(uint8_t *dest);
    bool readInt8(int8_t *dest);
    bool readUint16(uint16_t *dest);
    bool readInt16(int16_t *dest);
    bool readUint32(uint32_t *dest);
    bool readInt32(int32_t *dest);
    bool readFloat32(float *dest);
    bool readRaw(char *dest, size_t n);

    QIODevice *m_stream;
};

#endif
