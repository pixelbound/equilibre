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

// System Shock Random Song Generator

#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#include "WindowsMidiOut.h"
#define snprintf _snprintf
#else
#include "LinuxMidiOut.h"
#define MAX_PATH 260
#endif
#include "randgen.h"
#include "MidiOut.h"

int checkheader (unsigned char *data)
{
    int i;
    
    for (i = 0; i < 11; i++)
        if (data[i] != header[i]) return 1;

    return 0;
}

int weighted_random (int num, int usages[32])
{
    int total = 0;
    int i;
    float ends[32];
    int choice;

    if (1)
    {
        for (i = 0; i < 32; i++)
            ends[i] = random(num);

        return (int) ends[(int) random(32)];
    }

    for (i = 0; i < num; i++)
    {
        usages[i]+=1;
        total += usages[i];
    }
        
    total++;
    for (i = 0; i < num; i++)
    {
        usages[i] = total - usages[i];
    }
    total = 0;
    for (i = 0; i < num; i++)
    {
        total += usages[i];
    }

    for (i = 0; i < num; i++)
    {
        ends[i] = (usages[i] * 1000.0F) / total;
    }

    for (i = 1; i < num; i++)
    {
        ends[i] += ends[i-1];
    }
    
    choice = (int) random(1000);

    for (i = 0; i < num; i++)
        if (choice <= ends[i]) return i;
        
    return num-1;
}

