#include "g_local.h"

typedef struct ctfgame_s ctfgame_t;
struct ctfgame_s
{
	// The number of spawnpoints under the control of each team.
	int spawn1, spawn2;

	// The total number of spawnpoints they're fighting over.
	int spawncount;

	// The total frag scores.  (Not set until the end of the game.)
	int total1, total2;

	// The next time to try to respawn ghost players.
	float checkRespawnTime1, checkRespawnTime2;

	// Where to spawn the ghost player.
	edict_t *spawnHere;

	// When to start the match.  (0 == start immediately.)
	int matchStartFrame;

	// What round we're playing.  (Either 1 or 2.)
	int round;
};

ctfgame_t ctfgame;

cvar_t *teamplay;
cvar_t *ctf;
cvar_t *ctf_forcejoin;

char *teamplay_statusbar =
"yb	-24 "

// health
"xv	0 "
"hnum "
"xv	50 "
"pic 0 "

// ammo
"if 2 "
"	xv	100 "
"	anum "
"	xv	150 "
"	pic 2 "
"endif "

// armor
"if 4 "
"	xv	200 "
"	rnum "
"	xv	250 "
"	pic 4 "
"endif "

// selected item
"if 6 "
"	xv	296 "
"	pic 6 "
"endif "

"yb	-50 "

// picked up item
"if 7 "
"	xv	0 "
"	pic 7 "
"	xv	26 "
"	yb	-42 "
"	stat_string 8 "
"	yb	-50 "
"endif "

//  help / weapon icon 
"if 11 "
  "xv 148 "
  "pic 11 "
"endif "

//  frags
"xr	-50 "
"yt 2 "
"num 3 14 "

// red team
"yb -102 "
"if 17 "
  "xr -26 "
  "pic 17 "
"endif "
//joined overlay
"if 22 "
  "yb -104 "
  "xr -28 "
  "pic 22 "
"endif "

// blue team
"yb -75 "
"if 19 "
  "xr -26 "
  "pic 19 "
"endif "
//joined overlay
"if 23 "
  "yb -77 "
  "xr -28 "
  "pic 23 "
"endif "

// id view state
"if 27 "
  "xv 0 "
  "yb -58 "
  "string \"Viewing\" "
  "xv 64 "
  "stat_string 27 "
"endif "
;

char *ctf_statusbar =
"yb	-24 "

// health
"xv	0 "
"hnum "
"xv	50 "
"pic 0 "

// ammo
"if 2 "
"	xv	100 "
"	anum "
"	xv	150 "
"	pic 2 "
"endif "

// armor
"if 4 "
"	xv	200 "
"	rnum "
"	xv	250 "
"	pic 4 "
"endif "

// selected item
"if 6 "
"	xv	296 "
"	pic 6 "
"endif "

"yb	-50 "

// picked up item
"if 7 "
"	xv	0 "
"	pic 7 "
"	xv	26 "
"	yb	-42 "
"	stat_string 8 "
"	yb	-50 "
"endif "

// timer
"if 9 "
  "xv 246 "
  "num 2 10 "
  "xv 296 "
  "pic 9 "
"endif "

//  help / weapon icon 
"if 11 "
  "xv 148 "
  "pic 11 "
"endif "

//  frags
"xr	-50 "
"yt 2 "
"num 3 14 "

// red team
"yb -102 "
"if 17 "
  "xr -26 "
  "pic 17 "
"endif "
"xr -62 "
"num 2 18 "
//joined overlay
"if 22 "
  "yb -104 "
  "xr -28 "
  "pic 22 "
"endif "

// blue team
"yb -75 "
"if 19 "
  "xr -26 "
  "pic 19 "
"endif "
"xr -62 "
"num 2 20 "
//joined overlay
"if 23 "
  "yb -77 "
  "xr -28 "
  "pic 23 "
"endif "

//uncaptured spawnpoints
"if 26 "
  "yb -50 "
  "xr -62 "
  "num 2 26 "
"endif "

// id view state
"if 27 "
  "xv 0 "
  "yb -58 "
  "string \"Viewing\" "
  "xv 64 "
  "stat_string 27 "
"endif "
;

void stuffcmd(edict_t *ent, char *s) 	
{
	gi.WriteByte (svc_stufftext);	        
	gi.WriteString (s);
	gi.unicast (ent, true);	
}

/*--------------------------------------------------------------------------*/

void loc_buildboxpoints(vec3_t p[8], vec3_t org, vec3_t mins, vec3_t maxs)
{
	VectorAdd(org, mins, p[0]);
	VectorCopy(p[0], p[1]);
	p[1][0] -= mins[0];
	VectorCopy(p[0], p[2]);
	p[2][1] -= mins[1];
	VectorCopy(p[0], p[3]);
	p[3][0] -= mins[0];
	p[3][1] -= mins[1];
	VectorAdd(org, maxs, p[4]);
	VectorCopy(p[4], p[5]);
	p[5][0] -= maxs[0];
	VectorCopy(p[0], p[6]);
	p[6][1] -= maxs[1];
	VectorCopy(p[0], p[7]);
	p[7][0] -= maxs[0];
	p[7][1] -= maxs[1];
}

static qboolean loc_CanSee (edict_t *targ, edict_t *inflictor)
{
	trace_t	trace;
	vec3_t	targpoints[8];
	int i;
	vec3_t viewpoint;

// bmodels need special checking because their origin is 0,0,0
	if (targ->movetype == MOVETYPE_PUSH)
		return false; // bmodels not supported

	// if not in pvs, can't see
	if (!gi.inPVS (targ->s.origin, inflictor->s.origin))
		return false;

	loc_buildboxpoints(targpoints, targ->s.origin, targ->mins, targ->maxs);
	
	VectorCopy(inflictor->s.origin, viewpoint);
	viewpoint[2] += inflictor->viewheight;

	for (i = 0; i < 8; i++) {
		trace = gi.trace (viewpoint, vec3_origin, vec3_origin, targpoints[i], inflictor, MASK_SOLID);
		if (trace.fraction == 1.0)
			return true;
	}

	return false;
}

/*--------------------------------------------------------------------------*/

void CTFInit(void)
{
	// Reset the teamplay game status.
	memset (&ctfgame, 0, sizeof (ctfgame));

	// See if we're doing teamplay.
	teamplay = gi.cvar ("teamplay", "0", CVAR_SERVERINFO|CVAR_LATCH);
	ctf = gi.cvar ("ctf", "0", CVAR_SERVERINFO|CVAR_LATCH);
	ctf_forcejoin = gi.cvar ("ctf_forcejoin", "", 0);

	// Make sure the variables are consistent.
	if (!teamplay->value && ctf->value)
		gi.error ("ctf set without teamplay being set");

	// If we are doing Extinction, set up a few things.
	if (ctf->value)
	{
		// Change the gamename.
		gi.cvar_forceset ("gamename", GAMEVERSION_E);

		// Start the match in 40 seconds.
		ctfgame.matchStartFrame = 400;

		// Start with round 1.
		ctfgame.round = 1;
	}

	// If we are doing teamplay DM, set up a few things.
	else if (teamplay->value)
	{
		// Change the gamename.
		gi.cvar_forceset ("gamename", GAMEVERSION_T);
	}
}

/*--------------------------------------------------------------------------*/

char *CTFTeamName(int team)
{
	switch (team) {
	case CTF_TEAM1:
		return "RED";
	case CTF_TEAM2:
		return "BLUE";
	}
	return "UNKNOWN";
}

char *CTFOtherTeamName(int team)
{
	switch (team) {
	case CTF_TEAM1:
		return "BLUE";
	case CTF_TEAM2:
		return "RED";
	}
	return "UNKNOWN";
}

