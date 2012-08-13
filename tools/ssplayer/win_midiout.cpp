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
#include "win_midiout.h"

int	max_width = 79;
bool show_drum = false;
bool show_notes = true;
int	vis_speed = 8;

using namespace std;

CONSOLE_SCREEN_BUFFER_INFO info;
HANDLE out;

Windows_MidiOut::Windows_MidiOut()
{
	InitializeCriticalSection(&stateLock);
	InitializeConditionVariable(&stateCond);
	InitializeConditionVariable(&partListCond);
	partListClosed = false;
	set_state(NotAvailable);
	start_play_thread();
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

void Windows_MidiOut::wait_state(PlayerState waitState)
{
	EnterCriticalSection(&stateLock);
	while(state != waitState)
		SleepConditionVariableCS(&stateCond, &stateLock, INFINITE);
	LeaveCriticalSection(&stateLock);
}

Windows_MidiOut::PlayerState Windows_MidiOut::wait_any_state(PlayerState *waitStates, int count)
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

Windows_MidiOut::PlayerState Windows_MidiOut::get_state()
{
	EnterCriticalSection(&stateLock);
	PlayerState currentState = state;
	LeaveCriticalSection(&stateLock);
	return currentState;
}

void Windows_MidiOut::set_state(PlayerState newState)
{
	EnterCriticalSection(&stateLock);
	state = newState;
	WakeAllConditionVariable(&stateCond);
	LeaveCriticalSection(&stateLock);
}

bool Windows_MidiOut::dequeue_part(mid_data *part)
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

bool Windows_MidiOut::start_play_thread()
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
		thread_handle = (HANDLE*) CreateThread(NULL, 0, thread_start, this, 0, &thread_id);
		
		PlayerState states[] = {Available, InitializationFailed};
		PlayerState newState = wait_any_state(states, 2);
		if (newState == InitializationFailed)
		{
			cerr << "Failier to initialize midi playing thread" << endl;
			set_state(NotAvailable);
			return false;
		}
	}
	return true;
}

DWORD __stdcall Windows_MidiOut::thread_start(void *data)
{
	Windows_MidiOut *ptr = static_cast<Windows_MidiOut *>(data);
	return ptr->thread_main();
}

DWORD Windows_MidiOut::thread_main()
{
	out = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(out, &info);

	UINT mmsys_err = midiOutOpen(&midi_port, MIDI_MAPPER, 0, 0, 0);
	if(mmsys_err != MMSYSERR_NOERROR)
	{
		char buf[512];

		mciGetErrorString(mmsys_err, buf, 512);
		cerr << "Unable to open device: " << buf << endl;
		set_state(InitializationFailed);
		return 1;
	}

	set_state(Available);
	SetThreadPriority(thread_handle, THREAD_PRIORITY_HIGHEST);
	thread_play();
	midiOutClose(midi_port);
	set_state(NotAvailable);
	return 0;
}

void Windows_MidiOut::thread_play()
{
	mid_data part;
	while(true)
	{
		set_state(Available);
		if(!dequeue_part(&part))
			break;
		set_state(Playing);
		play_part(part);
		part.deleteList();
		midiOutReset(midi_port);
	}
}

void Windows_MidiOut::play_part(mid_data &part)
{
	play_data pd;
	note_data nd;

	pd.reset();
	pd.event = part.list;
	pd.tick = part.tempo * part.ippqn;
	pd.wmoInitClock();
	pd.loop_num = -1;

	while(1)
	{
		while(pd.event)
		{
			if(!pd.play_event(midi_port, part, nd))
			{
				// It's too early to play this event. Wait a bit before re-trying.
				break;
			}
			pd.event = pd.event->next;
		}

		// We played the last event. Repeat the last part or exit.
	 	if(!pd.event)
		{
			if(part.repeat)
			{
				pd.wmoInitClock();
				pd.at_end(part);
				// Handle note-offs?
				nd.play(midi_port);
			}
			else
			{
				return;
			}
		}

		if(show_notes)
			nd.show(part.tempo);
		pd.end_loop_delay();
	}
}

void Windows_MidiOut::add_track(midi_event *evntlist, int ppqn, bool repeat)
{
	if(!start_play_thread())
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

///////////////////////////////////////////////////////////////////////////////////////////////////

note_data::note_data()
{
	clear();
}

void note_data::clear()
{
	memset(notechr, 0, sizeof(notechr));
	memset(notecol, 0, sizeof(notecol));
	memset(first, 0, sizeof(first));
	memset(volume, 64, sizeof(volume));
	memset(pan, 64, sizeof(pan));
	outed = false;
	outnext = GetTickCount() + 50.0F;
}

void note_data::play(HMIDIOUT midi_port)
{
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

void note_data::show(int tempo)
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

void note_data::handle_event(midi_event *e)
{
	//if ((event->status >> 4) == MIDI_STATUS_NOTE_ON && event->data[1] && (event->status &0xF) != 9)
	//	printf ("Note On:  Channel %2i  Pitch %3i  Velocity %3i\n", (event->status & 0xF)+1, event->data[0], event->data[1]);
	//else if ((event->status >> 4) == MIDI_STATUS_NOTE_OFF || (!event->data[1] && (event->status >> 4) == MIDI_STATUS_NOTE_ON))
	//	printf ("Note Off: Channel %2i  Pitch %3i  Velocity %3i\r", (event->status & 0xF)+1, event->data[0], event->data[1]);

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

///////////////////////////////////////////////////////////////////////////////////////////////////

void play_data::reset()
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

bool play_data::play_event(HMIDIOUT midi_port, mid_data &current, note_data &nd)
{
	aim = last_time + (event->time - last_tick) * tick;
	diff = aim - wmoGetTime();

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

	}	// XMIDI Next/Break
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

	}	// Tempo Change
	else if(event->status == 0xFF && event->data[0] == 0x51) // Tempo change
	{
		current.tempo = (event->buffer[0] << 16) + (event->buffer[1] << 8) + event->buffer[2];
		tick = current.tempo * current.ippqn;
	}	
	else if(event->status < 0xF0)
	{
		midiOutShortMsg(midi_port, event->status + (event->data[0] << 8) + (event->data[1] << 16));
		nd.handle_event(event);
	}
	return true;
}

void play_data::at_end(mid_data &mid)
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

void play_data::end_loop_delay()
{
	if(event)
	{
	 	aim = last_time + (event->time - last_tick) * tick;
		diff = aim - wmoGetTime();
	}
	else 
	{
	 	diff = 1000;
	}

	if(diff >= 1000)
		wmoDelay(1000);
	else if(diff >= 200)
		wmoDelay(0);
}

void play_data::wmoInitClock()
{
	last_tick = 0;		
	last_time = 0;
	start = GetTickCount();
}

double play_data::wmoGetTime()
{
	return (GetTickCount() - start) * 1000.0;
}

void play_data::wmoDelay(double mcs_delay)
{
	if(mcs_delay >= 0)
		Sleep((int)(mcs_delay / 1000.0));
}