int main (int argc, char **argv)
{
    char        denoter[256];
    char        filename[260];
    unsigned char    headdata[11];

    int        maxlen = 1;
    int        seedv = 0;
    int        i = 0, j = 0;
    
    THEME        alldata;

    int        beginning = 0;
    int        transition = 0;
    int        current = 0;
    int        next = 0;
    int        nextamount = 0;
    bool        mt32 = false;
    bool        extract = false;
    bool        extractall = false;
    bool        repeat = false;

    memset (denoter, 0, sizeof(denoter));
    memset (filename, 0, sizeof(filename));
    memset (headdata, 0, sizeof(headdata));

#ifdef _WIN32
    SetConsoleTitle ("System Shock Random Song Generator");
#endif

    printf ("System Shock Random Song Generator by Cless.\n");
    printf ("Version "VERSION_STRING"\n", VERSION_MAJOR, VERSION_MINOR);
    
    if (argc >= 2 &&!strcmp (argv[1], "-create"))
        return rsgcreate (argc-1, argv+1);

    for (i = 1; i < argc; i++)
    {
        if (!strncmp(argv[i], "-width=", 7))
        {
            max_width = atoi(argv[i]+7);
            if (max_width < 1) max_width = 1;
            if (max_width > 128) max_width = 128;
            printf ("Setting Max Width to %i\n", max_width);
        }
        else if (!strcmp(argv[i], "-drum"))
        {
            show_drum = true;
            printf ("Enabling Drum Track Visulization\n");
        }
        else if (!strcmp(argv[i], "-hide"))
        {
            show_notes = false;
            printf ("Disabling Note Visulization\n");
        }
        else if (!strncmp(argv[i], "-visulize=", 10))
        {
            show_notes = true;
            vis_speed = atoi(argv[i]+10);
            if (vis_speed < 1) vis_speed = 1;

            printf ("Enabling Note Visulization\n");
            printf ("Update Speed %i\n", vis_speed);
        }
        else if (!strcmp(argv[i], "-visulize"))
        {
            show_notes = true;
            printf ("Enabling Note Visulization\n");
        }
        else if (!strcmp(argv[i], "-mt32"))
        {
            mt32 = true;
        }
        else if (!strcmp(argv[i], "-extract"))
        {
            extract = true;
        }
        else if (!strcmp(argv[i], "-extractall"))
        {
            printf ("Extracting ALL sequences\n");
            extractall = true;
        }
        else if (!strcmp(argv[i], "-?") || !strcmp(argv[i], "-h") || !strcmp(argv[i], "-help"))
        {
            printf ("Usage:\n"
                "\n"
                "SSPLAYER <options> (filename.rsg) (length (seed_value))\n"
                "or\n"
                "SSPLAYER <options> (filename.mid/rmi)\n"
                "or\n"
                "SSPLAYER <options> (filename.xmi) (sequence_number)\n"
                "\n"
                "Options:\n"
                "\n"
                "-?, -h, -h      - Displays this Information\n"
                "\n"
                                "-width=val      - Sets the visulization width to val. This value should be\n"
                                "                  smaller than the width of the Window. If the value is the\n"
                "                  same or larger than the window width, the visulization will\n"
                                "                  scoll. This defaults to 79.\n"
                "\n"
                                "-visulize=val   - Sets the visulization update value to this count per quarter\n"
                                "                  note. The Default Value is 8 update per quarter note.\n"
                "\n"
                "-drum           - Enable visulization of the drum track.\n"
                                "\n"
                "-hide           - Disable all Visulization.\n"
                "\n"
                "-mt32           - Convert Songs that use Captial MT32 Tones to General MIDI\n"
                "\n"
                "-extract        - Extract the Sequence from the File. This option does not work\n"
                "                  with .RSG files\n"
                "\n"
                "-extractall     - Extract all Sequences from the File. This option does not work\n"
                "                  with .RSG files\n"
                "\n"
                "NOTE that all options must begin with - and must be in lowercase\n"
                "\n"
                                "If the filename, length, seed value or sequence number are not specified you\n"
                                "will be prompted for them.\n"
                );
            return 0;
        }
    }

    // Get rid of all switches
    j = 0;
    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-') j = i;
    }
    
    argv += j;
    argc -= j;

    printf ("%i args\n", argc);

    printf ("Input rsg/xmi/mid/rmi filename: ");
    if (argc >= 2)
    {
        strcpy (filename, argv[1]);
        printf ("%s\n", filename);
    }
    else for (i = 0; ;i++)
    {
        if (i < 255)
        {
            filename[i] = getchar();
            if (filename[i] == '\n')
            {
                filename[i] = '\0';
                break;
            }
        }
        else if (getchar() == '\n')
        {
            filename[255] = '\0';
            break;
        }
    }

    // Check to see if it's an xmidi
    bool is_xmi = false;
    bool is_mid = false;
    bool is_rmi = false;
    
    if ((i = strlen(filename)) > 5)
    {
        i -= 4;
        is_xmi = filename[i] == '.';
        is_mid = filename[i] == '.';
        is_rmi = filename[i] == '.';

        i++;
        is_xmi = is_xmi && (filename[i] == 'X' || filename[i] == 'x');
        is_mid = is_mid && (filename[i] == 'M' || filename[i] == 'm');
        is_rmi = is_rmi && (filename[i] == 'R' || filename[i] == 'r');
        i++;
        is_xmi = is_xmi && (filename[i] == 'M' || filename[i] == 'm');
        is_mid = is_mid && (filename[i] == 'I' || filename[i] == 'i');
        is_rmi = is_rmi && (filename[i] == 'M' || filename[i] == 'm');
        i++;
        is_xmi = is_xmi && (filename[i] == 'I' || filename[i] == 'i');
        is_mid = is_mid && (filename[i] == 'D' || filename[i] == 'd');
        is_rmi = is_rmi && (filename[i] == 'I' || filename[i] == 'i');
    }

    if (!is_xmi && !is_mid && !is_rmi)
    {
        printf ("Reading file %s\n", filename);

        // We want to remove the extension for the denoter

        strcpy (denoter, filename);

        for (i = strlen (denoter)-1; i; i--)
            if (denoter[i] == '.' || denoter[i] == '\\' || denoter[i] == '/')
                break;

        if (denoter[i] == '.') denoter[i] = 0;


        FILE *file = fopen (filename, "rb");
        if (!file)
        {
            printf ("Failed to open file %s\n", filename);
            return 1;
        }
        
        fread (headdata, 1, sizeof (char) * 11, file);
        
        if (checkheader (headdata))
        {
            printf ("File has wrong header! Leaving\n");
            return 1;
        }

        fread (&beginning, 1, sizeof (int), file);
        fread (&transition, 1, sizeof (int), file);
        fread (alldata, 1, sizeof (THEME), file);

        fclose (file);

        printf ("Length of song? ");

        if (argc >= 3)
        {
            maxlen = atoi (argv[2]);
            printf ("%i\n", maxlen);
        }
        else
        {
            scanf ("%i", &maxlen);
        }
        
        printf ("Seed Value? ");
        if (argc >= 4)
        {
            seedv = atoi (argv[3]);
            printf ("%i\n", seedv);
        }
        else
        {
            scanf ("%i", &seedv);
        }

        srand (seedv);
        
        current = beginning;

        sprintf (filename, "%s.xmi", denoter);
    }
    else if (is_mid)
    {
        printf ("Filename is a Standard MIDI File\n");
    }
    else if (is_rmi)
    {
        printf ("Filename is a RIFF MIDI File\n");
    }
    else
    {
        printf ("Filename is a Miles XMIDI File\n");
    }
    
    FILE *xmidifile = fopen(filename, "rb");
    if (!xmidifile)
    {
        printf ("FNF %s\n", filename);
        return 1;
    }
    
    DataSource *temp = new FileDataSource (xmidifile);
    char *data;
    int size;
    size = temp->getSize();
    data = new char[size];
    temp->read(data, size);
    delete temp;
    DataSource *xmids = new BufferDataSource (data, size);
    fclose (xmidifile);
    XMIDI    *xmi = new XMIDI(xmids, mt32?XMIDI_CONVERT_MT32_TO_GS:XMIDI_CONVERT_NOCONVERSION);
    delete xmids;
    delete [] data;

    if (current == 0)
    {
        //repeat = true;
        maxlen = 1;

        // Ask for track number
        while (xmi->number_of_tracks() > 1 && (current <= 0 || current >= xmi->number_of_tracks()) && !extractall)
        {
            printf ("Sequence to %s (max %i)? ", extract?"extract":"play", xmi->number_of_tracks()-1);

            static bool allowed = true;

            if (argc >= 3 && allowed)
            {
                current = atoi (argv[2]);
                printf ("%i\n", current);
            }
            else
            {
                scanf ("%i", &current);
            }        

            if (current >= 0 && current < xmi->number_of_tracks())
            {
                break;
            }

            printf ("Invalid Track Number Specified.\n");
            allowed = false;
        }

        if (extract || extractall)
        {
            int max = current+1;

            if (extractall) max = xmi->number_of_tracks();

            for (; current < max; current++)
            {
                char    fn_out[MAX_PATH];

                strncpy (fn_out, filename, MAX_PATH);
    
                for (i = strlen (fn_out)-1; i; i--)
                    if (fn_out[i] == '.' || fn_out[i] == '\\' || fn_out[i] == '/')
                        break;

                if (fn_out[i] == '.') fn_out[i] = 0;

                snprintf (fn_out+i, MAX_PATH-i, "-%02i.mid", current);

                printf ("Attempting to write sequence %i to '%s'\n", current, fn_out);

                FILE    *fileout = fopen (fn_out, "wb");

                if (!fileout)
                {
                    printf ("Unable to open file\n");
                    delete xmi;
                    return 1;
                }

                DataSource *temp = new FileDataSource (xmidifile);

                xmi->retrieve (current, temp);

                delete temp;
                fclose (fileout);
            }
            printf ("Done\n");
            delete xmi;
            return 0;
        }
    }

    midi_event *events;
    int ppqn;