int CTFOtherTeam(int team)
{
	switch (team) {
	case CTF_TEAM1:
		return CTF_TEAM2;
	case CTF_TEAM2:
		return CTF_TEAM1;
	}
	return -1; // invalid value
}

/*--------------------------------------------------------------------------*/

edict_t *SelectRandomDeathmatchSpawnPoint (void);
edict_t *SelectFarthestDeathmatchSpawnPoint (void);
float	PlayersRangeFromSpot (edict_t *spot);

void CTFAssignSkin(edict_t *ent, char *s)
{
	int playernum = ent-g_edicts-1;
	char *p;
	char t[64];

	Com_sprintf(t, sizeof(t), "%s", s);

	if ((p = strrchr(t, '/')) != NULL)
		p[1] = 0;
	else
		strcpy(t, "male/");

	switch (ent->client->resp.ctf_team) {
	case CTF_TEAM1:
		gi.configstring (CS_PLAYERSKINS+playernum, va("%s\\%s%s", 
			ent->client->pers.netname, t, CTF_TEAM1_SKIN) );
		break;
	case CTF_TEAM2:
		gi.configstring (CS_PLAYERSKINS+playernum,
			va("%s\\%s%s", ent->client->pers.netname, t, CTF_TEAM2_SKIN) );
		break;
	default:
		gi.configstring (CS_PLAYERSKINS+playernum, 
			va("%s\\%s", ent->client->pers.netname, s) );
		break;
	}
//	gi.cprintf(ent, PRINT_HIGH, "You have been assigned to %s team.\n", ent->client->pers.netname);
}

void CTFAssignTeam(gclient_t *who)
{
	edict_t		*player;
	int i;
	int team1count = 0, team2count = 0;

	who->resp.ctf_state = CTF_STATE_START;

	if (!((int)dmflags->value & DF_CTF_FORCEJOIN)) {
		who->resp.ctf_team = CTF_NOTEAM;
		return;
	}

	for (i = 1; i <= maxclients->value; i++) {
		player = &g_edicts[i];

		if (!player->inuse || player->client == who)
			continue;

		switch (player->client->resp.ctf_team) {
		case CTF_TEAM1:
			team1count++;
			break;
		case CTF_TEAM2:
			team2count++;
		}
	}
	if (team1count < team2count)
		who->resp.ctf_team = CTF_TEAM1;
	else if (team2count < team1count)
		who->resp.ctf_team = CTF_TEAM2;
	else if (rand() & 1)
		who->resp.ctf_team = CTF_TEAM1;
	else
		who->resp.ctf_team = CTF_TEAM2;
}

/*
================
SelectCTFSpawnPoint

Find a place to spawn this player.  Returns true if a team-spawnpoint was found
and the player can be spawned live.  Returns false if the player should be
spawned as a ghost, waiting for a spawnpoint to open up.
================
*/
edict_t *SelectCTFSpawnPoint (edict_t *ent, qboolean *spawnlive)
{
	edict_t	*spot, *spot1, *spot2;
	int count = 0;
	int selection;
	float	range, range1, range2;

	spot = NULL;
	*spawnlive = true;

	// If we already found the spot, don't look again.
	if (ctfgame.spawnHere)
	{
		spot = ctfgame.spawnHere;
		ctfgame.spawnHere = NULL;
		goto spotFound;
	}

	// If the game hasn't started yet, spawn as a ghost.
	if (ctfgame.matchStartFrame && level.framenum < ctfgame.matchStartFrame)
		*spawnlive = false;

	// Try to spawn at a team-owned spawnpoint.
	if (ent->client->resp.ctf_state == CTF_STATE_PLAYING
	&& (ctfgame.spawn1 > 0 || ctfgame.spawn2 > 0)
	&& (!ctfgame.matchStartFrame || level.framenum > ctfgame.matchStartFrame))
	{
		float lowRespawnTime;
			// The respawn time of the player we can telefrag first.

		range1 = range2 = 99999;
		spot = spot1 = spot2 = NULL;
		lowRespawnTime = level.time + 1.0;

		while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch"))
			!= NULL)
		{
			// If the spawnpoint doesn't belong to this team, skip it.
			if ((ent->client->resp.ctf_team == CTF_TEAM1
				&& !(spot->s.renderfx & RF_SHELL_RED))
			|| (ent->client->resp.ctf_team == CTF_TEAM2
				&& !(spot->s.renderfx & RF_SHELL_BLUE)))
			{
				continue;
			}

			// Find any teammates on this spawnpoint, see how long they've been
			// there.
			{
				trace_t tr;
				vec3_t startpos, endpos;

				// Set up the start/end of the trace, to see if anyone is standing
				// on the spawnpoint.
				VectorCopy (spot->s.origin, startpos);
				startpos[2] += 8;
				VectorCopy (spot->s.origin, endpos);
				endpos[2] += 64;

				// Locate any teammate on this spawnpoint, remember when they
				// spawned into the game.
				spot->freetime = level.time + 1.0;
				tr = gi.trace (startpos, spot->mins, spot->maxs, endpos, spot,
					MASK_PLAYERSOLID);

				// If this player is on our team, keep track of when they spawned
				// into the game.
				if (tr.ent && tr.ent->client
				&& tr.ent->client->resp.ctf_team == ent->client->resp.ctf_team
				&& spot->freetime > tr.ent->client->respawn_time)
					spot->freetime = tr.ent->client->respawn_time;
			}

			// If any teammates were found on this spawnpoint, and we want to give
			// them more time to get off of it, skip this spot.
			if (level.time - spot->freetime > 0.0
			&& level.time - spot->freetime < 3.0)
			{
				// Remember the respawn-time of the first player we can telefrag.
				if (lowRespawnTime > spot->freetime)
					lowRespawnTime = spot->freetime;
				continue;
			}
			else
				spot->freetime = 0.0;

			count++;

			range = PlayersRangeFromSpot(spot);
			if (range < range1)
			{
				range1 = range;
				spot1 = spot;
			}
			else if (range < range2)
			{
				range2 = range;
				spot2 = spot;
			}
		}

		// If we have a choice of spawnpoints, pick one.
		if (count)
		{
			if (count <= 2)
			{
				spot1 = spot2 = NULL;
			}
			else
				count -= 2;

			selection = rand() % count;

			spot = NULL;
			do
			{
				spot = G_Find (spot, FOFS(classname), "info_player_deathmatch");
				if ((ent->client->resp.ctf_team == CTF_TEAM1
					&& !(spot->s.renderfx & RF_SHELL_RED))
				|| (ent->client->resp.ctf_team == CTF_TEAM2
					&& !(spot->s.renderfx & RF_SHELL_BLUE))
				|| spot == spot1 || spot == spot2
				|| spot->freetime > 0.0)
				{
					// Skip this one.
					selection++;
					spot->freetime = 0.0;
				}
			} while(selection--);
		}

		// If we still couldn't find a spot, then this player will have to be a
		// ghost until they can respawn live.
		if (!spot)
		{
			// In addition, no one on their team is going to be able to respawn for
			// a few seconds, either.
			if (lowRespawnTime < level.time + 1.0)
			{
				// Three seconds, to be exact.
				lowRespawnTime += 3.0;

				if (ent->client->resp.ctf_team == CTF_TEAM1)
				{
					if (ctfgame.checkRespawnTime1 < lowRespawnTime)
						ctfgame.checkRespawnTime1 = lowRespawnTime;
				}
				else if (ent->client->resp.ctf_team == CTF_TEAM2)
				{
					if (ctfgame.checkRespawnTime2 < lowRespawnTime)
						ctfgame.checkRespawnTime2 = lowRespawnTime;
				}
			}

			*spawnlive = false;
		}
	}

	// If the game just started, or if they can't spawn live, let them spawn
	// in the base.
	if (ent->client->resp.ctf_state == CTF_STATE_START
	|| (ctfgame.spawn1 == 0 && ctfgame.spawn2 == 0)
	|| !spot
	|| (ctfgame.matchStartFrame > 0
		&& level.framenum == ctfgame.matchStartFrame))
	{
		char *cname;

		// Anyone joining an extinct team starts as a ghost.
		if (ent->client->resp.ctf_state == CTF_STATE_START
		&& ((ent->client->resp.ctf_team == CTF_TEAM1
				&& ctfgame.spawn1 == 0 && ctfgame.spawn2 > 0)
			|| (ent->client->resp.ctf_team == CTF_TEAM2
				&& ctfgame.spawn2 == 0 && ctfgame.spawn1 > 0)))
			*spawnlive = false;

		ent->client->resp.ctf_state = CTF_STATE_PLAYING;

		switch (ent->client->resp.ctf_team)
		{
			case CTF_TEAM1:
				if (TeamplayCheckRound1())
					cname = "info_player_team1";
				else
					cname = "info_player_team2";
				break;
			case CTF_TEAM2:
				if (TeamplayCheckRound1())
					cname = "info_player_team2";
				else
					cname = "info_player_team1";
				break;
			default:
				spot = G_Find (NULL, FOFS(classname), "info_player_start");
				*spawnlive = false; // redundant; teamless players are ghosts anyway
				goto spotFound;
		}

		spot = NULL;
		range1 = range2 = 99999;
		spot1 = spot2 = NULL;

		while ((spot = G_Find (spot, FOFS(classname), cname)) != NULL)
		{
			count++;
			range = PlayersRangeFromSpot(spot);
			if (range < range1)
			{
				range1 = range;
				spot1 = spot;
			}
			else if (range < range2)
			{
				range2 = range;
				spot2 = spot;
			}
		}

		if (!count)
		{
			spot = SelectRandomDeathmatchSpawnPoint();
			goto spotFound;
		}

		if (count <= 2)
		{
			spot1 = spot2 = NULL;
		}
		else
			count -= 2;

		selection = rand() % count;

		spot = NULL;
		do
		{
			spot = G_Find (spot, FOFS(classname), cname);
			if (spot == spot1 || spot == spot2)
				selection++;
		} while (selection--);
	}

