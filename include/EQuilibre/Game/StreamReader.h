// Copyright (C) 2012 PiB <pixelbound@gmail.com>
//  
// EQuilibre is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifndef EQUILIBRE_STREAM_READER_H
#define EQUILIBRE_STREAM_READER_H

#include <QObject>
#include "EQuilibre/Render/Platform.h"

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
