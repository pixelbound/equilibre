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

#include <windows.h>
#include <winbase.h>
#include <mmsystem.h>
#include <iostream>

#include "randgen.h"
#include "xmidi.h"
#include "WindowsMidiOut.h"

int max_width = 79;
bool show_drum = false;
bool show_notes = true;
int vis_speed = 8;

using namespace std;

CONSOLE_SCREEN_BUFFER_INFO info;
HANDLE out;

Windows_MidiOut::Windows_MidiOut()
{
    InitializeCriticalSection(&stateLock);
    InitializeConditionVariable(&stateCond);
    InitializeConditionVariable(&partListCond);
    partListClosed = false;
}

Windows_MidiOut::~Windows_MidiOut()
{
    // If the thread is alive, notify it that it should terminate.
    bool threadAlive = false;
    EnterCriticalSection(&stateLock);
    if(state != NotAvailable)
    {
        threadAlive = true;
        partListClosed = true;
        WakeAllConditionVariable(&partListCond);
    }
    LeaveCriticalSection(&stateLock);

    if(threadAlive)
    {
        // Wait one second for the thread to terminate.
        WaitForSingleObject(thread_handle, 1000);

        // Kill the thread if it hasn't terminated yet.
        EnterCriticalSection(&stateLock);
        if(state != NotAvailable)
        {
            TerminateThread(thread_handle, 1);
            partList.clear();
            partListClosed = false;
            state = NotAvailable;
        }
        LeaveCriticalSection(&stateLock);
    }
}

HMIDIOUT Windows_MidiOut::midiPort() const
{
    return midi_port;
}

void Windows_MidiOut::waitState(PlayerState waitState)
{
    EnterCriticalSection(&stateLock);
    while(state != waitState)
        SleepConditionVariableCS(&stateCond, &stateLock, INFINITE);
    LeaveCriticalSection(&stateLock);
}

MidiOut::PlayerState Windows_MidiOut::waitAnyState(PlayerState *waitStates, int count)
{
    bool found = false;
    PlayerState newState;
    EnterCriticalSection(&stateLock);
    while(true)
    {
        for(int i = 0; i < count; i++)
        {
            if(state == waitStates[i])
            {
                newState = state;
                found = true;
                break;
            }
        }
        if(found)
            break;
        else
            SleepConditionVariableCS(&stateCond, &stateLock, INFINITE);
    }
    LeaveCriticalSection(&stateLock);
    return newState;
}

MidiOut::PlayerState Windows_MidiOut::getState()
{
    EnterCriticalSection(&stateLock);
    PlayerState currentState = state;
    LeaveCriticalSection(&stateLock);
    return currentState;
}

void Windows_MidiOut::setState(PlayerState newState)
{
    EnterCriticalSection(&stateLock);
    state = newState;
    WakeAllConditionVariable(&stateCond);
    LeaveCriticalSection(&stateLock);
}

bool Windows_MidiOut::dequeuePart(mid_data *part)
{
    bool dequeued = false;
    EnterCriticalSection(&stateLock);
    while((partList.size() == 0) && !partListClosed)
        SleepConditionVariableCS(&partListCond, &stateLock, INFINITE);
    if(!partListClosed)
    {
        *part = partList.front();
        partList.erase(partList.begin());
        dequeued = true;
    }
    LeaveCriticalSection(&stateLock);
    return dequeued;
}

bool Windows_MidiOut::startPlayThread()
{
    bool started = false;
    EnterCriticalSection(&stateLock);
    if(state == NotAvailable)
    {
        started = true;
        state = Starting;
        WakeAllConditionVariable(&stateCond);
    }
    LeaveCriticalSection(&stateLock);

    if(started)
    {
        thread_handle = (HANDLE*)CreateThread(NULL, 0, threadStart, this, 0, &thread_id);

        PlayerState states[] = {Available, InitializationFailed};
        PlayerState newState = waitAnyState(states, 2);
        if (newState == InitializationFailed)
        {
            cerr << "Failier to initialize midi playing thread" << endl;
            setState(NotAvailable);
            return false;
        }
    }
    return true;
}