spotFound:

	return spot;
}

// called when we enter the intermission
void CTFCalcScores(void)
{
	int i;

	ctfgame.total1 = ctfgame.total2 = 0;
	for (i = 0; i < maxclients->value; i++)
	{
		if (!g_edicts[i+1].inuse)
			continue;
		if (game.clients[i].resp.ctf_team == CTF_TEAM1)
			ctfgame.total1 += game.clients[i].resp.score;
		else if (game.clients[i].resp.ctf_team == CTF_TEAM2)
			ctfgame.total2 += game.clients[i].resp.score;
	}
}

void CTFID_f (edict_t *ent)
{
	// No player ID if it's been banned.
	if ((int)featureban->value & FB_ID)
		return;

	if (ent->client->resp.id_state) {
		gi.cprintf(ent, PRINT_HIGH, "Disabling player identication display.\n");
		ent->client->resp.id_state = false;
	} else {
		gi.cprintf(ent, PRINT_HIGH, "Activating player identication display.\n");
		ent->client->resp.id_state = true;
	}
}

void CTFID (edict_t *ent, pmenu_t *p)
{
	// No player ID if it's been banned.
	if ((int)featureban->value & FB_ID)
		return;

	ent->client->resp.id_state = !ent->client->resp.id_state;
	PMenu_Close (ent);
}

static void CTFSetIDView(edict_t *ent)
{
	vec3_t	forward, dir, end;
	trace_t	tr;
	edict_t	*who, *best;
	float	bd = 0, d;
	int i;

	ent->client->ps.stats[STAT_CTF_ID_VIEW] = 0;

	AngleVectors (ent->client->v_angle, forward, NULL, NULL);
	VectorMA (ent->s.origin, 1024, forward, end);
	tr = gi.trace(ent->s.origin, NULL, NULL, end, ent, MASK_SOLID);
	if (tr.fraction < 1 && tr.ent && tr.ent->client) {
		ent->client->ps.stats[STAT_CTF_ID_VIEW] = 
			CS_PLAYERSKINS + (tr.ent - g_edicts - 1);
		return;
	}

	best = NULL;
	for (i = 1; i <= maxclients->value; i++) {
		who = g_edicts + i;
		if (!who->inuse)
			continue;
		VectorSubtract(who->s.origin, ent->s.origin, dir);
		VectorNormalize(dir);
		d = DotProduct(forward, dir);
		if (d > bd && loc_CanSee(who, ent)) {
			bd = d;
			best = who;
		}
	}
	if (bd > 0.90)
		ent->client->ps.stats[STAT_CTF_ID_VIEW] = 
			CS_PLAYERSKINS + (best - g_edicts - 1);
}

void SetCTFStats(edict_t *ent)
{
	int p1, p2;

	if (!teamplay->value)
		return;

	// logo headers for the frag display
	ent->client->ps.stats[STAT_CTF_TEAM1_HEADER] = gi.imageindex ("ctfsb1");
	ent->client->ps.stats[STAT_CTF_TEAM2_HEADER] = gi.imageindex ("ctfsb2");

	// if during intermission, we must blink the team header of the winning team
	if ((level.intermissiontime || level.framenum < ctfgame.matchStartFrame)
	&& (level.framenum & 8)) // blink 1/8th second
	{
		if (ctf->value)
		{
			if (ctfgame.spawn1 == ctfgame.spawncount)
				ent->client->ps.stats[STAT_CTF_TEAM1_HEADER] = 0;
			else if (ctfgame.spawn2 == ctfgame.spawncount)
				ent->client->ps.stats[STAT_CTF_TEAM2_HEADER] = 0;
			// else tie game!
		}
		else if (teamplay->value)
		{
			if (ctfgame.total1 > ctfgame.total2)
				ent->client->ps.stats[STAT_CTF_TEAM1_HEADER] = 0;
			else if (ctfgame.total1 < ctfgame.total2)
				ent->client->ps.stats[STAT_CTF_TEAM2_HEADER] = 0;
			// else tie game!
		}
	}

	// figure out what icon to display for team logos
	// three states:
	//   flag at base
	//   flag taken
	//   flag dropped
	p1 = gi.imageindex ("i_ctf1");
	p2 = gi.imageindex ("i_ctf2");

	ent->client->ps.stats[STAT_CTF_TEAM1_PIC] = p1;
	ent->client->ps.stats[STAT_CTF_TEAM2_PIC] = p2;

	ent->client->ps.stats[STAT_CTF_JOINED_TEAM1_PIC] = 0;
	ent->client->ps.stats[STAT_CTF_JOINED_TEAM2_PIC] = 0;
	if (ent->client->resp.ctf_team == CTF_TEAM1)
		ent->client->ps.stats[STAT_CTF_JOINED_TEAM1_PIC]
			= gi.imageindex ("i_ctfj");
	else if (ent->client->resp.ctf_team == CTF_TEAM2)
		ent->client->ps.stats[STAT_CTF_JOINED_TEAM2_PIC]
			= gi.imageindex ("i_ctfj");

	ent->client->ps.stats[STAT_CTF_ID_VIEW] = 0;
	if (ent->client->resp.id_state)
		CTFSetIDView (ent);

	if (ctf->value)
	{
		ent->client->ps.stats[STAT_CTF_TEAM1_CAPS] = ctfgame.spawn1;
		ent->client->ps.stats[STAT_CTF_TEAM2_CAPS] = ctfgame.spawn2;
		ent->client->ps.stats[STAT_CTF_NOTCAPPED] = ctfgame.spawncount
			- ctfgame.spawn1 - ctfgame.spawn2;

		// Display countdown to the beginning of the match.
		if (ctfgame.matchStartFrame)
		{
			if (level.framenum & 4)
			{
				if (ent->client->resp.ctf_team == CTF_TEAM1)
					ent->client->ps.stats[STAT_TIMER_ICON] = p1;
				else if (ent->client->resp.ctf_team == CTF_TEAM2)
					ent->client->ps.stats[STAT_TIMER_ICON] = p2;
			}
			else
			{
				if (ent->client->resp.ctf_team == CTF_TEAM1)
					ent->client->ps.stats[STAT_TIMER_ICON]
						= gi.imageindex ("i_ctf1t");
				else if (ent->client->resp.ctf_team == CTF_TEAM2)
					ent->client->ps.stats[STAT_TIMER_ICON]
						= gi.imageindex ("i_ctf2t");
			}
			ent->client->ps.stats[STAT_TIMER]
				= (ctfgame.matchStartFrame + 3 - level.framenum)/10;
		}
	}
}

