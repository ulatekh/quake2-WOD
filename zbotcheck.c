/*============================================================================
ZbotCheck v1.01 for Quake 2 by Matt "WhiteFang" Ayres (matt@lithium.com)

This is provided for mod authors to implement Zbot detection, nothing more.
The code has so far proven to be reliable at detecting Zbot auto-aim clients
(cheaters).  However, no guarantees of any kind are made.  This is provided
as-is.  You must be familiar with Quake 2 mod coding to make use of this.

In g_local.h, add to struct client_respawn_t:

	short angles[2][2];
	int tog;
	int jitter;				// The number of jitters in the series
	float jitter_time;	// The time of the first jitter in the series
	float jitter_last;	// The time of the last jitter in the series

Next, in p_client.c, add a simple forward declaration:

	qboolean ZbotCheck(edict_t *ent, usercmd_t *ucmd);

Then in p_client.c, anywhere in the ClientThink function, call the
ZbotCheck function.  Pass it the same parameters you get from ClientThink.
It will return true if the client is using a Zbot.  Simple example:

	if(ZbotCheck(ent, ucmd))
		gi.bprintf(PRINT_HIGH, ">>> Zbot detected: %s\n",
		ent->client->pers.netname);

From here you can do as you please with the cheater.  ZbotCheck will only
return true once, following returns will be false.
============================================================================*/

#include "g_local.h"
#include "zbotcheck.h"



#ifdef BOT_HARD_EVIDENCE

static void RecordPlayerMovement (edict_t *ent, bot_record_t br);

#endif BOT_HARD_EVIDENCE



// Calculate the distance between a point and a line.
static double
DistanceFromPointToLine (vec3_t point, vec3_t linePoint, vec3_t lineDir)
{
	vec3_t v1, v2, v3;

	VectorSubtract (point, linePoint, v1);
	CrossProduct (v1, lineDir, v2);
	CrossProduct (v2, lineDir, v3);
	VectorNormalize (v3);
	return fabs (DotProduct (v1, v3));
}



bot_record_t
GetBotRecord (edict_t *ent, usercmd_t *ucmd)
{
	bot_record_t br;

	// Record the next bit of player movement.
	br.fire = (ucmd->buttons & BUTTON_ATTACK) ? 1 : 0;
	br.pitch = ucmd->angles[0];
	br.yaw = ucmd->angles[1];
	br.forwardmove = ucmd->forwardmove;
	br.sidemove = ucmd->sidemove;
	br.upmove = ucmd->upmove;
	br.impulse = ucmd->impulse;

	// Examine the aiming pattern.
	if (br.fire)
	{
		vec3_t offset, forward, right, start, end;
		trace_t tr;

		// Figure out where they're aiming.
		VectorSet (offset, 0, 0, ent->viewheight - 8);
		AngleVectors (ent->client->v_angle, forward, right, NULL);
		P_ProjectSource (ent->client, ent->s.origin, offset, forward, right,
			start);
		VectorMA (start, 8192, forward, end);

		// Shoot there.
		tr = gi.trace (start, NULL, NULL, end, ent, MASK_SHOT);
		if (!tr.ent || !tr.ent->client)
		{
			br.hitClient = 0;
			br.clientDist = -1;
		}
		else
		{
			// We hit somebody.
			br.hitClient = 1;
			br.clientDist = DistanceFromPointToLine (tr.ent->s.origin,
				start, forward);
		}
	}

	// Return the record we just put together.
	return br;
}



#define ZBOT_JITTERMAX	3
#define ZBOT_JITTERTIME	20
#define ZBOT_NOJITTERMOVE 100
#define ZBOT_JITTERMOVE 500

static qboolean
ZbotCheckLithium (edict_t *ent, usercmd_t *ucmd)
{
	bot_record_t *br0, *br1, *br2;

	// Get the information we need.
	br0 = &ent->client->botRecord[0];
	br1 = &ent->client->botRecord[1];
	br2 = &ent->client->botRecord[2];

	// Are they jittering in a zbot-like way?
	if (abs (br0->pitch - br2->pitch) + abs (br0->yaw - br2->yaw)
		<= ZBOT_NOJITTERMOVE
	&& abs (br0->pitch - br1->pitch) + abs (br0->yaw - br1->yaw)
		>= ZBOT_JITTERMOVE)
	{
		if (ent->client->jitter == 0
		|| level.framenum <= ent->client->jitter_last + 1)
		{
			// Remember the time we received the first jitter.
			if (ent->client->jitter == 0)
				ent->client->jitter_time = level.framenum;

			// If that's too many jitters in the given amount of time, scream.
			ent->client->jitter++;
			if (ent->client->jitter >= ZBOT_JITTERMAX)
				return true;

			// Remember the last time we saw jitter.
			ent->client->jitter_last = level.framenum;
		}
	}

	// If enough time has passed since this series began, start again.
	if (ent->client->jitter
	&& level.framenum > ent->client->jitter_time + ZBOT_JITTERTIME)
		ent->client->jitter = 0;

	return false;
}



