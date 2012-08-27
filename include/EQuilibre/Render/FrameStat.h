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