/*------------------------------------------------------------------------*/

/*QUAKED info_player_team1 (1 0 0) (-16 -16 -24) (16 16 32)
potential team1 spawning position for ctf games
*/
void SP_info_player_team1 (edict_t *self)
{
	// Only for CTF.
	if (!ctf->value)
		G_FreeEdict (self);

	// No need to send this to the client.
	self->svflags |= SVF_NOCLIENT;
}

/*QUAKED info_player_team2 (0 0 1) (-16 -16 -24) (16 16 32)
potential team2 spawning position for ctf games
*/
void SP_info_player_team2 (edict_t *self)
{
	// Only for CTF.
	if (!ctf->value)
		G_FreeEdict (self);

	// No need to send this to the client.
	self->svflags |= SVF_NOCLIENT;
}

void CTFSetTeam (edict_t *ent, int desired_team)
{
	char *s;

	ent->svflags = 0;
	ent->flags &= ~FL_GODMODE;
	ent->client->resp.ctf_team = desired_team;
	ent->client->resp.ctf_state = CTF_STATE_START;
	s = Info_ValueForKey (ent->client->pers.userinfo, "skin");
	CTFAssignSkin(ent, s);

	if (ent->solid == SOLID_NOT) { // spectator
		PutClientInServer (ent);
		// add a teleportation effect
		ent->s.event = EV_PLAYER_TELEPORT;
		// hold in place briefly
		ent->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		ent->client->ps.pmove.pm_time = 14;
		gi.bprintf(PRINT_HIGH, "%s joined the %s team.\n",
			ent->client->pers.netname, CTFTeamName(desired_team));
		return;
	}

	ent->health = 0;
	player_die (ent, ent, ent, 100000, vec3_origin);
	// don't even bother waiting for death frames
	ent->deadflag = DEAD_DEAD;
	respawn (ent);

	ent->client->resp.score = 0;

	gi.bprintf(PRINT_HIGH, "%s changed to the %s team.\n",
		ent->client->pers.netname, CTFTeamName(desired_team));
}

void CTFTeam_f (edict_t *ent)
{
	char *t;
	int desired_team;

	if (!teamplay->value)
		return;

	t = gi.args();
	if (!*t) {
		gi.cprintf(ent, PRINT_HIGH, "You are on the %s team.\n",
			CTFTeamName (ent->client->resp.ctf_team));
		return;
	}

	if (Q_stricmp(t, "red") == 0)
		desired_team = CTF_TEAM1;
	else if (Q_stricmp(t, "blue") == 0)
		desired_team = CTF_TEAM2;
	else
	{
		gi.cprintf(ent, PRINT_HIGH, "Unknown team %s.\n", t);
		return;
	}

	if (ent->client->resp.ctf_team == desired_team)
	{
		gi.cprintf(ent, PRINT_HIGH, "You are already on the %s team.\n",
			CTFTeamName (ent->client->resp.ctf_team));
		return;
	}

	// Don't let them change more often than every 5 seconds -- prevent
	// team-change flooding.  (Damn pinheads ruin it for everyone.)
	if ((level.time - ent->client->respawn_time) < 5)
	{
		gi.cprintf (ent, PRINT_HIGH, "You can't change teams for another %.1f "
			"seconds.\n", level.time - ent->client->respawn_time);
		return;
	}

	CTFSetTeam (ent, desired_team);
}

/*
==================
CTFScoreboardMessage
==================
*/
void CTFScoreboardMessage (edict_t *ent, edict_t *killer, char string[1400])
{
	char	entry[1024];
	int		len;
	int		i, j, k, n;
	int		sorted[2][MAX_CLIENTS];
	int		sortedscores[2][MAX_CLIENTS];
	int		score, total[2], totalscore[2];
	int		last[2];
	gclient_t	*cl;
	edict_t		*cl_ent;
	int team;
	int maxsize = 1000;

	// sort the clients by team and score
	total[0] = total[1] = 0;
	last[0] = last[1] = 0;
	totalscore[0] = totalscore[1] = 0;
	for (i=0 ; i<game.maxclients ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse)
			continue;
		if (game.clients[i].resp.ctf_team == CTF_TEAM1)
			team = 0;
		else if (game.clients[i].resp.ctf_team == CTF_TEAM2)
			team = 1;
		else
			continue; // unknown team?

		score = game.clients[i].resp.score;
		for (j=0 ; j<total[team] ; j++)
		{
			if (score > sortedscores[team][j])
				break;
		}
		for (k=total[team] ; k>j ; k--)
		{
			sorted[team][k] = sorted[team][k-1];
			sortedscores[team][k] = sortedscores[team][k-1];
		}
		sorted[team][j] = i;
		sortedscores[team][j] = score;
		totalscore[team] += score;
		total[team]++;
	}

	// print level name and exit rules
	// add the clients in sorted order
	*string = 0;
	len = 0;

	// team one
	if (ctf->value)
		sprintf(string, "if 24 xv 8 yv 8 pic 24 endif "
			"xv 40 yv 28 string \"%4d/%-3d\" "
			"xv 98 yv 12 num 2 18 "
			"if 25 xv 168 yv 8 pic 25 endif "
			"xv 200 yv 28 string \"%4d/%-3d\" "
			"xv 256 yv 12 num 2 20 ",
			totalscore[0], total[0],
			totalscore[1], total[1]);
	else
		sprintf(string, "if 24 xv 8 yv 8 pic 24 endif "
			"xv 40 yv 28 string \"%4d/%-3d\" "
			"if 25 xv 168 yv 8 pic 25 endif "
			"xv 200 yv 28 string \"%4d/%-3d\" ",
			totalscore[0], total[0],
			totalscore[1], total[1]);
	len = strlen(string);

	for (i=0 ; i<16 ; i++)
	{
		if (i >= total[0] && i >= total[1])
			break; // we're done

#if 0 //ndef NEW_SCORE
		// set up y
		sprintf(entry, "yv %d ", 42 + i * 8);
		if (maxsize - len > strlen(entry)) {
			strcat(string, entry);
			len = strlen(string);
		}
#else
		*entry = 0;
#endif

		// left side
		if (i < total[0]) {
			cl = &game.clients[sorted[0][i]];
			cl_ent = g_edicts + 1 + sorted[0][i];

			sprintf(entry+strlen(entry),
				"ctf 0 %d %d %d %d ",
				42 + i * 8,
				sorted[0][i],
				cl->resp.score,
				cl->ping > 999 ? 999 : cl->ping);

			if (maxsize - len > strlen(entry)) {
				strcat(string, entry);
				len = strlen(string);
				last[0] = i;
			}
		}

		// right side
		if (i < total[1]) {
			cl = &game.clients[sorted[1][i]];
			cl_ent = g_edicts + 1 + sorted[1][i];

			sprintf(entry+strlen(entry),
				"ctf 160 %d %d %d %d ",
				42 + i * 8,
				sorted[1][i],
				cl->resp.score,
				cl->ping > 999 ? 999 : cl->ping);

			if (maxsize - len > strlen(entry)) {
				strcat(string, entry);
				len = strlen(string);
				last[1] = i;
			}
		}
	}

	// put in spectators if we have enough room
	if (last[0] > last[1])
		j = last[0];
	else
		j = last[1];
	j = (j + 2) * 8 + 42;

	k = n = 0;
	if (maxsize - len > 50) {
		for (i = 0; i < maxclients->value; i++) {
			cl_ent = g_edicts + 1 + i;
			cl = &game.clients[i];
			if (!cl_ent->inuse ||
				cl_ent->solid != SOLID_NOT ||
				cl_ent->client->resp.ctf_team != CTF_NOTEAM)
				continue;

			if (!k) {
				k = 1;
				sprintf(entry, "xv 0 yv %d string2 \"Spectators\" ", j);
				strcat(string, entry);
				len = strlen(string);
				j += 8;
			}

			sprintf(entry+strlen(entry),
				"ctf %d %d %d %d %d ",
				(n & 1) ? 160 : 0, // x
				j, // y
				i, // playernum
				cl->resp.score,
				cl->ping > 999 ? 999 : cl->ping);
			if (maxsize - len > strlen(entry)) {
				strcat(string, entry);
				len = strlen(string);
			}
			
			if (n & 1)
				j += 8;
			n++;
		}
	}

	if (total[0] - last[0] > 1) // couldn't fit everyone
		sprintf(string + strlen(string), "xv 8 yv %d string \"..and %d more\" ",
			42 + (last[0]+1)*8, total[0] - last[0] - 1);
	if (total[1] - last[1] > 1) // couldn't fit everyone
		sprintf(string + strlen(string), "xv 168 yv %d string \"..and %d more\" ",
			42 + (last[1]+1)*8, total[1] - last[1] - 1);

	//gi.WriteByte (svc_layout);
	//gi.WriteString (string);
}

