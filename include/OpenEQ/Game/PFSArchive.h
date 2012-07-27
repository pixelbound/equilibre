#ifndef OPENEQ_PFS_ARCHIVE_H
#define OPENEQ_PFS_ARCHIVE_H

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QMap>
#include "OpenEQ/Render/Platform.h"

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
class PFSArchive : public QObject
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
