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

#include <iostream>
#include "xmidi.h"
#include "midiout.h"

using namespace std;

MidiOut::MidiOut()
{
    setState(NotAvailable);
}

MidiOut::~MidiOut()
{
}

void MidiOut::playLoop()
{
    mid_data part;
    while(true)
    {
        setState(Available);
        if(!dequeuePart(&part))
            break;
        setState(Playing);
        playPart(part);
        finishPart(part);
    }
    setState(NotAvailable);
}

void MidiOut::playPart(mid_data &part)
{
    NoteData *nd = createNoteData();

    resetPlayData();
    event = part.list;
    tick = part.tempo * part.ippqn;
    initClock();

    while(1)
    {
        if(event)
        {
            if(playEvent(part, nd))
            {
                // We managed to play the event without having to wait. Move to the next one.
                event = event->next;
            }
        }
        else
        {
            // We played the last event. Repeat the last part or exit.
            if(part.repeat)
            {
                last_tick = 0;
                last_time = 0;
                initClock();
                atEnd(part);
                // Handle note-offs?
                nd->play();
            }
            else
            {
                // XXX handle XMIDI loop break.
                break;
            }
        }

        if(show_notes)
            nd->show(part.tempo);
        endLoopDelay();
    }
    delete nd;
}

void MidiOut::resetPlayData()
{
    tick = 1;
    last_tick = 0;
    last_time = 0;
    aim = 0;
    diff = 0;
    start = 0;
    event = NULL;
    loop_num = -1;
}

bool MidiOut::playEvent(mid_data &current, NoteData *nd)
{
    aim = last_time + (event->time - last_tick) * tick;
    diff = aim - elapsed();

    if(diff > 0)
        return false;

    last_tick = event->time;
    last_time = aim;

    // XMIDI For Loop
    if((event->status >> 4) == MIDI_STATUS_CONTROLLER && event->data[0] == XMIDI_CONTROLLER_FOR_LOOP)
    {
        if(loop_num < XMIDI_MAX_FOR_LOOP_COUNT)
            loop_num++;

        loop_count[loop_num] = event->data[1];
        loop_ticks[loop_num] = event->time;
        loop_event[loop_num] = event->next;

    }    // XMIDI Next/Break
    else if((event->status >> 4) == MIDI_STATUS_CONTROLLER && event->data[0] == XMIDI_CONTROLLER_NEXT_BREAK)
    {
        if(loop_num != -1)
        {
            if(event->data[1] < 64)
            {
                loop_num--;
            }
        }
        event = NULL;

    }    // Tempo Change
    else if(event->status == 0xFF && event->data[0] == 0x51) // Tempo change
    {
        current.tempo = (event->buffer[0] << 16) + (event->buffer[1] << 8) + event->buffer[2];
        tick = current.tempo * current.ippqn;
    }
    else if(event->status < 0xF0)
    {
        nd->handleEvent(event);
    }
    return true;
}

void MidiOut::atEnd(mid_data &mid)
{
    if(loop_num == -1)
    {
        event = mid.list;
    }
    else
    {
        event = loop_event[loop_num];
        last_tick = loop_ticks[loop_num];

        if (loop_count[loop_num])
            if (!--loop_count[loop_num])
                loop_num--;
    }
}

void MidiOut::endLoopDelay()
{
    if(event)
    {
        aim = last_time + (event->time - last_tick) * tick;
        diff = aim - elapsed();
    }
    else
    {
        diff = 1000;
    }

    if(diff >= 1000)
        wait(1000);
    else if(diff >= 200)
        wait(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void mid_data::reset()
{
    ppqn = 1;
    repeat = false;
    tempo = 0x07A120;
    list = NULL;
    ippqn = 1.0;
}

void mid_data::deleteList()
{
    if(list)
    {
        XMIDI::DeleteEventList(list);
        list = NULL;
    }
}
