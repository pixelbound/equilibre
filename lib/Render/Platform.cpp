#include <QFile>
#include "EQuilibre/Render/Platform.h"

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

#ifdef WIN32
#include <windows.h>
double currentTime()
{
    return (double)GetTickCount() * 10e-4;
}
#else
#include <sys/time.h>
double currentTime()
{
    timeval tv;
    gettimeofday(&tv, 0);
    return (double)tv.tv_sec + ((double)tv.tv_usec * 10e-7);
}
#endif
