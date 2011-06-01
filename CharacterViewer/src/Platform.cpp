#include <QFile>
#include "Platform.h"

QString resolvePath(QString path)
{
    // try to load from resources if the file is not found
    if(QFile::exists(path))
        return path;
    else
        return QString(":/%1").arg(path);
}

char *loadFileData(std::string path)
{
    QString realPath = resolvePath(QString::fromStdString(path));
    QFile f(realPath);
    if(!f.open(QFile::ReadOnly))
    {
        fprintf(stderr, "Could not open file '%s' for reading.\n", realPath.toLatin1().constData());
        return 0;
    }
    QByteArray data = f.readAll();
    char *code = new char[data.length() + 1];
    memcpy(code, data.constData(), data.length());
    code[data.length()] = '\0';
    return code;
}

void freeFileData(char *data)
{
    delete [] data;
}