DWORD __stdcall Windows_MidiOut::threadStart(void *data)
{
    Windows_MidiOut *ptr = static_cast<Windows_MidiOut *>(data);
    return ptr->threadMain();
}

DWORD Windows_MidiOut::threadMain()
{
    out = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(out, &info);

    UINT mmsys_err = midiOutOpen(&midi_port, MIDI_MAPPER, 0, 0, 0);
    if(mmsys_err != MMSYSERR_NOERROR)
    {
        char buf[512];

        mciGetErrorString(mmsys_err, buf, 512);
        cerr << "Unable to open device: " << buf << endl;
        setState(InitializationFailed);
        return 1;
    }

    SetThreadPriority(thread_handle, THREAD_PRIORITY_HIGHEST);
    playLoop();
    midiOutClose(midi_port);
    return 0;
}

void Windows_MidiOut::finishPart(mid_data &part)
{
    part.deleteList();
    midiOutReset(midi_port);
}

void Windows_MidiOut::initClock()
{
    start = GetTickCount();
}

double Windows_MidiOut::elapsed()
{
    return (GetTickCount() - start) * 1000.0;
}

void Windows_MidiOut::wait(double usec)
{
    if(usec >= 0)
        Sleep((int)(usec / 1000.0));
}

void Windows_MidiOut::addTrack(midi_event *evntlist, int ppqn, bool repeat)
{
    if(!startPlayThread())
        return;

    mid_data data;
    data.reset();
    data.list = evntlist;
    data.ppqn = ppqn;
    data.repeat = repeat;
    data.ippqn = 1.0 / ppqn;

    EnterCriticalSection(&stateLock);
    partList.push_back(data);
    WakeAllConditionVariable(&partListCond);
    LeaveCriticalSection(&stateLock);
}

