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

#ifndef EQUILIBRE_STATISTIC_H
#define EQUILIBRE_STATISTIC_H

#include <QString>
#include <QVector>
#include "EQuilibre/Render/Platform.h"

typedef uint32_t gpu_timer_t;

class RENDER_DLL FrameStat
{
public:
    enum Type
    {
        Counter,
        WallTime,
        CPUTime,
        GPUTime
    };

    FrameStat(QString name, int samples, Type type);
    virtual ~FrameStat();

    QString name() const;
    float average() const;
    Type type() const;

    float current() const;
    void setCurrent(float s);
    void beginTime();
    void endTime();
    void clear();
    void next();

private:
    QString m_name;
    QVector<float> m_samples;
    Type m_type;
    bool m_pendingGpuQuery;
    gpu_timer_t m_timer;
    int m_current;
    int m_count;
    double m_startTime;
};

#endif
