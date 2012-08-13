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

#ifndef RANDGEN_WIN_MIDIOUT_H
#define RANDGEN_WIN_MIDIOUT_H

#include <vector>
#include <windows.h>
#include <mmsystem.h>
#include "xmidi.h"
#include "midiout.h"

class Windows_MidiOut
{
public:
    Windows_MidiOut();
    virtual ~Windows_MidiOut();

    HMIDIOUT midiPort() const;

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

private:
    HMIDIOUT midi_port;

    HANDLE *thread_handle;
    DWORD thread_id;

    // Thread communications
    PlayerState state;
    CRITICAL_SECTION stateLock;
    CONDITION_VARIABLE stateCond;
    std::vector<mid_data> partList;
    bool partListClosed;
    CONDITION_VARIABLE partListCond;

    // Methods
    static DWORD __stdcall threadStart(void *data);
    bool startPlayThread();
    DWORD threadMain();
};

class WindowsNoteData : public NoteData
{
public:
    WindowsNoteData(Windows_MidiOut *player);
    virtual void clear();
    virtual void play();
    virtual void show(int tempo);
    virtual void handle_event(midi_event *e);

private:
    Windows_MidiOut *player;
    unsigned char volume[16];
    unsigned char pan[16];
    unsigned char notechr[16][127];
    int notecol[16][127];
    int first[16][127];
    bool outed;
    double outnext;
};

#endif // RANDGEN_WIN_MIDIOUT_H
