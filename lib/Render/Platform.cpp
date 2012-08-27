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
