/*
Copyright (C) 2012 PiB <pixelbound@gmail.com>

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

#ifndef RANDGEN_LINUX_MIDIOUT_H
#define RANDGEN_LINUX_MIDIOUT_H

#include <vector>
#include <pthread.h>
#include <inttypes.h>
#include "xmidi.h"
#include "MidiOut.h"

class LinuxMidiOut : public MidiOut
{
public:
    LinuxMidiOut();
    virtual ~LinuxMidiOut();

    virtual void addTrack(midi_event *evntlist, int ppqn, bool repeat);

    virtual PlayerState getState();
    virtual void waitState(PlayerState waitState);
    virtual PlayerState waitAnyState(PlayerState *waitStates, int count);

protected:
    virtual void setState(PlayerState newState);
    virtual NoteData * createNoteData();
    virtual void enqueuePart(mid_data *part);
    virtual bool dequeuePart(mid_data *part);
    virtual void finishPart(mid_data &part);
    virtual void initClock();
    virtual double elapsed();
    virtual void wait(double usec);

private:
    pthread_t thread;
    uint64_t start;

    // Thread communications
    PlayerState state;
    pthread_mutex_t stateLock;
    pthread_cond_t stateCond;
    std::vector<mid_data> partList;
    bool partListClosed;
    pthread_cond_t partListCond;

    // Methods
    static void *threadStart(void *data);
    bool startPlayThread();
    void * threadMain();
};

class LinuxNoteData : public NoteData
{
public:
    LinuxNoteData(LinuxMidiOut *player);
    virtual void clear();
    virtual void play();
    virtual void show(int tempo);
    virtual void handleEvent(midi_event *e);

private:
    LinuxMidiOut *player;
};

#endif // RANDGEN_WIN_MIDIOUT_H
