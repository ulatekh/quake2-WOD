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

#define ZBOT_JITTERMAX	3
#define ZBOT_JITTERTIME	1
#define ZBOT_NOJITTERMOVE 100
#define ZBOT_JITTERMOVE 500

qboolean
ZbotCheck (edict_t *ent, usercmd_t *ucmd)
{
	int tog0, tog1;
	client_respawn_t *resp = &ent->client->resp;

	// Rotate our interpretation of the stored angles.
	tog0 = resp->tog;
	resp->tog ^= 1;
	tog1 = resp->tog;

	// Are they jittering in a zbot-like way?
	if(abs(ucmd->angles[0] - resp->angles[tog1][0]) +
	   abs(ucmd->angles[1] - resp->angles[tog1][1]) <= ZBOT_NOJITTERMOVE &&
	   abs(ucmd->angles[0] - resp->angles[tog0][0]) +
	   abs(ucmd->angles[1] - resp->angles[tog0][1]) >= ZBOT_JITTERMOVE)
	{
		// Remember the time we received the first jitter.
		if (!resp->jitter)
			resp->jitter_time = level.time;

		// If that's too many jitters in the given amount of time, scream.
		if (++resp->jitter >= ZBOT_JITTERMAX)
			return true;
	}

	// Remember where they're pointing this frame.
	resp->angles[tog1][0] = ucmd->angles[0];
	resp->angles[tog1][1] = ucmd->angles[1];

	// If enough time has passed since this series began, start again.
	if (resp->jitter
	&& level.time > resp->jitter_time + ZBOT_JITTERTIME)
		resp->jitter = 0;

	return false;
}