/*-----------------------------------------------------------------------*/
/*QUAKED misc_ctf_banner (1 .5 0) (-4 -64 0) (4 64 248) TEAM2
The origin is the bottom of the banner.
The banner is 248 tall.
*/
static void misc_ctf_banner_think (edict_t *ent)
{
	ent->s.frame = (ent->s.frame + 1) % 16;
	ent->nextthink = level.time + FRAMETIME;
}

void SP_misc_ctf_banner (edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex ("models/ctf/banner/tris.md2");
	if (ent->spawnflags & 1) // team2
		ent->s.skinnum = 1;

	ent->s.frame = rand() % 16;
	gi.linkentity (ent);

	ent->think = misc_ctf_banner_think;
	ent->nextthink = level.time + FRAMETIME;
}

/*QUAKED misc_ctf_small_banner (1 .5 0) (-4 -32 0) (4 32 124) TEAM2
The origin is the bottom of the banner.
The banner is 124 tall.
*/
void SP_misc_ctf_small_banner (edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex ("models/ctf/banner/small.md2");
	if (ent->spawnflags & 1) // team2
		ent->s.skinnum = 1;

	ent->s.frame = rand() % 16;
	gi.linkentity (ent);

	ent->think = misc_ctf_banner_think;
	ent->nextthink = level.time + FRAMETIME;
}


/*-----------------------------------------------------------------------*/

void CTFJoinTeam(edict_t *ent, int desired_team)
{
	char *s;

	PMenu_Close(ent);

	ent->svflags &= ~SVF_NOCLIENT;
	ent->client->resp.ctf_team = desired_team;
	ent->client->resp.ctf_state = CTF_STATE_START;
	s = Info_ValueForKey (ent->client->pers.userinfo, "skin");
	CTFAssignSkin(ent, s);

	PutClientInServer (ent);
	// add a teleportation effect
	ent->s.event = EV_PLAYER_TELEPORT;
	// hold in place briefly
	ent->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	ent->client->ps.pmove.pm_time = 14;
	gi.bprintf(PRINT_HIGH, "%s joined the %s team.\n",
		ent->client->pers.netname, CTFTeamName(desired_team));
}

void CTFJoinTeam1(edict_t *ent, pmenu_t *p)
{
	CTFJoinTeam(ent, CTF_TEAM1);
}

void CTFJoinTeam2(edict_t *ent, pmenu_t *p)
{
	CTFJoinTeam(ent, CTF_TEAM2);
}

void CTFChaseCam(edict_t *ent, pmenu_t *p)
{
	int i, numspec;
	edict_t *e;

	if (ent->client->chase_target) {
		ent->client->chase_target = NULL;
		PMenu_Close(ent);
		return;
	}

	// count spectators
	for (i = 1, numspec = 0; i <= maxclients->value; i++)
		if (g_edicts[i].inuse && g_edicts[i].client->chase_target != NULL)
			numspec++;

	if (numspec >= maxspectators->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "Server spectator limit is full.");
		return;
	}

	// pick someone to chase
	for (i = 1; i <= maxclients->value; i++)
	{
		e = g_edicts + i;
		if (e->inuse && e != ent && e->solid != SOLID_NOT)
		{
			ent->client->chase_target = e;
			TeamplayMakeObserver (ent);
			ent->client->resp.ctf_team = CTF_NOTEAM;
			PMenu_Close(ent);
			ent->client->update_chase = true;
			break;
		}
	}
}

void CTFReturnToMain(edict_t *ent, pmenu_t *p)
{
	PMenu_Close(ent);
	CTFOpenJoinMenu(ent);
}

void CTFCredits(edict_t *ent, pmenu_t *p);

void DeathmatchScoreboard (edict_t *ent);

void CTFShowScores(edict_t *ent, pmenu_t *p)
{
	PMenu_Close(ent);

	ent->client->showscores = true;
	ent->client->showinventory = false;
	DeathmatchScoreboard (ent);
}

pmenu_t creditsmenu[] = {
	{ "*Weapons of Destruction:",		PMENU_ALIGN_CENTER, NULL, NULL },
	{ "*EXTINCTION",						PMENU_ALIGN_CENTER, NULL, NULL },
	{ NULL,									PMENU_ALIGN_CENTER, NULL, NULL },
	{ "*Instructions:",					PMENU_ALIGN_CENTER, NULL, NULL }, 
	{ "Capture all the spawnpoints",	PMENU_ALIGN_LEFT, NULL, NULL },
	{ "for your team, keep the",		PMENU_ALIGN_LEFT, NULL, NULL },
	{ "other team from recapturing",	PMENU_ALIGN_LEFT, NULL, NULL },
	{ "them...then when they can't",	PMENU_ALIGN_LEFT, NULL, NULL },
	{ "respawn, proceed to kill",		PMENU_ALIGN_LEFT, NULL, NULL },
	{ "everyone on the other team",	PMENU_ALIGN_LEFT, NULL, NULL },
	{ "and thus drive them to",		PMENU_ALIGN_LEFT, NULL, NULL },
	{ "extinction.",						PMENU_ALIGN_LEFT, NULL, NULL },
	{ NULL,									PMENU_ALIGN_CENTER, NULL, NULL },
	{ "Each map is played twice;",	PMENU_ALIGN_LEFT, NULL, NULL },
	{ "teams switch sides between",	PMENU_ALIGN_LEFT, NULL, NULL },
	{ "rounds.",							PMENU_ALIGN_LEFT, NULL, NULL },
	{ NULL,									PMENU_ALIGN_CENTER, NULL, NULL },
	{ "Return to Main Menu",			PMENU_ALIGN_LEFT, NULL, CTFReturnToMain }
};


