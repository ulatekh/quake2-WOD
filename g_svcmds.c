
#include "g_local.h"

void Svcmd_Survey_f (void);

void	Svcmd_Test_f (void)
{
	gi.cprintf (NULL, PRINT_HIGH, "Svcmd_Test_f()\n");
}

/*
==============================================================================

PACKET FILTERING
 

You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and any unspecified digits will
match any value, so you can specify an entire class C network with
"addip 192.246.40".

Removeip will only remove an address specified exactly the same way.  You
cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

writeip
Dumps "addip <ip>" commands to listip.cfg so it can be execed at a later date.  The filter lists are not saved and restored by default, because I beleive it would cause too much confusion.

filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be
prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you
easily set up a private game, or a game that only allows players from your
local network.


==============================================================================
*/

#define	MAX_IPFILTERS	1024

ipfilter_t	ipfilters[MAX_IPFILTERS];
int			numipfilters;

/*
=================
StringToFilter

The formats for IP filters recognized by this command are:

x.x.x.x
	Full IP address.
x.x.x.x/y.y.y.y
	IP address with a mask
x.x.x.x/y
	IP address with the number of significant bits

Omitting digits in the addr or mask automatically implies 0 there.
=================
*/
qboolean StringToFilter (char *s, ipfilter_t *f)
{
	char	num[128];
	int		i, j;
	byte	b[4];
	byte	m[4];
	
	for (i=0 ; i<4 ; i++)
	{
		b[i] = 0;
		m[i] = 255;
	}
	
	for (i=0 ; i<4 ; i++)
	{
		if (*s < '0' || *s > '9')
		{
			gi.cprintf(NULL, PRINT_HIGH, "Bad filter address: %s\n", s);
			return false;
		}
		
		j = 0;
		while (*s >= '0' && *s <= '9')
		{
			num[j++] = *s++;
		}
		num[j] = 0;
		b[i] = atoi(num);

		if (!*s)
		{
			for (i = i + 1; i < 4; i++)
				m[i] = 0;
			break;
		}
		s++;
	}
	
	f->mask = *(unsigned *)m;
	f->compare = *(unsigned *)b;

	return true;
}

/*
=================
SV_FilterPacket
=================
*/
qboolean SV_FilterPacket (char *from)
{
	int		i;
	unsigned	in;
	byte m[4];
	char *p;

	i = 0;
	p = from;
	while (*p && i < 4) {
		m[i] = 0;
		while (*p >= '0' && *p <= '9') {
			m[i] = m[i]*10 + (*p - '0');
			p++;
		}
		if (!*p || *p == ':')
			break;
		i++, p++;
	}
	
	in = *(unsigned *)m;

	for (i=0 ; i<numipfilters ; i++)
		if ( (in & ipfilters[i].mask) == ipfilters[i].compare)
			return (int)filterban->value;

	return (int)!filterban->value;
}


/*
=================
SV_AddIP_f
=================
*/
static void addIP (ipfilter_t ip)
{
	if (numipfilters == MAX_IPFILTERS)
	{
		gi.cprintf (NULL, PRINT_HIGH, "IP filter list is full\n");
		return;
	}
	
	ipfilters[numipfilters] = ip;
	numipfilters++;
}

void SVCmd_AddIP_f (void)
{
	int i;
	ipfilter_t ipAddr;

	if (gi.argc() < 3) {
		gi.cprintf(NULL, PRINT_HIGH, "Usage:  addip <ip-mask>\n");
		return;
	}


	for (i = 2; i < gi.argc(); i++)
		if (StringToFilter (gi.argv (i), &ipAddr))
			addIP (ipAddr);
}

/*
=================
SV_RemoveIP_f
=================
*/
void SVCmd_RemoveIP_f (void)
{
	ipfilter_t	f;
	int			i;

	if (gi.argc() < 3) {
		gi.cprintf(NULL, PRINT_HIGH, "Usage:  sv removeip <ip-mask>\n");
		return;
	}

	if (!StringToFilter (gi.argv(2), &f))
		return;

	for (i=0 ; i<numipfilters ; i++)
	{
		if (ipfilters[i].mask == f.mask
		&& ipfilters[i].compare == f.compare)
		{
			if (numipfilters > 0)
				ipfilters[i] = ipfilters[numipfilters - 1];
			numipfilters--;
			gi.cprintf (NULL, PRINT_HIGH, "Removed.\n");
			return;
		}
	}
	gi.cprintf (NULL, PRINT_HIGH, "Didn't find %s.\n", gi.argv(2));
}

