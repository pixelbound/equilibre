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

#include <iostream>
#include <unistd.h>
#include <signal.h>

#include "xmidi.h"
#include "LinuxMidiOut.h"

using namespace std;

LinuxMidiOut::LinuxMidiOut()
{
    pthread_mutex_init(&stateLock, NULL);
    pthread_cond_init(&stateCond, NULL);
    pthread_cond_init(&partListCond, NULL);
    partListClosed = false;
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
    start = 0;
}

double LinuxMidiOut::elapsed()
{
    return 0.0;
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
    this->player = player;
    clear();
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

void LinuxNoteData::handleEvent(midi_event *e)
{
}