#define ZBOT_ALTERNATE_FIRE	10		// How many alternate fire/non-fire
#define ZBOT_MAX_FRAMES			5		// Must occur within # server frames

static qboolean
ZbotCheckLFire (edict_t *ent, usercmd_t *ucmd)
{
	// Not what we're looking for?
	if (ent->client->botRecord[0].fire == ent->client->botRecord[1].fire
	|| (ent->client->botRecord[0].pitch == ent->client->botRecord[1].pitch
		&& ent->client->botRecord[0].yaw == ent->client->botRecord[1].yaw))
	{
		// Reset detection.
		ent->client->zbotSequenceCount = 0;
		return false;
	}

	// First in the sequence?
	if (ent->client->zbotSequenceCount == 0)
		ent->client->zbotDetectFrame = level.framenum;

	// Did we get enough alternate fire/non-fire sequences with *some* movement
	// between them?
	ent->client->zbotSequenceCount++;
	if (ent->client->zbotSequenceCount < ZBOT_ALTERNATE_FIRE)
		return false;

	// We got enough alternate fire/non-fire to be pretty sure this is
	// a zbot doing it.

	// Reset so we can detect another round.
	ent->client->zbotSequenceCount = 0;

	// Make sure it all happened quick enough, so a human couldn't possibly
	// be responsible.
	if (level.framenum - ent->client->zbotDetectFrame > ZBOT_MAX_FRAMES)
		return false;

	// Looks to be a zbot.
	return true;
}



static qboolean
RatBotCheck (edict_t *ent, usercmd_t *ucmd)
{
	gclient_t *client = ent->client;

	// Fetch the data we need to do this check.
	vec_t currentPitch = client->botRecord[0].pitch;
	vec_t currentYaw = client->botRecord[0].yaw;
	qboolean currentFire = client->botRecord[0].fire;
	vec_t previousPitch = client->botRecord[1].pitch;
	vec_t previousYaw = client->botRecord[1].yaw;
	qboolean previousFire = client->botRecord[1].fire;
	vec_t angleSumChange = fabs (previousPitch - currentPitch)
		+ fabs (previousYaw - currentYaw);

	// What we do depends on where we are in our RatBot check.
	if (client->ratBotState == 0)
	{
		// If they just started firing, and the angle-sum-change is 1000 or
		// greater, this may be the beginning of a RatBot firing sequence.
		if (!previousFire && currentFire
		&& angleSumChange >= 1000)
			client->ratBotState = 1;
	}
	else
	{
		// Are they continuing to fire?
		if (previousFire && currentFire)
		{
			// If the angle-sum-change is 100 or less (but not zero), this may be
			// a RatBot firing sequence.
			if (angleSumChange <= 100 && angleSumChange > 0)
				client->ratBotState++;

			// If not, it still might be, but be more strict about it.
			else
				client->ratBotState = 1;
		}

		// Have they stopped firing?
		else if (previousFire && !currentFire)
		{
			// If they fired enough times to make a convincing case, and the
			// angle-sum-change is 1000 or greater, we have a detection.
			if (client->ratBotState > 5 && angleSumChange >= 1000)
			{
				client->ratBotState = 0;
				return true;
			}

			// Otherwise, we don't have a convincing case.
			else
				client->ratBotState = 0;
		}

		// Otherwise, we don't have enough data.
		else
			client->ratBotState = 0;
	}

	// No detection.
	return false;
}



static qboolean
SlowRatBotCheck (edict_t *ent, usercmd_t *ucmd)
{
	gclient_t *client = ent->client;
	int i, fire_count;
	bot_record_t *br0, *br1;

	// Locate the current fire frame.
	i = client->nextBotLog - 1;
	if (i < 0)
		i = 49;
	br0 = client->botLog + i;

	// Make sure they're not firing any more.
	if (br0->fire)
		return false;

	// Go back another frame.
	br1 = br0;
	i--; if (i < 0) i = 49;
	br0 = client->botLog + i;

	// Make sure they *are* firing.
	if (!br0->fire)
		return false;

	// Make sure the angle-sum-change is 1000 or greater.
	if (abs (br0->pitch - br1->pitch) + abs (br0->yaw - br1->yaw) < 500)
		return false;

	// Look at the whole string of fire frames, make sure the angle-sum-change
	// between them all is less than 100.
	fire_count = 0;
	for (;;)
	{
		// Go back another frame.
		if (i == client->nextBotLog)
			return false;
		br1 = br0;
		i--; if (i < 0) i = 49;
		br0 = client->botLog + i;

		// If they're no longer firing, stop looking at the string of fire frames.
		if (!br0->fire)
			break;

		// Make sure the angle-sum-change is 100 or less.
		if (abs (br0->pitch - br1->pitch) + abs (br0->yaw - br1->yaw) > 100)
			return false;

		// That's one more fire frame.
		fire_count++;
	}

	// Make sure there were enough fire frames to be convincing.
	if (fire_count < 5)
		return false;

	// Make sure the angle-sum-change is, again, 1000 or greater.
	if (abs (br0->pitch - br1->pitch) + abs (br0->yaw - br1->yaw) < 500)
		return false;

	// Looks like a RatBot to us.
	return true;
}



