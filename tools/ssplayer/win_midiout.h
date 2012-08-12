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

#include "xmidi.h"
#include <windows.h>
#include <mmsystem.h>
#include <vector>

class note_data
{
public:
	note_data();
	void clear();
	void play(HMIDIOUT midi_port);
	void show(int tempo);
	void handle_event(midi_event *e);

private:
	unsigned char volume[16];
	unsigned char pan[16];
    unsigned char notechr[16][127];
    int notecol[16][127];
    int first[16][127];
	bool outed;
	double outnext;
};

struct mid_data
{
	midi_event	*list;
	int 		ppqn;
	BOOL		repeat;
	int		    tempo;
	double		ippqn;

	void reset();
	void deleteList();
};

struct play_data
{
	double tick;
	double last_tick;
	double last_time;
	double aim;
	double diff;
	
	midi_event *event;

	// Xmidi Looping
	midi_event *loop_event[XMIDI_MAX_FOR_LOOP_COUNT];
	int loop_count[XMIDI_MAX_FOR_LOOP_COUNT];
	int loop_ticks[XMIDI_MAX_FOR_LOOP_COUNT];
	int loop_num;

	void reset();
};

class	Windows_MidiOut
{
public:
	enum PlayerState
	{
		NotAvailable = 0,
		Starting,
		InitializationFailed,
		Available,
		Playing,
		FinishedPlaying
	};

	// Do we accept events, YES!
	virtual BOOL accepts_events(void) { return TRUE; }

	virtual void start_track(midi_event *evntlist, int ppqn, BOOL repeat);
	virtual void add_track(midi_event *evntlist, int ppqn, BOOL repeat);
	virtual void stop_track(void);
	virtual PlayerState get_state();
	virtual void wait_state(PlayerState waitState);
	virtual PlayerState wait_any_state(PlayerState *waitStates, int count);
	virtual const char *copyright(void);

	Windows_MidiOut();
	virtual ~Windows_MidiOut();

private:
	HMIDIOUT	midi_port;
	
	HANDLE	 	*thread_handle;
	DWORD		thread_id;

	// Thread communications
	LONG		thread_com;
	PlayerState state;
	CRITICAL_SECTION stateLock;
	CONDITION_VARIABLE stateCond;
	std::vector<mid_data> midList;
	bool midListClosed;
	CONDITION_VARIABLE midCond;

	mid_data *thread_data;

	mid_data data;

	// Methods
	static DWORD __stdcall thread_start(void *data);
	bool start_play_thread();
	void set_state(PlayerState newState);
	bool dequeue_mid(mid_data *data);
	DWORD thread_main();
	void thread_play();
	bool play_event(play_data &pd, mid_data &current, note_data &nd);

	// Microsecond Clock
	unsigned int start;
	unsigned int sfx_start;

	inline void wmoInitClock ()
	{ start = GetTickCount(); }

	inline double wmoGetTime ()
	{ return (GetTickCount() - start) * 1000.0; }

	inline void wmoInitSFXClock ()
	{ sfx_start = GetTickCount(); }

	inline double wmoGetSFXTime ()
	{ return (GetTickCount() - sfx_start) * 1000.0; }

	inline void wmoDelay (const double mcs_delay)
	{ if (mcs_delay >= 0) Sleep ((int) (mcs_delay / 1000.0)); }

};

#endif // RANDGEN_WIN_MIDIOUT_H
