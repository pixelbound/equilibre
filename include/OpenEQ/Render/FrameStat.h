#ifndef OPENEQ_STATISTIC_H
#define OPENEQ_STATISTIC_H

#include <QString>
#include <QVector>
#include "OpenEQ/Render/Platform.h"

class RENDER_DLL FrameStat
{
public:
    FrameStat(QString name, int samples);

    QString name() const;
    float average() const;

    float current() const;
    void setCurrent(float s);
    void beginTime();
    void endTime();
    void clear();
    void next();

private:
    QString m_name;
    QVector<float> m_samples;
    int m_current;
    int m_count;
    double m_startTime;
};

#endif
