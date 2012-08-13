/*
Copyright (C) 2000, 2001  Ryan Nunn

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

// Midi Player

#ifndef RANDGEN_MIDIOUT_H
#define RANDGEN_MIDIOUT_H

#include "xmidi.h"

class NoteData
{
public:
    virtual void clear() = 0;
    virtual void play() = 0;
    virtual void show(int tempo) = 0;
    virtual void handleEvent(midi_event *e) = 0;
};

struct mid_data
{
    midi_event *list;
    int ppqn;
    bool repeat;
    int tempo;
    double ippqn;

    void reset();
    void deleteList();
};

class MidiOut
{
public:
    enum PlayerState
    {
        NotAvailable = 0,
        Starting,
        InitializationFailed,
        Available,
        Playing
    };

    MidiOut();
    virtual ~MidiOut();

    virtual void addTrack(midi_event *evntlist, int ppqn, bool repeat) = 0;
    virtual PlayerState getState() = 0;
    virtual void waitState(PlayerState waitState) = 0;
    virtual PlayerState waitAnyState(PlayerState *waitStates, int count) = 0;

protected:
    virtual void setState(PlayerState newState) = 0;
    virtual void enqueuePart(mid_data *part) = 0;
    virtual bool dequeuePart(mid_data *part) = 0;
    virtual void finishPart(mid_data &part) = 0;
    virtual NoteData * createNoteData() = 0;
    virtual void playLoop();
    virtual void playPart(mid_data &part);
    virtual void initClock() = 0;
    virtual double elapsed() = 0;
    virtual void wait(double usec) = 0;

private:
    double tick;
    double last_tick;
    double last_time;
    double aim;
    double diff;
    midi_event *event;
    midi_event *loop_event[XMIDI_MAX_FOR_LOOP_COUNT];
    int loop_count[XMIDI_MAX_FOR_LOOP_COUNT];
    int loop_ticks[XMIDI_MAX_FOR_LOOP_COUNT];
    int loop_num;

    void resetPlayData();
    void atEnd(mid_data &mid);
    void endLoopDelay();
    bool playEvent(mid_data &current, NoteData *nd);
};

#endif // RANDGEN_WIN_MIDIOUT_H