/*
=================
SV_ListIP_f
=================
*/
void SVCmd_ListIP_f (void)
{
	int		i;
	byte	b[4];

	gi.cprintf (NULL, PRINT_HIGH, "Filter list:\n");
	for (i=0 ; i<numipfilters ; i++)
	{
		*(unsigned *)b = ipfilters[i].compare;
		gi.cprintf (NULL, PRINT_HIGH, "%3i.%3i.%3i.%3i\n", b[0], b[1], b[2], b[3]);
	}
}

/*
=================
SV_WriteIP_f
=================
*/
void SVCmd_WriteIP_f (void)
{
	FILE	*f;
	char	name[MAX_OSPATH];
	byte	b[4];
	int		i;

	if (gamedir->string && gamedir->string[0])
		sprintf (name, "./%s/listip.cfg", gamedir->string);
	else
		sprintf (name, "./listip.cfg");

	gi.cprintf (NULL, PRINT_HIGH, "Writing %s.\n", name);

	f = fopen (name, "wb");
	if (!f)
	{
		gi.cprintf (NULL, PRINT_HIGH, "Couldn't open %s\n", name);
		return;
	}
	
	fprintf(f, "set filterban %d\n", (int)filterban->value);

	for (i=0 ; i<numipfilters ; i++)
	{
		*(unsigned *)b = ipfilters[i].compare;
		fprintf (f, "sv addip %i.%i.%i.%i\n", b[0], b[1], b[2], b[3]);
	}
	
	fclose (f);
}


// Used to survey parts of the map (usually bases).
void Svcmd_Survey_f (void)
{
	int x, y;
	trace_t tr;
	vec3_t mins, maxs;
	FILE *out;
	vec3_t start, end;
	vec3_t lookto, lookvec, lookangs;

	out = fopen ("survey.txt", "wt");
	if (!out)
	{
		gi.cprintf (NULL, PRINT_HIGH, "Couldn't open survey.txt\n");
		return;
	}

	VectorSet (mins, -32, -32, -24);
	VectorSet (maxs, 32, 32, -24);

	for (y = -319; y <= 127; y += 50)
	{
		for (x = -511; x <= 319; x += 50)
		{
			VectorSet (start, x, y, (126 + 24));
			VectorSet (end, x, y, -1);
			VectorSet (lookto, -127,-352, 0);

			tr = gi.trace (start, mins, maxs, end, NULL, MASK_SOLID);

			if (tr.allsolid || tr.startsolid)
				continue;
			if ((int)tr.endpos[2] != 24)
				continue;

			VectorSubtract (lookto, tr.endpos, lookvec);
			vectoangles (lookvec, lookangs);

			fprintf (out, "{\n");
			fprintf (out, "\"classname\" \"info_player_team1\"\n");
			fprintf (out, "\"origin\" \"%i %i %i\"\n",
				(int)tr.endpos[0], (int)tr.endpos[1], (int)tr.endpos[2]);
			fprintf (out, "\"angle\" \"%i\"\n", (int)lookangs[YAW]);
			fprintf (out, "}\n");
		}
	}

	for (y = 1536; y <= 1823; y += 50)
	{
		for (x = 1920; x <= 2687; x += 50)
		{
			VectorSet (start, x, y, (-65 + 24));
			VectorSet (end, x, y, -256);
			VectorSet (lookto, 2112, 1503, -255);

			tr = gi.trace (start, mins, maxs, end, NULL, MASK_SOLID);

			if (tr.allsolid || tr.startsolid)
				continue;
			if ((int)tr.endpos[2] != -231)
				continue;

			VectorSubtract (lookto, tr.endpos, lookvec);
			vectoangles (lookvec, lookangs);

			fprintf (out, "{\n");
			fprintf (out, "\"classname\" \"info_player_team2\"\n");
			fprintf (out, "\"origin\" \"%i %i %i\"\n",
				(int)tr.endpos[0], (int)tr.endpos[1], (int)tr.endpos[2]);
			fprintf (out, "\"angle\" \"%i\"\n", (int)lookangs[YAW]);
			fprintf (out, "}\n");
		}
	}

	gi.cprintf (NULL, PRINT_HIGH, "Survey done.\n");
	fclose (out);
}


