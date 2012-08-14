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

#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <iostream>
#include "xmidi.h"
#include "LinuxMidiOut.h"

using namespace std;

LinuxMidiOut::LinuxMidiOut()
{
    pthread_mutex_init(&stateLock, NULL);
    pthread_cond_init(&stateCond, NULL);
    pthread_cond_init(&partListCond, NULL);
    partListClosed = false;
    setState(NotAvailable);
}

LinuxMidiOut::~LinuxMidiOut()
{
    // If the thread is alive, notify it that it should terminate.
    bool threadAlive = false;
    pthread_mutex_lock(&stateLock);
    if(state != NotAvailable)
    {
        threadAlive = true;
        partListClosed = true;
        pthread_cond_broadcast(&partListCond);
    }
    pthread_mutex_unlock(&stateLock);

    if(threadAlive)
    {
        // Wait one second for the thread to terminate.
        usleep(1000*1000);

        // Kill the thread if it hasn't terminated yet.
        pthread_mutex_lock(&stateLock);
        if(state != NotAvailable)
        {
            pthread_kill(thread, SIGTERM);
            partList.clear();
            partListClosed = false;
            state = NotAvailable;
        }
        pthread_mutex_unlock(&stateLock);
    }
}

std::string LinuxMidiOut::fontPath() const
{
    return m_fontPath;
}

void LinuxMidiOut::setFontPath(std::string path)
{
    m_fontPath = path;
}

void LinuxMidiOut::waitState(PlayerState waitState)
{
    pthread_mutex_lock(&stateLock);
    while(state != waitState)
        pthread_cond_wait(&stateCond, &stateLock);
    pthread_mutex_unlock(&stateLock);
}

MidiOut::PlayerState LinuxMidiOut::waitAnyState(PlayerState *waitStates, int count)
{
    bool found = false;
    PlayerState newState;
    pthread_mutex_lock(&stateLock);
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
            pthread_cond_wait(&stateCond, &stateLock);
    }
    pthread_mutex_unlock(&stateLock);
    return newState;
}

MidiOut::PlayerState LinuxMidiOut::getState()
{
    pthread_mutex_lock(&stateLock);
    PlayerState currentState = state;
    pthread_mutex_unlock(&stateLock);
    return currentState;
}

void LinuxMidiOut::setState(PlayerState newState)
{
    pthread_mutex_lock(&stateLock);
    state = newState;
    pthread_cond_broadcast(&stateCond);
    pthread_mutex_unlock(&stateLock);
}

void LinuxMidiOut::enqueuePart(mid_data *part)
{
	pthread_mutex_lock(&stateLock);
    partList.push_back(*part);
    pthread_cond_broadcast(&partListCond);
    pthread_mutex_unlock(&stateLock);
}

bool LinuxMidiOut::dequeuePart(mid_data *part)
{
    bool dequeued = false;
    pthread_mutex_lock(&stateLock);
    while((partList.size() == 0) && !partListClosed)
        pthread_cond_wait(&partListCond, &stateLock);
    if(!partListClosed)
    {
        *part = partList.front();
        partList.erase(partList.begin());
        dequeued = true;
    }
    pthread_mutex_unlock(&stateLock);
    return dequeued;
}

bool LinuxMidiOut::startPlayThread()
{
    bool started = false;
    pthread_mutex_lock(&stateLock);
    if(state == NotAvailable)
    {
        started = true;
        state = Starting;
        pthread_cond_broadcast(&stateCond);
    }
    pthread_mutex_unlock(&stateLock);

    if(started)
    {
        pthread_create(&thread, NULL, threadStart, (void *)this);

        PlayerState states[] = {Available, InitializationFailed};
        PlayerState newState = waitAnyState(states, 2);
        if(newState == InitializationFailed)
        {
            cerr << "Failier to initialize midi playing thread" << endl;
            setState(NotAvailable);
            return false;
        }
    }
    return true;
}

void * LinuxMidiOut::threadStart(void *data)
{
    LinuxMidiOut *ptr = static_cast<LinuxMidiOut *>(data);
    return ptr->threadMain();
}

void * LinuxMidiOut::threadMain()
{
    playLoop();
    return NULL;
}

void LinuxMidiOut::finishPart(mid_data &part)
{
    part.deleteList();
}

void LinuxMidiOut::initClock()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    start = ((uint64_t)tv.tv_sec * 1000 * 1000) + tv.tv_usec;
}

double LinuxMidiOut::elapsed()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t now = ((uint64_t)tv.tv_sec * 1000 * 1000) + tv.tv_usec;
    return (double)(now - start);
}

void LinuxMidiOut::wait(double usec)
{
    uint32_t dur = (uint32_t)usec;
    if(dur >= 0)
        usleep(dur);
}

void LinuxMidiOut::addTrack(midi_event *evntlist, int ppqn, bool repeat)
{
    if(startPlayThread())
		MidiOut::addTrack(evntlist, ppqn, repeat);
}

