#include <string.h>
#include "OpenEQ/Render/FrameStat.h"

FrameStat::FrameStat(QString name, int samples)
{
    m_name = name;
    m_startTime = 0.0;
    for(int i = 0; i < samples; i++)
        m_samples.append(0.0f);
    clear();
}

QString FrameStat::name() const
{
    return m_name;
}

float FrameStat::average() const
{
    float sum = 0.0f;
    if(m_count == 0)
        return sum;
    for(int i = 0; i < m_count; i++)
    {
        int pos = (m_current + i) % m_samples.count();
        sum += m_samples[pos];
    }
    return sum / m_count;
}

void FrameStat::beginTime()
{
    m_startTime = currentTime();
}

void FrameStat::endTime()
{
    double duration = currentTime() - m_startTime;
    setCurrent((float)(duration * 1000.0f));
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