int
BotCheck (edict_t *ent, usercmd_t *ucmd)
{
	int botCode;
	bot_record_t br;

	// Get the next bit of evidence.
	ent->client->botRecord[2] = ent->client->botRecord[1];
	ent->client->botRecord[1] = ent->client->botRecord[0];
	ent->client->botRecord[0] = br = GetBotRecord (ent, ucmd);

#ifdef BOT_HARD_EVIDENCE
	// Store the next bit of evidence.
	RecordPlayerMovement (ent, br);
#endif BOT_HARD_EVIDENCE

	// No bot detected yet.
	botCode = 0;

	// Look for ZBot impulses.
	if (ucmd->impulse >= 169 && ucmd->impulse <= 174)
		botCode = 1;

	// Use Lithium's ZBot checker.
	if (ZbotCheckLithium (ent, ucmd))
		botCode = 2;

	// Use L-Fire's ZBot checker.
	if (ZbotCheckLFire (ent, ucmd))
		botCode = 3;

	// Look for RatBot impulses.
	if (ucmd->impulse >= 150 && ucmd->impulse <= 153)
		botCode = 4;

	// Look for a RatBot firing pattern.
	if (RatBotCheck (ent, ucmd))
		botCode = 5;

	// Return what happened.
	return botCode;
}



#ifdef BOT_HARD_EVIDENCE

static void
RecordPlayerMovement (edict_t *ent, bot_record_t br)
{
	gclient_t *client = ent->client;

	// Store the bot record.
	client->botLog[client->nextBotLog] = br;

	// Move to the next one.
	client->nextBotLog++;
	if (client->nextBotLog == 50)
		client->nextBotLog = 0;
}



void
LogPlayerMovement (edict_t *ent, int botCode)
{
	gclient_t *client = ent->client;
	const char *botType;
	const char *testType;
	union
	{
		unsigned int i;
		unsigned char c[4];
	} u;
	int i;
	char buffer[MAX_QPATH + 1];
	FILE *log;

	// Determine what kind of bot they are, and what test detected it.
	switch (botCode)
	{
		case 1:
			botType = "ZBot"; testType = "impulse"; break;
		case 2:
			botType = "ZBot"; testType = "Lithium"; break;
		case 3:
			botType = "ZBot"; testType = "Hash"; break;
		case 4:
			botType = "RatBot"; testType = "impulse"; break;
		case 5:
			botType = "RatBot"; testType = "Hash"; break;
		default:
			botType = "UNKNOWN"; testType = "UNKNOWN"; break;
	}

	// Parse their IP address.
	u.i = client->pers.ipAddr;

	// Get ready to append to our bot detection log.
	sprintf (buffer, "./%s/botdetect.log", gamedir->string);
	log = fopen (buffer, "at");

	// Log this bot detection.
	{
		struct tm *newtime;
		long ltime;
		time (&ltime);
		newtime = localtime (&ltime);
		fprintf (log, "Date: %s", asctime (newtime));
	}
	fprintf (log, "Name: \"%s\"\nIP address: %i.%i.%i.%i\n",
		client->pers.netname,
		(int)u.c[0], (int)u.c[1], (int)u.c[2], (int)u.c[3]);
	fprintf (log, "Bot type: %s\nTest type: %s\n", botType, testType);
	fprintf (log, "\n");
	fprintf (log, "Fire    Angles     Forward  Side    Up   Imp ShotError\n");
	fprintf (log, "---- ------------- ------- ------ ------ --- ---------\n");
// fprintf (log, "FIRE -xxxxx,-xxxxx  -xxxxx -xxxxx -xxxxx xxx xx.xxxxxx\n");
	i = client->nextBotLog;
	for (;;)
	{
		char angleBuffer[15];
		char shotErrorBuffer[15];
		const bot_record_t *br = client->botLog + i;

		// Print this record.
		sprintf (angleBuffer, "%i,%i", br->pitch, br->yaw);
		sprintf (shotErrorBuffer, "%9.6lf", br->clientDist);
		fprintf (log, "%4s %13s  %6i %6i %6i %3i %9s\n",
			(br->fire) ? "FIRE" : "",
			angleBuffer,
			br->forwardmove,
			br->sidemove,
			br->upmove,
			br->impulse,
			(br->hitClient) ? shotErrorBuffer : "");

		// Move to the next record.
		i++;
		if (i == 50)
			i = 0;
		if (i == client->nextBotLog)
			break;
 	}
	fprintf (log, "#------------------------------------------------#\n");

	// That's it for the log entry.
	fclose (log);
}

#endif BOT_HARD_EVIDENCE