NoteData * Windows_MidiOut::createNoteData()
{
    return new WindowsNoteData(this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

WindowsNoteData::WindowsNoteData(Windows_MidiOut *player)
{
    this->player = player;
    clear();
}

void WindowsNoteData::clear()
{
    memset(notechr, 0, sizeof(notechr));
    memset(notecol, 0, sizeof(notecol));
    memset(first, 0, sizeof(first));
    memset(volume, 64, sizeof(volume));
    memset(pan, 64, sizeof(pan));
    outed = false;
    outnext = GetTickCount() + 50.0F;
}

void WindowsNoteData::play()
{
    HMIDIOUT midi_port = player->midiPort();
    for(int chan = 0; chan < 16; chan++)
    {
        for(int note = 0; note < 128; note++)
        {
            if(notechr[chan][note])
            {
                midiOutShortMsg(midi_port, chan + (MIDI_STATUS_NOTE_ON<<4) + (note << 8));
                notechr[chan][note] = 0;
                notecol[chan][note] = 0;
                first[chan][note] = 0;
            }
        }
    }
}

void WindowsNoteData::show(int tempo)
{
    if(outnext >= GetTickCount())
        return;
    //putchar ('\n');
    outed = false;
    outnext += tempo/(vis_speed*1000.0F);

    if (max_width > 128) max_width = 128;
    for (int i = 0; i < max_width; i++)
    {
        int ch = ' ';
        int co = FOREGROUND_RED|FOREGROUND_BLUE|FOREGROUND_GREEN;
        int chan = -1, pit = -1;
        int was_first = false;

        for (int k = i*128/max_width; k < (i+1)*128/max_width; k++)
        {
            for (int j = 0; j < 16; j++)
            {
                if (was_first) break;
                if (j == 9 && !show_drum) continue;

                if (notechr[j][k])
                {
                    chan = j;
                    pit = k;
                    ch = notechr[chan][pit];
                    co = notecol[chan][pit];
                    was_first = first[chan][pit];
                    outed = true;
                }
            }
        }

        if (chan != -1 && pit != -1)
        {
            //notechr[chan][pit] = 0;
            //notecol[chan][pit] = 0;
            first[chan][pit] = 0;
        }
        SetConsoleTextAttribute (out, co);
        putchar (ch);
        SetConsoleTextAttribute (out, info.wAttributes);
    }
    putchar ('\r');
}

void WindowsNoteData::handleEvent(midi_event *e)
{
    HMIDIOUT midi_port = player->midiPort();
    midiOutShortMsg(midi_port, e->status + (e->data[0] << 8) + (e->data[1] << 16));

    //if ((event->status >> 4) == MIDI_STATUS_NOTE_ON && event->data[1] && (event->status &0xF) != 9)
    //    printf ("Note On:  Channel %2i  Pitch %3i  Velocity %3i\n", (event->status & 0xF)+1, event->data[0], event->data[1]);
    //else if ((event->status >> 4) == MIDI_STATUS_NOTE_OFF || (!event->data[1] && (event->status >> 4) == MIDI_STATUS_NOTE_ON))
    //    printf ("Note Off: Channel %2i  Pitch %3i  Velocity %3i\r", (event->status & 0xF)+1, event->data[0], event->data[1]);

    if ((e->status >> 4) == MIDI_STATUS_CONTROLLER && e->data[0] == 7)
    {
        volume[e->status&0xF] = e->data[1];
    }
    else if ((e->status >> 4) == MIDI_STATUS_CONTROLLER && e->data[0] == 10)
    {
        pan[e->status&0xF] = e->data[1];
    }
    else if ((e->status >> 4) == MIDI_STATUS_NOTE_OFF || ((e->status >> 4) == MIDI_STATUS_NOTE_ON && !e->data[1]))
    {
        notechr[e->status & 0xF][e->data[0]] = 0;
        notecol[e->status & 0xF][e->data[0]] = 0;
        first[e->status & 0xF][e->data[0]] = 0;
    }
    else if ((e->status >> 4) == MIDI_STATUS_NOTE_ON && e->data[1])
    {
        //printf ("Note On:  Channel %2i - ", (event->status & 0xF)+1);

        float fore = (e->data[1] * volume[e->status & 0xF])/127.0F;
        float back = fore;

        int lr = 'C';
        if (pan[e->status & 0xF] < 52)
        {
            lr = 'L';
            back *= pan[e->status & 0xF] / 64.0F;
        }
        else if (pan[e->status & 0xF] > 76)
        {
            lr = 'R';
            back *= (127-pan[e->status & 0xF]) / 64.0F;
        }
        else
        {
            int c = (int) fore;

            if (fore >= 84)
            {
                back = 43;
                c -= 84;
                c /= 11;
            }
            else if (fore >= 43)
            {
                back = 5;
                c -= 43;
                c /= 11;
            }
            else if (fore >= 5)
            {
                back = 0;
                c -= 5;
            }
            else
            {
                back = fore = 0;
                c = 3;
            }

            if (c == 0) lr = 0xB2;
            else if (c == 1) lr = 0xB0;
            else if (c == 2) lr = 0xB1;
            else lr = 0xDB;
        }

        int fore_col;
        if (fore < 5)
            fore_col = 0;
        else if (fore < 43)
            fore_col = FOREGROUND_INTENSITY;
        else if (fore < 84)
            fore_col = FOREGROUND_RED|FOREGROUND_BLUE|FOREGROUND_GREEN;
        else
            fore_col = FOREGROUND_RED|FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_INTENSITY;

        int back_col;
        if (back < 5)
            back_col = 0;
        else if (back < 43)
            back_col = BACKGROUND_INTENSITY;
        else if (back < 84)
            back_col = BACKGROUND_RED|BACKGROUND_BLUE|BACKGROUND_GREEN;
        else
            back_col = BACKGROUND_RED|BACKGROUND_BLUE|BACKGROUND_GREEN|BACKGROUND_INTENSITY;

        notechr[e->status & 0xF][e->data[0]] = lr;
        notecol[e->status & 0xF][e->data[0]] = fore_col|back_col;
        first[e->status & 0xF][e->data[0]] = 1;
    }
}
