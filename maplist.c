#include "g_local.h"



cvar_t *maplist;


// Returns true if it has stored the name of
// the next map to run in level.nextmap .

qboolean MaplistNext (void)
{
	long i, offset;
	FILE *in;
	char *res_cp;
	char buffer[MAX_QPATH + 1];

	// Make sure we can find the game directory.
	if (!gamedir)
	{
		gi.dprintf ("No maplist -- can't find gamedir\n");
		return false;
	}

	// Get the offset in the maplist.txt file.  Zero means maplist is turned
	// off.
	offset = (int)(maplist->value);
	if (offset <= 0)
		return false;

	// Open the maplist file.
openmaplist:
	sprintf (buffer, "./%s/maplist.txt", gamedir->string);
	in = fopen (buffer, "r");
	if (in == NULL)
	{
		gi.dprintf ("No maplist -- can't open ./%s/maplist.txt\n",
			gamedir->string);
		return false;
	}

	// Skip as many entries as we need to.  (This is a stupid and inefficient way
	// to do it, but ftell() keeps returning -555, so I can't do it that way.)
	for (i = 0; i < offset; i++)
	{
		res_cp = fgets (buffer, sizeof (buffer), in);
		if (res_cp == NULL && feof (in))
		{
			// End of file.  Loop back.
			offset = 1;
			fclose (in);
			goto openmaplist;
		}
	}

	// Chop the newline from the end of the string.
	buffer[strlen (buffer) - 1] = '\0';

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