NoteData * LinuxMidiOut::createNoteData()
{
    return new LinuxNoteData(this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

LinuxNoteData::LinuxNoteData(LinuxMidiOut *player)
{
    m_player = player;
    m_synthSettings = new_fluid_settings();
    m_synth = new_fluid_synth(m_synthSettings);
    fluid_settings_setstr(m_synthSettings, "audio.driver", "pulseaudio");
    m_driver = new_fluid_audio_driver(m_synthSettings, m_synth);
    if(player->fontPath().size() > 0)
        m_fontID = fluid_synth_sfload(m_synth, player->fontPath().c_str(), 0);
}

LinuxNoteData::~LinuxNoteData()
{
    delete_fluid_audio_driver(m_driver);
    delete_fluid_synth(m_synth);
    delete_fluid_settings(m_synthSettings);
}

void LinuxNoteData::clear()
{
}

void LinuxNoteData::play()
{
}

void LinuxNoteData::show(int tempo)
{
}

static const char * MIDIStatusName(int status)
{
    switch(status)
    {
    case MIDI_STATUS_NOTE_OFF:
        return "NOTE_OFF";
    case MIDI_STATUS_NOTE_ON:
        return "NOTE_ON";
    case MIDI_STATUS_AFTERTOUCH:
        return "AFTERTOUCH";
    case MIDI_STATUS_CONTROLLER:
        return "CONTROLLER";
    case MIDI_STATUS_PROG_CHANGE:
        return "PROG_CHANGE";
    case MIDI_STATUS_PRESSURE:
        return "PRESSURE";
    case MIDI_STATUS_PITCH_WHEEL:
        return "PITCH_WHEEL";
    case MIDI_STATUS_SYSEX:
        return "SYSEX";
    default:
        return "UNKNOWN";
    }
}

static void dumpEvent(midi_event *e, bool verbose)
{
    int status = e->status >> 4;
    int chan = e->status & 0xF;
    if(status == MIDI_STATUS_NOTE_ON && e->data[1])
    {
        int note = e->data[0];
        int vel = e->data[1];
        if(verbose)
            printf("NOTE_ON (chan = %x, note = %x, vel = %x)\n", chan, note, vel);
    }
    else if(status == MIDI_STATUS_NOTE_OFF || (status == MIDI_STATUS_NOTE_ON && !e->data[1]))
    {
        int note = e->data[0];
        if(verbose)
            printf("NOTE_OFF (chan = %x, note = %x)\n", chan, note);
    }
    else if(status == MIDI_STATUS_CONTROLLER)
    {
        int ctrl = e->data[0];
        int val = e->data[1];
        switch(ctrl)
        {
        case 0:
            printf("BANK SELECT (chan = %x, patch = %x)\n", chan, ctrl | (val < 7));
            break;
        case 7:
            printf("VOLUME (chan = %x, val = %x)\n", chan, val);
            break;
        case 8:
            printf("BALANCE (chan = %x, val = %x)\n", chan, val);
            break;
        case 10:
            printf("PAN (chan = %x, val = %x)\n", chan, val);
            break;
        case 64:
            printf("PEDAL %s (chan = %x)\n", (val & 0x7f) ? "ON" : "OFF", chan);
            break;
        case 91:
            printf("REVERB (chan = %x, val = %x)\n", chan, val);
            break;
        case 93:
            printf("CHORUS (chan = %x, val = %x)\n", chan, val);
            break;
        case 120:
            printf("ALL SOUNDS OFF (chan = %x)\n", chan);
            break;
        default:
            printf("CONTROLLER (chan = %x, ctrl = %x, val = %x)\n", chan, ctrl, val);
            break;
        }
    }
    else if(status == MIDI_STATUS_PROG_CHANGE)
    {
        int prog = e->data[0];
        printf("PROG_CHANGE (chan = %x, prog = %x)\n", chan, prog);
    }
    else if(status == MIDI_STATUS_PRESSURE)
    {
        int pressure = e->data[0];
        printf("CHAN_PRESSURE (chan = %x, val = %x)\n", chan, pressure);
    }
    else if(status == MIDI_STATUS_PITCH_WHEEL)
    {
        int val = e->data[0] | (e->data[1] << 7);
        printf("PITCH_WHEEL (chan = %x, val = %x)\n", chan, val);
    }
    else
    {
        const char *statusName = MIDIStatusName(status);
        printf("%s\n", statusName);
    }
}

void LinuxNoteData::handleEvent(midi_event *e)
{
    int status = e->status >> 4;
    int chan = e->status & 0xF;
    if(status == MIDI_STATUS_NOTE_ON && (e->data[1] > 0))
    {
        int note = e->data[0];
        int vel = e->data[1];
        fluid_synth_noteon(m_synth, chan, note, vel);
    }
    else if(status == MIDI_STATUS_NOTE_OFF || (status == MIDI_STATUS_NOTE_ON && (e->data[1] == 0)))
    {
        int note = e->data[0];
        fluid_synth_noteoff(m_synth, chan, note);
    }
    else if(status == MIDI_STATUS_AFTERTOUCH)
    {
        int note = e->data[0];
        int pressure = e->data[1];
        assert(0);
    }
    else if(status == MIDI_STATUS_CONTROLLER)
    {
        int ctrl = e->data[0];
        int val = e->data[1];
        fluid_synth_cc(m_synth, chan, ctrl, val);
    }
    else if(status == MIDI_STATUS_PROG_CHANGE)
    {
        int prog = e->data[0];
        fluid_synth_program_change(m_synth, chan, prog);
    }
    else if(status == MIDI_STATUS_PRESSURE)
    {
        int pressure = e->data[0];
        fluid_synth_channel_pressure(m_synth, chan, pressure);
    }
    else if(status == MIDI_STATUS_PITCH_WHEEL)
    {
        fluid_synth_pitch_bend(m_synth, chan, e->data[0] | (e->data[1] << 7));
    }
    else
    {
        printf("Unknown MIDI status %x\n", status);
    }
    dumpEvent(e, false);
}
