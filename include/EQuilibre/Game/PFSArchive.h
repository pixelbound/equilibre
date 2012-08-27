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

#ifndef EQUILIBRE_PFS_ARCHIVE_H
#define EQUILIBRE_PFS_ARCHIVE_H

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QMap>
#include "EQuilibre/Render/Platform.h"

class QFile;
class StreamReader;

class PFSEntry
{
public:
    uint32_t crc;
    uint32_t dataOffset;
    uint32_t inflatedSize;
};

/*!
  \brief Allows extraction of PFS archives (e.g. .pfs, .s3d, .pak files).
  */
class GAME_DLL PFSArchive : public QObject
{
public:
    PFSArchive(QString path, QObject *parent = 0);
    virtual ~PFSArchive();
    bool isOpen() const;
    void close();

    const QList<QString> & files() const;

    QByteArray unpackFile(QString name);

private:
    void openArchive(QString path);
    bool readEntries(PFSEntry &dir, QList<PFSEntry> &entries);
    QByteArray unpackFileEntry(PFSEntry e);
    bool unpackFileList(StreamReader *sr, QList<QString> &names);

    static const uint32_t DIRECTORY_CRC = 0x61580AC9;

    QFile *m_file;
    StreamReader *m_reader;
    QList<QString> m_fileNames;
    QMap<QString, PFSEntry> m_entries;
};

#endif
