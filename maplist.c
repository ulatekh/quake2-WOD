#include "g_local.h"



cvar_t *maplist;
cvar_t *maplistfile;


// Returns true if it has stored the name of
// the next map to run in level.nextmap .

qboolean MaplistNext (void)
{
	long i, offset;
	FILE *in;
	char *res_cp;
	char buffer[MAX_QPATH + 1];

	// Make sure we can find the game directory.
	if (!gamedir || !gamedir->string[0])
	{
		gi.dprintf ("No maplist -- can't find gamedir\n");
		return false;
	}

	// Make sure we can find the maplist file.
	if (!maplistfile || !maplistfile->string[0])
	{
		gi.dprintf ("No maplist -- can't find maplistfile\n");
		return false;
	}

	// Get the offset in the maplist.txt file.  Zero means maplist is turned
	// off.
	offset = (int)(maplist->value);
	if (offset <= 0)
		return false;

	// Open the maplist file.
openmaplist:
	sprintf (buffer, "./%s/%s", gamedir->string, maplistfile->string);
	in = fopen (buffer, "r");
	if (in == NULL)
	{
		gi.dprintf ("No maplist -- can't open ./%s/%s\n",
			gamedir->string, maplistfile->string);
		return false;
	}

	// Skip as many entries as we need to.  (This is a stupid and inefficient way
	// to do it, but ftell() keeps returning -555, so I can't do it that way.)
	for (i = 0; i < offset; i++)
	{
		res_cp = fgets (buffer, sizeof (buffer), in);
		if (res_cp == NULL)
		{
			// End-of-file errors are OK.
			if (feof (in))
			{
				// End of file.  Loop back.
				offset = 1;
				fclose (in);
				goto openmaplist;
			}

			// Other errors are not.
			gi.dprintf ("No maplist -- error reading ./%s/%s\n",
				gamedir->string, maplistfile->string);
			return false;
		}
	}

	// Chop the newline(s) from the end of the string.
	res_cp = buffer + strlen (buffer) - 1;
	while (res_cp >= buffer && (*res_cp == 10 || *res_cp == 13))
	{
		*res_cp = 0;
		res_cp--;
	}

	// If we didn't give them a map name, we failed.  (Leave the value of
	// maplist unchanged, so they know where to look for the problem.)
	if (!buffer[0])
		return false;

	// Retrieve the name of the next map to run.
	strcpy (level.nextmap, buffer);

	// Store the new maplist offset.
	offset++;
	sprintf (buffer, "%ld", offset);
	maplist = gi.cvar_set ("maplist", buffer);

	// Close the file.
	fclose (in);

	// Let them know we succeeded.
	return true;
}