pmenu_t joinmenu[] = {
	{ "*Weapons of Destruction:",		PMENU_ALIGN_CENTER, NULL, NULL },
	{ "*EXTINCTION",						PMENU_ALIGN_CENTER, NULL, NULL },
	{ NULL,									PMENU_ALIGN_CENTER, NULL, NULL },
	{ NULL,									PMENU_ALIGN_CENTER, NULL, NULL },
	{ "Join Red Team",					PMENU_ALIGN_LEFT, NULL, CTFJoinTeam1 },
	{ NULL,									PMENU_ALIGN_LEFT, NULL, NULL },
	{ "Join Blue Team",					PMENU_ALIGN_LEFT, NULL, CTFJoinTeam2 },
	{ NULL,									PMENU_ALIGN_LEFT, NULL, NULL },
	{ "Chase Camera",						PMENU_ALIGN_LEFT, NULL, CTFChaseCam },
	{ "Player ID",							PMENU_ALIGN_LEFT, NULL, CTFID },
	{ "Instructions",						PMENU_ALIGN_LEFT, NULL, CTFCredits },
	{ NULL,									PMENU_ALIGN_LEFT, NULL, NULL },
	{ "Use [ and ] to move cursor",	PMENU_ALIGN_LEFT, NULL, NULL },
	{ "ENTER to select",					PMENU_ALIGN_LEFT, NULL, NULL },
	{ "ESC to Exit Menu",				PMENU_ALIGN_LEFT, NULL, NULL },
	{ "(TAB to Return)",					PMENU_ALIGN_LEFT, NULL, NULL },
};

int CTFUpdateJoinMenu(edict_t *ent)
{
	static char levelname[32];
	static char team1players[32];
	static char team2players[32];
	int num1, num2, i;

	if (ctf->value)
	{
		joinmenu[1].text = "*EXTINCTION";
		joinmenu[10].text = "Instructions";
		joinmenu[10].SelectFunc = CTFCredits;
	}
	else
	{
		joinmenu[1].text = "*Teamplay DM";
		joinmenu[10].text = NULL;
		joinmenu[10].SelectFunc = NULL;
	}

	joinmenu[4].text = "Join Red Team";
	joinmenu[4].SelectFunc = CTFJoinTeam1;
	joinmenu[6].text = "Join Blue Team";
	joinmenu[6].SelectFunc = CTFJoinTeam2;

	if (ctf_forcejoin->string && *ctf_forcejoin->string)
	{
		if (Q_stricmp(ctf_forcejoin->string, "red") == 0)
		{
			joinmenu[6].text = NULL;
			joinmenu[6].SelectFunc = NULL;
		}
		else if (Q_stricmp(ctf_forcejoin->string, "blue") == 0)
		{
			joinmenu[4].text = NULL;
			joinmenu[4].SelectFunc = NULL;
		}
	}

	if (ent->client->chase_target)
		joinmenu[8].text = "Leave Chase Camera";
	else
		joinmenu[8].text = "Chase Camera";

	if (ent->client->resp.id_state)
		joinmenu[9].text = "Turn Player ID off";
	else
		joinmenu[9].text = "Turn Player ID on";

	levelname[0] = '*';
	if (g_edicts[0].message)
		strncpy(levelname+1, g_edicts[0].message, sizeof(levelname) - 2);
	else
		strncpy(levelname+1, level.mapname, sizeof(levelname) - 2);
	levelname[sizeof(levelname) - 1] = 0;

	num1 = num2 = 0;
	for (i = 0; i < maxclients->value; i++) {
		if (!g_edicts[i+1].inuse)
			continue;
		if (game.clients[i].resp.ctf_team == CTF_TEAM1)
			num1++;
		else if (game.clients[i].resp.ctf_team == CTF_TEAM2)
			num2++;
	}

	sprintf(team1players, "  (%d players)", num1);
	sprintf(team2players, "  (%d players)", num2);

	joinmenu[2].text = levelname;
	if (joinmenu[4].text)
		joinmenu[5].text = team1players;
	else
		joinmenu[5].text = NULL;
	if (joinmenu[6].text)
		joinmenu[7].text = team2players;
	else
		joinmenu[7].text = NULL;
	
	if (num1 > num2)
		return CTF_TEAM1;
	else if (num2 > num1)
		return CTF_TEAM1;
	return (rand() & 1) ? CTF_TEAM1 : CTF_TEAM2;
}

void CTFOpenJoinMenu(edict_t *ent)
{
	int team;

	team = CTFUpdateJoinMenu(ent);
	if (ent->client->chase_target)
		team = 8;
	else if (team == CTF_TEAM1)
		team = 4;
	else
		team = 6;
	PMenu_Open(ent, joinmenu, team, sizeof(joinmenu) / sizeof(pmenu_t));
}

void CTFCredits(edict_t *ent, pmenu_t *p)
{
	PMenu_Close(ent);
	PMenu_Open(ent, creditsmenu, -1, sizeof(creditsmenu) / sizeof(pmenu_t));
}

void TeamplayMakeObserver (edict_t *ent)
{
	ent->movetype = MOVETYPE_NOCLIP;
	//ent->deadflag = DEAD_DEAD;
	//ent->health = 0;
	ent->solid = SOLID_NOT;
	ent->svflags |= SVF_NOCLIENT;
	ent->client->ps.pmove.pm_type = PM_NORMAL;
	ent->client->ps.gunindex = 0;
	gi.linkentity (ent);
}

qboolean CTFStartClient(edict_t *ent)
{
	if (ent->client->resp.ctf_team != CTF_NOTEAM)
		return false;

	if (!((int)dmflags->value & DF_CTF_FORCEJOIN))
	{
		// start as 'observer'
		TeamplayMakeObserver (ent);
		ent->client->resp.ctf_team = CTF_NOTEAM;

		// show them the initial menu
		CTFOpenJoinMenu(ent);

		return true;
	}
	return false;
}

char *CTFCheckRules (void)
{
	int live1, live2, i;

	// No winners until the match starts.
	if (level.framenum < ctfgame.matchStartFrame)
		return NULL;

	// If one team doesn't have all the spawnpoints, the game can't be over yet.
	if (ctfgame.spawn1 != ctfgame.spawncount
	&& ctfgame.spawn2 != ctfgame.spawncount)
		return NULL;

	// Count up the live players on each team.
	live1 = live2 = 0;
	for (i = 1; i <= maxclients->value; i++)
	{
		edict_t *ent;

		// Fetch the player.
		ent = &g_edicts[i];
		if (!ent->inuse)
			continue;
		if (!ent->client)
			continue;

		// If this player is live, count them.
		if (!(ent->movetype == MOVETYPE_NOCLIP && ent->solid == SOLID_NOT)
		&& ent->deadflag == DEAD_NO)
		{
			if (ent->client->resp.ctf_team == CTF_TEAM1)
				live1++;
			else if (ent->client->resp.ctf_team == CTF_TEAM2)
				live2++;
		}
	}

	// If one team has no live players, and the other team has all the
	// spawnpoints, the game is over.  (We have to double-check the winner
	// using spawnpoints, since the last two players may kill each other
	// at the same time.)
	if (live1 == 0 && ctfgame.spawn2 == ctfgame.spawncount)
		return CTFTeamName (CTF_TEAM2);
	else if (live2 == 0 && ctfgame.spawn1 == ctfgame.spawncount)
		return CTFTeamName (CTF_TEAM1);

	// Otherwise, keep playing.
	else
		return NULL;
}

/*--------------------------------------------------------------------------
 * just here to help old map conversions
 *--------------------------------------------------------------------------*/

