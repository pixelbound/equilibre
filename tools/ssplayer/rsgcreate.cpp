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

/// System Shock Random Song Generator Datafile generator

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "randgen.h"

int rsgcreate (int argc, char **argv)
{
	FILE		*output;
	char		denoter[256];
	char		filename[260];
	THEME		alldata;
	int		beginning;
	int		transition;
	int		lineinput;
	char		i,j;
	FILE		*input;

	printf ("System Shock Random Song Generator RSG dile creator\n\n");
	
	memset (alldata, 0, sizeof (alldata));
	if (argc == 1)
	{
		printf ("Grabbing input from stdin\n");
		input = stdin;
	}
	else
	{
		input = fopen (argv[1], "r");
		if (!input)
		{
			printf ("Unable to open file specified on command line for input.\nGrabbing input from stdin!\n");
			input = stdin;
		}
		else
		{
			printf ("Grabbing input from %s\n", argv[1]);
		}
	}
	
	printf ("Imput file denoter: ");
	fscanf (input, "%255s", denoter);
	sprintf (filename, "%s.rsg", denoter);
	printf ("Outputting to file %s\n", filename);

	output = fopen (filename, "wb");
	if (!output)
	{
		printf ("Failed to open file %s\n", filename);
		return 1;
	}

	printf ("Input start number: ");
	fscanf (input, "%i", &beginning);
	
	printf ("Input Transition number: ");
	fscanf (input, "%i", &transition);

	alldata[0][0] = 0;
	
	for (i = 1; i < 32; i++)
	{
		for (j = 0; j < 32; j++)
		{
			printf ("Input link %i (32 max) for part %i (0 finalizes -128 quits): ", j+1, i);
			fscanf (input, "%i", &lineinput);
			
			if (lineinput == -128)
			{
				printf ("Leaving!\n");
				alldata [i][j] = 0;
				break;
			}
			
			alldata [i][j] = lineinput;
			if (!lineinput) break;
		}
		
		if (alldata[i][0])
		{
			printf ("Links for part %i are:", i);
			for (j = 0; j < 32 && alldata[i][j]; j++) printf (" %i", alldata[i][j]);
			printf ("\n");
		}

		if (lineinput == -128) break;
	}
	
	printf ("Writing File\n");
	
	fwrite (header, 1, sizeof (char) * 11, output);
	fwrite (&beginning, 1, sizeof (int), output);
	fwrite (&transition, 1, sizeof (int), output);
	fwrite (alldata, 1, sizeof (THEME), output);

	fclose (output);
	return 0;
}