#ifdef _WIN32
    Windows_MidiOut *player = new Windows_MidiOut();
#else
    LinuxMidiOut *player = new LinuxMidiOut();
#endif
    player->setShowNotes(show_notes);

    int usage[32];
    int nextones[32];
    
    for (i = 0; i < xmi->number_of_tracks(); i++)
    {
        usage[i] = 0;
        xmi->retrieve(i, &events, ppqn);
    }

#ifdef _WIN32
    SetPriorityClass (GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif

    bool is_transition = false;

    for (i = 0; i < maxlen || maxlen == -1; i++)
    {
        if (!xmi->retrieve(current, &events, ppqn))
        {
            printf ("Unable to load part %i\n", current);
            break;
        }

    player->addTrack(events, ppqn, repeat);
        //printf ("%s: %2i  PPQN: %i\n", filename, current, ppqn);

        is_transition = false;

    player->waitState(MidiOut::Playing);

#ifdef _WIN32
        char title[256];
        _snprintf (title, 255, "System Shock Random Song Generator Playing: %s  Part %i/%i (%i)", filename, i+1, maxlen, current);
        SetConsoleTitle (title);
#endif

        usage[current]++;

        //Choose next one
        for (nextamount = 0; nextamount < 32 && alldata[current][nextamount]; nextamount++)
        {
            nextones[nextamount] = usage[alldata[current][nextamount]];
        }
        
        next = weighted_random (nextamount, nextones);
        
        current = alldata[current][next];

        // negetive numbers are transitions
        if (current < 0)
        {
            is_transition = true;
            current = -current;
        }

        // Also randomly add transitions sometimes
        if (rand() < 100) is_transition = true;
    }
    
  player->waitState(MidiOut::Available);
    delete player;
    delete xmi;
    return 0;
}