static void old_teleporter_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	edict_t		*dest;
	int			i;
	vec3_t		forward;

	if (!other->client)
		return;
	dest = G_Find (NULL, FOFS(targetname), self->target);
	if (!dest)
	{
		gi.dprintf ("Couldn't find destination\n");
		return;
	}

//ZOID
	DropHook (other);
//ZOID

	// unlink to make sure it can't possibly interfere with KillBox
	gi.unlinkentity (other);

	VectorCopy (dest->s.origin, other->s.origin);
	VectorCopy (dest->s.origin, other->s.old_origin);
//	other->s.origin[2] += 10;

	// clear the velocity and hold them in place briefly
	VectorClear (other->velocity);
	other->client->ps.pmove.pm_time = 160>>3;		// hold time
	other->client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;

	// draw the teleport splash at source and on the player
	self->enemy->s.event = EV_PLAYER_TELEPORT;
	other->s.event = EV_PLAYER_TELEPORT;

	// set angles
	for (i=0 ; i<3 ; i++)
		other->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(dest->s.angles[i] - other->client->resp.cmd_angles[i]);

	other->s.angles[PITCH] = 0;
	other->s.angles[YAW] = dest->s.angles[YAW];
	other->s.angles[ROLL] = 0;
	VectorCopy (dest->s.angles, other->client->ps.viewangles);
	VectorCopy (dest->s.angles, other->client->v_angle);

	// give a little forward velocity
	AngleVectors (other->client->v_angle, forward, NULL, NULL);
	VectorScale(forward, 200, other->velocity);

	// kill anything at the destination
	if (!KillBox (other))
	{
	}

	gi.linkentity (other);
}

/*QUAKED trigger_teleport (0.5 0.5 0.5) ?
Players touching this will be teleported
*/
void SP_trigger_teleport (edict_t *ent)
{
	edict_t *s;
	int i;

	if (!ent->target)
	{
		gi.dprintf ("teleporter without a target.\n");
		G_FreeEdict (ent);
		return;
	}

	ent->svflags |= SVF_NOCLIENT;
	ent->solid = SOLID_TRIGGER;
	ent->touch = old_teleporter_touch;
	gi.setmodel (ent, ent->model);
	gi.linkentity (ent);

	// noise maker and splash effect dude
	s = G_Spawn();
	ent->enemy = s;
	for (i = 0; i < 3; i++)
		s->s.origin[i] = ent->mins[i] + (ent->maxs[i] - ent->mins[i])/2;
	s->s.sound = gi.soundindex ("world/hum1.wav");
	gi.linkentity(s);
	
}

/*QUAKED info_teleport_destination (0.5 0.5 0.5) (-16 -16 -24) (16 16 32)
Point trigger_teleports at these.
*/
void SP_info_teleport_destination (edict_t *ent)
{
	ent->s.origin[2] += 16;
}


/*-----------------------------------------------------------------------*/



// Finish setting up a deathmatch spawnpoint for teamplay.
void TeamplaySetupSpawnpoint (edict_t *spot)
{
	// That's one more spawnpoint to fight over.
	ctfgame.spawncount++;

	// Set it up so that when a player touches it, it changes to that team.
	spot->touch = TeamplaySpawnpointTouch;

	// The spawnpoint's team is determined by the color it glows.  Green means
	// it belongs to no team.
	spot->s.effects |= EF_COLOR_SHELL;
	spot->s.renderfx |= RF_SHELL_GREEN;

	gi.linkentity (spot);
}

// Deal with players touching spawnpoints during teamplay.
void TeamplaySpawnpointTouch (edict_t *self, edict_t *other,
										cplane_t *plane, csurface_t *surf)
{
	qboolean captured;
		// Whether this spot was captured from the other team.

	// We only care about players touching us.
	if (!other || !other->client)
		return;

	// If they don't belong to a team, ignore them.  (Can this happen?)
	if (other->client->resp.ctf_team != CTF_TEAM1
	&& other->client->resp.ctf_team != CTF_TEAM2)
		return;

	// If the spawnpoint already belongs to this team, we're done.
	if ((other->client->resp.ctf_team == CTF_TEAM1
		&& (self->s.renderfx & RF_SHELL_RED))
	|| (other->client->resp.ctf_team == CTF_TEAM2
		&& (self->s.renderfx & RF_SHELL_BLUE)))
	{
		return;
	}

	// A spawnpoint has been claimed for the player's team.

	// Who did the spawnpoint belong to before?
	if (self->s.renderfx & RF_SHELL_GREEN)
	{
		// This is the first time this spawnpoint has been touched.
		self->s.renderfx &= ~RF_SHELL_GREEN;
		captured = false;
	}
	else
	{
		// This spawnpoint just got captured from the other team.
		self->s.renderfx &= ~(RF_SHELL_RED|RF_SHELL_BLUE);
		captured = true;
	}

	// Who does it belong to now?
	if (other->client->resp.ctf_team == CTF_TEAM1)
	{
		// The red team just captured it.
		ctfgame.spawn1++;
		if (captured)
			ctfgame.spawn2--;
		self->s.renderfx |= RF_SHELL_RED;
	}
	else
	{
		// The blue team just captured it.
		ctfgame.spawn2++;
		if (captured)
			ctfgame.spawn1--;
		self->s.renderfx |= RF_SHELL_BLUE;
	}

	// Keep the capturing player from being telefragged by teammates spawning
	// into the game.
	other->client->respawn_time = level.time;

	// Display the changed spawnpoint.
	gi.linkentity (self);

#ifdef EXT_DEVT
	// HACK to use when I'm editing maps for Extinction.
	if (sv_cheats->value)
		gi.cprintf (other, PRINT_HIGH, "dmspot at [%i,%i,%i]\n",
			(int)self->s.origin[0], (int)self->s.origin[1],
			(int)self->s.origin[2]);
#endif // EXT_DEVT
}


// See if any ghost-players can be respawned into the game.
void
TeamplayCheckRespawn (void)
{
	qboolean canspawn1, canspawn2;
		// Whether members of each team can be respawned into the game.
	edict_t *spawn1, *spawn2;
		// A ghost player from each team that we'll respawn in this pass.
	int i;
		// Used to loop through clients.

	// Don't respawn players if the game hasn't started yet.
	if (ctfgame.matchStartFrame)
		return;

	// No ghost-players to respawn...yet.
	spawn1 = spawn2 = NULL;

	// Start by allowing both teams to be respawned.
	canspawn1 = true;
	canspawn2 = true;

	// If a team has no spawnpoints, don't let them spawn.
	if (ctfgame.spawn1 == 0)
		canspawn1 = false;
	if (ctfgame.spawn2 == 0)
		canspawn2 = false;

	// If there's a delay before we can spawn, don't let them spawn.
	if (level.time < ctfgame.checkRespawnTime1)
		canspawn1 = false;
	if (level.time < ctfgame.checkRespawnTime2)
		canspawn2 = false;

	// If we can't spawn anyone from either team, just leave now.
	if (!canspawn1 && !canspawn2)
		return;

	// Now loop through the clients and find ghost-players to respawn.
	for (i = 1; i <= maxclients->value; i++)
	{
		edict_t *ent;

		// Fetch the player.
		ent = &g_edicts[i];
		if (!ent->inuse)
			continue;
		if (!ent->client)
			continue;

		// We're only interested in ghost players.
		if (ent->movetype != MOVETYPE_NOCLIP)
			continue;
		if (ent->solid != SOLID_NOT)
			continue;

		// If it's not time for them to respawn, skip them.
		if (level.time < ent->client->respawn_time)
			continue;

		// If they've been waiting the longest to respawn, give them priority.
		if (ent->client->resp.ctf_team == CTF_TEAM1 && canspawn1)
		{
			if (!spawn1
			|| spawn1->client->respawn_time > ent->client->respawn_time)
				spawn1 = ent;
		}
		else if (ent->client->resp.ctf_team == CTF_TEAM2 && canspawn2)
		{
			if (!spawn2
			|| spawn2->client->respawn_time > ent->client->respawn_time)
				spawn2 = ent;
		}
	}

	// Try to spawn these players live.
	{
		edict_t *spot;
		qboolean spawnlive;

		if (canspawn1 && spawn1)
		{
			spot = SelectCTFSpawnPoint (spawn1, &spawnlive);
			if (spawnlive)
			{
				ctfgame.spawnHere = spot;
				respawn (spawn1);
			}
		}
		if (canspawn2 && spawn2)
		{
			spot = SelectCTFSpawnPoint (spawn2, &spawnlive);
			if (spawnlive)
			{
				ctfgame.spawnHere = spot;
				respawn (spawn2);
			}
		}
	}
}



