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

#define W32MO_THREAD_COM_READY		0
#define W32MO_THREAD_COM_PLAY		1
#define W32MO_THREAD_COM_STOP		2
#define W32MO_THREAD_COM_INIT		3
#define W32MO_THREAD_COM_INIT_FAILED	4
#define W32MO_THREAD_COM_PLAY_NEXT	5
#define W32MO_THREAD_COM_EXIT		-1

int	max_width = 79;
bool	show_drum = false;
bool	show_notes = true;
int	vis_speed= 8;

using namespace std;

CONSOLE_SCREEN_BUFFER_INFO info;
HANDLE out;

Windows_MidiOut::Windows_MidiOut()
{
	InitializeCriticalSection(&stateLock);
	InitializeConditionVariable(&stateCond);
	set_state(NotAvailable);
	InterlockedExchange (&playing, FALSE);
	start_play_thread();
}

Windows_MidiOut::~Windows_MidiOut()
{
	PlayerState currentState = get_state();
	if(currentState == NotAvailable)
		return;

	while (thread_com != W32MO_THREAD_COM_READY) Sleep (1);
	
	InterlockedExchange (&thread_com, W32MO_THREAD_COM_EXIT);

	int count = 0;
	
	while (count < 100)
	{
		DWORD code;
		GetExitCodeThread (thread_handle, &code);
		
		// Wait 1 MS before trying again
		if (code == STILL_ACTIVE) Sleep (10);
		else break;
		
		count++;
	}

	// We waited a second and it still didn't terminate
	currentState = get_state();
	if (count == 100 && (currentState != NotAvailable))
		TerminateThread (thread_handle, 1);
	
	set_state(NotAvailable);
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

void Windows_MidiOut::start_play_thread()
{
	BOOL started = false;
	EnterCriticalSection(&stateLock);
	if(state == PlayerState::NotAvailable)
	{
		started = true;
		state = PlayerState::Starting;
		WakeAllConditionVariable(&stateCond);
	}
	LeaveCriticalSection(&stateLock);

	if(started)
	{
		thread_handle = (HANDLE*) CreateThread (NULL, 0, thread_start, this, 0, &thread_id);
		
		PlayerState states[] = {Available, InitializationFailed};
		PlayerState newState = wait_any_state(states, 2);
		if (newState == InitializationFailed)
		{
			cerr << "Failier to initialize midi playing thread" << endl;
			set_state(NotAvailable);
		}
	}
}

DWORD __stdcall Windows_MidiOut::thread_start(void *data)
{
	Windows_MidiOut *ptr=static_cast<Windows_MidiOut *>(data);
	return ptr->thread_main();
}

DWORD Windows_MidiOut::thread_main()
{
	thread_data = NULL;
	InterlockedExchange (&playing, FALSE);

	UINT mmsys_err = midiOutOpen (&midi_port, MIDI_MAPPER, 0, 0, 0);

	out = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo (out, &info);

	if (mmsys_err != MMSYSERR_NOERROR)
	{
		char buf[512];

		mciGetErrorString(mmsys_err, buf, 512);
		cerr << "Unable to open device: " << buf << endl;
		set_state(InitializationFailed);
		return 1;
	}
	set_state(Available);
	
	SetThreadPriority (thread_handle, THREAD_PRIORITY_HIGHEST);
	
	InterlockedExchange (&thread_com, W32MO_THREAD_COM_READY);

	thread_play();

	midiOutClose (midi_port);
	
	set_state(NotAvailable);
	return 0;
}

void Windows_MidiOut::thread_play ()
{
	int	ppqn = 1;
	int	repeat = FALSE;
	int	tempo = 0x07A120;
	double	Ippqn = 1;
	double	tick = 1;
	double	last_tick = 0;
	double	last_time = 0;
	double	aim = 0;
	double	diff = 0;
	
	midi_event *evntlist = NULL;
	midi_event *event = NULL;

	// Xmidi Looping
	midi_event	*loop_event[XMIDI_MAX_FOR_LOOP_COUNT];
	int		loop_count[XMIDI_MAX_FOR_LOOP_COUNT];
	int		loop_ticks[XMIDI_MAX_FOR_LOOP_COUNT];
	int		loop_num = -1;

	int		ppqn_next = 1;
	int		repeat_next = FALSE;
	int		tempo_next = 0x07A120;
	double		Ippqn_next = 1;
	midi_event	*evntlist_next = NULL;

	unsigned char	volume[16];
	unsigned char	pan[16];
	unsigned char	notechr[16][127];
	int		notecol[16][127];
	int		first[16][127];

	memset (volume, 64, sizeof (volume));
	memset (pan, 64, sizeof (pan));

	memset (notechr, 0, sizeof (notechr));
	memset (notecol, 0, sizeof (notecol));
	memset (first, 0, sizeof (first));
	double	outnext = GetTickCount() + 50.0F;
	bool	outed = false;

	// Play while there isn't a message waiting
	while (1)
	{
		if (thread_com == W32MO_THREAD_COM_EXIT && !playing) break;
		
		while (event)
		{
	 		aim = last_time + (event->time-last_tick)*tick;
			diff = aim - wmoGetTime ();

			if (diff > 0) break;

			last_tick = event->time;
			last_time = aim;
		
				// XMIDI For Loop
			if ((event->status >> 4) == MIDI_STATUS_CONTROLLER && event->data[0] == XMIDI_CONTROLLER_FOR_LOOP)
			{
				if (loop_num < XMIDI_MAX_FOR_LOOP_COUNT) loop_num++;

				loop_count[loop_num] = event->data[1];
				loop_ticks[loop_num] = event->time;
				loop_event[loop_num] = event->next;

			}	// XMIDI Next/Break
			else if ((event->status >> 4) == MIDI_STATUS_CONTROLLER && event->data[0] == XMIDI_CONTROLLER_NEXT_BREAK)
			{
				if (loop_num != -1)
				{
					if (event->data[1] < 64)
					{
						loop_num--;
					}
				}
				event = NULL;

			}	// Tempo Change
			else if (event->status == 0xFF && event->data[0] == 0x51) // Tempo change
			{
				tempo = (event->buffer[0] << 16) +
					(event->buffer[1] << 8) +
					event->buffer[2];
					
				tick = tempo*Ippqn;
			}	
			else if (event->status < 0xF0)
			{
				midiOutShortMsg (midi_port, event->status + (event->data[0] << 8) + (event->data[1] << 16));

				//if ((event->status >> 4) == MIDI_STATUS_NOTE_ON && event->data[1] && (event->status &0xF) != 9)
				//	printf ("Note On:  Channel %2i  Pitch %3i  Velocity %3i\n", (event->status & 0xF)+1, event->data[0], event->data[1]);
				//else if ((event->status >> 4) == MIDI_STATUS_NOTE_OFF || (!event->data[1] && (event->status >> 4) == MIDI_STATUS_NOTE_ON))
				//	printf ("Note Off: Channel %2i  Pitch %3i  Velocity %3i\r", (event->status & 0xF)+1, event->data[0], event->data[1]);

				if ((event->status >> 4) == MIDI_STATUS_CONTROLLER && event->data[0] == 7)
				{
					volume[event->status&0xF] = event->data[1];
				}
				else if ((event->status >> 4) == MIDI_STATUS_CONTROLLER && event->data[0] == 10)
				{
					pan[event->status&0xF] = event->data[1];
				}
				else if ((event->status >> 4) == MIDI_STATUS_NOTE_OFF || ((event->status >> 4) == MIDI_STATUS_NOTE_ON && !event->data[1]))
				{
					notechr[event->status & 0xF][event->data[0]] = 0;
					notecol[event->status & 0xF][event->data[0]] = 0;
					first[event->status & 0xF][event->data[0]] = 0;
				}
				else if ((event->status >> 4) == MIDI_STATUS_NOTE_ON && event->data[1])	
				{
					//printf ("Note On:  Channel %2i - ", (event->status & 0xF)+1);

					float fore = (event->data[1] * volume[event->status & 0xF])/127.0F;
					float back = fore;

					int lr = 'C';
					if (pan[event->status & 0xF] < 52)
					{
						lr = 'L';
						back *= pan[event->status & 0xF] / 64.0F;
					}
					else if (pan[event->status & 0xF] > 76)
					{
						lr = 'R';
						back *= (127-pan[event->status & 0xF]) / 64.0F;
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

					notechr[event->status & 0xF][event->data[0]] = lr;
					notecol[event->status & 0xF][event->data[0]] = fore_col|back_col;
					first[event->status & 0xF][event->data[0]] = 1;
				}


			}
		
		 	if (event) event = event->next;
	
	 		if (!event || (thread_com != W32MO_THREAD_COM_READY && thread_com != W32MO_THREAD_COM_PLAY_NEXT && thread_com != W32MO_THREAD_COM_PLAY))
		 	{
				bool clean = !repeat || (thread_com != W32MO_THREAD_COM_READY);

		 		if (clean || evntlist_next)
		 		{
		 			// Clean up
					//midiOutReset (midi_port);
					if (evntlist) XMIDI::DeleteEventList (evntlist);
					evntlist = NULL;
					event = NULL;
					if (!evntlist_next)
					{
						InterlockedExchange (&playing, FALSE);
						set_state(PlayerState::FinishedPlaying);
					}
					// If stop was requested, we are ready to receive another song
					if (!evntlist_next && thread_com == W32MO_THREAD_COM_STOP)
						InterlockedExchange (&thread_com, W32MO_THREAD_COM_READY);

					loop_num = -1;
		 		}

				wmoInitClock ();
				last_tick = 0;
				last_time = 0;

				if (evntlist_next)
				{
					ppqn = ppqn_next;
					repeat = repeat_next;
					tempo = tempo_next;
					Ippqn = Ippqn_next;
					event = evntlist = evntlist_next;
					evntlist_next = NULL;

					// Reset XMIDI Looping
					loop_num = -1;
				}
				else if (evntlist)
				{
	 				if (loop_num == -1) event = evntlist;
					else
					{
						event = loop_event[loop_num];
						last_tick = loop_ticks[loop_num];

						if (loop_count[loop_num])
							if (!--loop_count[loop_num])
								loop_num--;
					}
				}
				for (int chan = 0; chan < 16; chan++)
				for (int note = 0; note < 128; note++)
				if (notechr[chan][note])
				{
					midiOutShortMsg (midi_port, chan + (MIDI_STATUS_NOTE_ON<<4) + (note << 8));
					notechr[chan][note] = 0;
					notecol[chan][note] = 0;
					first[chan][note] = 0;
				}
		 	}
		}

		if (show_notes && outnext < GetTickCount())
		{
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

		// Got issued a music play command
		// set up the music playing routine
		while (thread_com == W32MO_THREAD_COM_PLAY || (thread_com == W32MO_THREAD_COM_PLAY_NEXT && !playing))
		{
			if (evntlist)
			{
				midiOutReset (midi_port);
				XMIDI::DeleteEventList (evntlist);
				evntlist = NULL;
				event = NULL;
				//InterlockedExchange (&playing, FALSE);
			}

			if (evntlist_next) 
			{
				XMIDI::DeleteEventList (evntlist_next);
				evntlist_next = NULL;
			}
			
			// Make sure that the data exists
			if (!thread_data) break;
			
			evntlist = thread_data->list;
			repeat = thread_data->repeat;

			ppqn = thread_data->ppqn;
			set_state(PlayerState::Playing);
			InterlockedExchange (&playing, TRUE);
			InterlockedExchange ((LONG*) &thread_data, (LONG) NULL);
			InterlockedExchange (&thread_com, W32MO_THREAD_COM_READY);
			
			event = evntlist;
			tempo = 0x07A120;
			
			Ippqn = 1.0/ppqn;
			tick = tempo*Ippqn;

			last_tick = 0;
			last_time = 0;
			wmoInitClock ();

			// Reset XMIDI Looping
			loop_num = -1;

			break;
		}

		// Got issued a music play command
		// set up the music playing routine
		while (thread_com == W32MO_THREAD_COM_PLAY_NEXT)
		{	
			//printf ("add %i %i\n", playing, thread_com);
			if (evntlist_next)
			{
				XMIDI::DeleteEventList (evntlist_next);
				evntlist_next = NULL;
			}
			
			// Make sure that the data exists
			if (!thread_data) break;
			
			evntlist_next = thread_data->list;
			repeat_next = thread_data->repeat;

			ppqn_next = thread_data->ppqn;
			InterlockedExchange ((LONG*) &thread_data, (LONG) NULL);
			InterlockedExchange (&thread_com, W32MO_THREAD_COM_READY);
			
			tempo_next = 0x07A120;
			Ippqn_next = 1.0/ppqn;

			break;
		}

	 	if (event)
	 	{
	 		aim = last_time + (event->time-last_tick)*tick;
			diff = aim - wmoGetTime ();
	 	}
	 	else 
	 		diff = 1000;

		if (diff >= 1000) wmoDelay (1000);
		else if (diff >= 200) wmoDelay (0);
	}
	midiOutReset (midi_port);
}

void Windows_MidiOut::start_track (midi_event *evntlist, const int ppqn, BOOL repeat)
{
	start_play_thread();

	if (get_state() == NotAvailable)
		return;

	while (thread_com != W32MO_THREAD_COM_READY) Sleep (1);
	
	data.list = evntlist;
	data.ppqn = ppqn;
	data.repeat = repeat;
	
	InterlockedExchange ((LONG*) &thread_data, (LONG) &data);
	InterlockedExchange (&thread_com, W32MO_THREAD_COM_PLAY);

	while (thread_com != W32MO_THREAD_COM_READY) Sleep (1);
}

void Windows_MidiOut::add_track (midi_event *evntlist, const int ppqn, BOOL repeat)
{
	start_play_thread();

	if (get_state() == NotAvailable)
		return;

	while (thread_com != W32MO_THREAD_COM_READY) Sleep (1);
	
	data.list = evntlist;
	data.ppqn = ppqn;
	data.repeat = repeat;
	
	InterlockedExchange ((LONG*) &thread_data, (LONG) &data);
	InterlockedExchange (&thread_com, W32MO_THREAD_COM_PLAY_NEXT);

	while (thread_com != W32MO_THREAD_COM_READY) Sleep (1);
}

void Windows_MidiOut::stop_track(void)
{
	PlayerState currentState = get_state();
	if(currentState != Playing);
		return;

	while (thread_com != W32MO_THREAD_COM_READY) Sleep (1);
	InterlockedExchange (&thread_com, W32MO_THREAD_COM_STOP);
}

BOOL Windows_MidiOut::is_playing(void)
{
	return playing;
}

const char *Windows_MidiOut::copyright(void)
{
	return "Internal Win32 Midiout Midi Player for Exult.";
}