/*
=================
SVCmd_KickBan_f
=================
*/
void SVCmd_KickBan_f (void)
{
	int i;
	edict_t *ent;
	
	if (gi.argc() != 3)
	{
		gi.cprintf (NULL, PRINT_HIGH, "Usage: kickban <player#>\n");
		return;
	}

	i = atoi (gi.argv (2));
	if (i >= 0 && i < (int)maxclients->value)
	{
		ent = g_edicts + i + 1;
		if (ent->inuse && ent->client)
		{
			ipfilter_t ipAddr;

			// Ban them from the server.
			ipAddr.compare = ent->client->pers.ipAddr;
			ipAddr.mask = 0xffffffff;
			addIP (ipAddr);

			// Now kick them.
			gi.AddCommandString (va ("kick %d\n", i));
			stuffcmd (ent, "disconnect\n");
		}
	}
}


/*
=================
SVCmd_Team_f
=================
*/
void SVCmd_Team_f (void)
{
	int team;
	int i;
	edict_t *ent;

	if (!ctf->value)
		return;
	
	if (gi.argc() != 4)
	{
		gi.cprintf (NULL, PRINT_HIGH, "Usage: team <red/blue/none> <player#>\n");
		return;
	}

	if (Q_stricmp (gi.argv (2), "red") == 0)
		team = CTF_TEAM1;
	else if (Q_stricmp (gi.argv (2), "blue") == 0)
		team = CTF_TEAM2;
	else if (Q_stricmp (gi.argv (2), "none") == 0)
		team = CTF_NOTEAM;
	else
	{
		gi.cprintf (NULL, PRINT_HIGH, "Unknown team: %s\n", gi.argv (2));
		return;
	}

	i = atoi (gi.argv (3));
	if (i >= 0 && i < (int)maxclients->value)
	{
		ent = g_edicts + i + 1;
		if (ent->inuse && ent->client)
		{
			// Forcibly alter their team.
			CTFSetTeam (ent, team);
		}
	}
}


void SVCmd_PlaySound_f (void)
{
	if (gi.argc() != 3)
	{
		gi.cprintf (NULL, PRINT_HIGH, "Usage: playsound <filename>\n");
		return;
	}

	// Play the sound they want.
	gi.sound (world, CHAN_SONG+CHAN_RELIABLE+CHAN_NO_PHS_ADD,
		gi.soundindex (gi.argv (2)), 1, ATTN_NONE, 0);
}

/*
=================
ServerCommand

ServerCommand will be called when an "sv" command is issued.
The game can issue gi.argc() / gi.argv() commands to get the rest
of the parameters
=================
*/
void	ServerCommand (void)
{
	char	*cmd;

	cmd = gi.argv(1);
	if (Q_stricmp (cmd, "test") == 0)
		Svcmd_Test_f();
	else if (Q_stricmp (cmd, "addip") == 0)
		SVCmd_AddIP_f ();
	else if (Q_stricmp (cmd, "removeip") == 0)
		SVCmd_RemoveIP_f ();
	else if (Q_stricmp (cmd, "listip") == 0)
		SVCmd_ListIP_f ();
	else if (Q_stricmp (cmd, "writeip") == 0)
		SVCmd_WriteIP_f ();
	else if (Q_stricmp (cmd, "kickban") == 0)
		SVCmd_KickBan_f ();
	else if (Q_stricmp (cmd, "playsound") == 0)
		SVCmd_PlaySound_f();
	else if (Q_stricmp (cmd, "survey") == 0)
		Svcmd_Survey_f();
	else if (Q_stricmp (cmd, "team") == 0)
		SVCmd_Team_f();
	else
		gi.cprintf (NULL, PRINT_HIGH, "Unknown server command \"%s\"\n", cmd);
}