// Returns true if we're counting down to play.
qboolean TeamplayCheckCountdown (void)
{
	return (ctf->value && ctfgame.matchStartFrame != 0);
}



// Play the countdown sound, and spawn players into the game (when the time
// comes).
void TeamplayDoCountdown (void)
{
	// Doing a countdown?
	if (ctfgame.matchStartFrame)
	{
		// 10 seconds remaining?
		if (level.framenum == ctfgame.matchStartFrame - 11*10)
		{
			// Play 10 to 1 countdown file.
			gi.sound (g_edicts,
				CHAN_RELIABLE+CHAN_AUTO+CHAN_NO_PHS_ADD,
				gi.soundindex ("world/10_0.wav"), 1, ATTN_NONE, 0);
		}
		else if (level.framenum == ctfgame.matchStartFrame)
		{
			int i;

			// Time to fight!
			
			// Reset the score for round 2.  (They were able to see the score
			// during the intermission.)
			if (TeamplayCheckRound2())
				ctfgame.spawn1 = ctfgame.spawn2 = 0;
			
			// Put all team members into the game.
			for (i = 1; i <= maxclients->value; i++)
			{
				edict_t *ent;

				// Fetch the team player.
				ent = g_edicts + i;
				if (!ent->inuse)
					continue;
				if (!ent->client)
					continue;
				if (ent->client->resp.ctf_team != CTF_TEAM1
				&& ent->client->resp.ctf_team != CTF_TEAM2)
					continue;

				// Spawn them into the game.
				respawn (ent);
			}

			// No more countdown.
			ctfgame.matchStartFrame = 0;
		}
	}
}



// Return whether we're in round 1.
qboolean TeamplayCheckRound1 (void)
{
	return (ctf->value && ctfgame.round == 1);
}



// Return whether we're in round 2.
qboolean TeamplayCheckRound2 (void)
{
	return (ctf->value && ctfgame.round == 2);
}



// Start round 2.
void TeamplayStartRound2 (void)
{
	edict_t *ent, *spot;
	int i;

	// Calculate the score so far.
	CTFCalcScores();

	// Now it's round 2.
	ctfgame.round = 2;

	// Wait 15 seconds before starting round 2.
	ctfgame.matchStartFrame = level.framenum + 150;

	// Remember the time from this level.
	if (timelimit->value)
		level.roundTimelimit = level.time + 15;

	// Remember the max frags from this level.
	if (fraglimit->value)
	{
		gclient_t *cl;

		for (i = 0; i < maxclients->value; i++)
		{
			cl = game.clients + i;
			if (!g_edicts[i+1].inuse)
				continue;

			if (level.roundFraglimit < cl->resp.score)
				level.roundFraglimit = cl->resp.score;
		}
	}

	// Reset the spawnpoints.  (The score doesn't get reset until the beginning
	// of round 2.)
	ent = NULL;
	while ((ent = G_Find (ent, FOFS (classname), "info_player_deathmatch"))
		!= NULL)
	{
		ent->s.renderfx &= ~(RF_SHELL_RED|RF_SHELL_BLUE);
		ent->s.renderfx |= RF_SHELL_GREEN;
	}

	// Find an intermission spot.
	spot = G_Find (NULL, FOFS(classname), "info_player_intermission");
	if (!spot)
	{	// the map creator forgot to put in an intermission point...
		spot = G_Find (NULL, FOFS(classname), "info_player_start");
		if (!spot)
			spot = G_Find (NULL, FOFS(classname), "info_player_deathmatch");
	}
	else
	{	// chose one of four spots
		i = rand() & 3;
		while (i--)
		{
			spot = G_Find (spot, FOFS(classname), "info_player_intermission");
			if (!spot)	// wrap around the list
				spot = G_Find (spot, FOFS(classname), "info_player_intermission");
		}
	}

	// Move all clients to intermission.
	for (i = 0; i < maxclients->value; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse)
			continue;

		// Make them a spectator at the intermission point.
		VectorCopy (spot->s.origin, ent->s.origin);
		VectorCopy (spot->s.angles, ent->s.angles);
		TeamplayMakeObserver (ent);

		// Show them the scoreboard.
		ent->client->showscores = true;
		DeathmatchScoreboardMessage (ent, NULL);
		gi.unicast (ent, true);
	}
}



// Rebalance the teams.
static int __cdecl CompareEntitiesByScore (const void *a, const void *b)
{
	edict_t *ea, *eb;

	// a and b point inside of sorted[], defined in TeamplayRebalanceTeams().
	// Convert the indexes back into entities.
	ea = g_edicts + (*(int *)a) + 1;
	eb = g_edicts + (*(int *)b) + 1;

	// Compare their scores so that qsort() arranges the list by decreasing
	// score.
	return eb->client->resp.score - ea->client->resp.score;
}
void TeamplayRebalanceTeams (void)
{
	float total1, total2;
		// The sum of the frag-rates of each new team.
	int sorted[MAX_CLIENTS];
		// The indexes of the clients, sorted by decreasing score.
	int i, count;
	edict_t *ent;
		// Loop/index variables.

	// Make a list of all the clients.
	count = 0;
	for (i = 0; i < maxclients->value; i++)
	{
		// Get the next team player.
		ent = g_edicts + i + 1;
		if (!ent->inuse)
			continue;
		if (ent->client->resp.ctf_team != CTF_TEAM1
		&& ent->client->resp.ctf_team != CTF_TEAM2)
			continue;

		// Store their index.
		sorted[count] = i;
		count++;
	}

	// Sort the clients by decreasing score.
	qsort (sorted, count, sizeof (sorted[0]), CompareEntitiesByScore);

	// Now examine the clients in this order, calculate each client's fragrate,
	// and put them in the team that has the lowest total fragrate so far.
	total1 = total2 = 0;
	for (i = 0; i < count; i++)
	{
		float minutes;
			// The number of minutes the player has been in the game.
		float fragrate;
			// The current player's frags-per-minute.

		// Get the next player.
		ent = g_edicts + sorted[i] + 1;

		// Calculate this player's fragrate.  (I picked frags-per-minute for no
		// particular reason.  It could have been frags-per-frame.  I just thought
		// I'd get less roundoff error this way, or maybe someday one will want to
		// display the total frags-per-minute of each team, or something.)
		minutes = (level.framenum - ent->client->resp.enterframe) / 600.0;
		fragrate = ent->client->resp.score / minutes;

		// Put them on the team with the lowest total fragrate.
		if (total1 < total2)
		{
			// Put them on the red team.
			ent->client->resp.ctf_team = CTF_TEAM1;
			total1 += fragrate;
		}
		else
		{
			// Put them on the blue team.
			ent->client->resp.ctf_team = CTF_TEAM2;
			total2 += fragrate;
		}
	}
}
