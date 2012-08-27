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

#include <string.h>
#include <time.h>
#include <GL/glew.h>
#include "EQuilibre/Render/FrameStat.h"

FrameStat::FrameStat(QString name, int samples, FrameStat::Type type)
{
    m_name = name;
    m_startTime = 0.0;
    for(int i = 0; i < samples; i++)
        m_samples.append(0.0f);
    m_timer = 0;
    if(type == GPUTime)
    {
        if(GLEW_EXT_timer_query)
            glGenQueries(1, &m_timer);
        m_type = (m_timer > 0) ? GPUTime : CPUTime;
    }
    else
    {
        m_type = type;
    }
    m_pendingGpuQuery = false;
    clear();
}

FrameStat::~FrameStat()
{
    if(m_type == GPUTime)
        glDeleteQueries(1, &m_timer);
}

QString FrameStat::name() const
{
    return m_name;
}

FrameStat::Type FrameStat::type() const
{
    return m_type;
}

float FrameStat::average() const
{
    float sum = 0.0f;
    if(m_count < 2)
        return sum;
    for(int i = 1; i < m_count; i++)
    {
        int pos = (m_current + i) % m_samples.count();
        sum += m_samples[pos];
    }
    return sum / (m_count - 1);
}

void FrameStat::beginTime()
{
    if(m_type == GPUTime)
    {
        if(!m_pendingGpuQuery)
        {
            glBeginQuery(GL_TIME_ELAPSED, m_timer);
            m_pendingGpuQuery = true;
        }
    }
    else if(m_type == CPUTime)
    {
        m_startTime = (double)clock() / CLOCKS_PER_SEC;
    }
    else if(m_type == WallTime)
    {
        m_startTime = currentTime();
    }
}

void FrameStat::endTime()
{
    if(m_type == GPUTime)
    {
        if(m_pendingGpuQuery)
            glEndQuery(GL_TIME_ELAPSED);
    }
    else if(m_type == CPUTime)
    {
        double current = (double)clock() / CLOCKS_PER_SEC;
        double duration = current - m_startTime;
        setCurrent((float)(duration * 1000.0f));
    }
    else if(m_type == WallTime)
    {
        double duration = currentTime() - m_startTime;
        setCurrent((float)(duration * 1000.0f));
    }
}

float FrameStat::current() const
{
    return m_samples[m_current];
}

void FrameStat::setCurrent(float s)
{
    m_samples[m_current] = s;
}

void FrameStat::next()
{
    if(m_type == GPUTime && m_pendingGpuQuery)
    {
        uint64_t elapsedNs = 0;
        glGetQueryObjectui64vEXT(m_timer, GL_QUERY_RESULT, &elapsedNs);
        double elapsedMs = (double)elapsedNs / 1000000.0;
        setCurrent((float)elapsedMs);
        m_pendingGpuQuery = false;
    }

    // Move towards the beginning of the buffer, wrapping to the end when we get there.
    m_current = (m_current > 0) ? (m_current - 1) : (m_samples.count() - 1);
    m_count = qMin(m_count + 1, m_samples.count());
    setCurrent(0.0f);
}

void FrameStat::clear()
{
    m_current = m_samples.count() - 1;
    m_count = 0;
    for(int i = m_current; i >= 0; i--)
        m_samples[i] = 0.0f;
}
