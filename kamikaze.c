#include "g_local.h"

void Start_Kamikaze_Mode(edict_t *self)
{
	/* see if we are already in  kamikaze mode*/

	if (self->client->kamikaze_mode & 1)
		return;             

	/* not in kamikaze mode yet */
	self->client->kamikaze_mode = 1;

	/*  Give us only so long */
	self->client->kamikaze_timeleft = KAMIKAZE_BLOW_TIME;
	self->client->kamikaze_framenum = level.framenum
		+ self->client->kamikaze_timeleft;

	/* Warn the world. */
	gi.bprintf (PRINT_MEDIUM, "%s is a kamikaze - BANZAI!!\n",
		self->client->pers.netname);
	gi.sound (self, CHAN_WEAPON, gi.soundindex("makron/rail_up.wav"),
		1, ATTN_NONE, 0 );

	return;
}

qboolean Kamikaze_Active (edict_t *self)
{
	return (self->client->kamikaze_mode);
}

void Kamikaze_Cancel (edict_t *self)
{
	/* Cancel what we started. */
	self->client->kamikaze_mode = 0;
	self->client->kamikaze_timeleft = 0;
	self->client->kamikaze_framenum = 0;

	return;
}

void Kamikaze_Explode (edict_t *self)
{
	int mod;

	// Set up the means of death.
	mod = MOD_KAMIKAZE;
	if ((int)fragban->value & WFB_KAMIKAZE)
		mod |= MOD_NOFRAG;

	// Go out with a bang!
	T_RadiusDamage (self, self, KAMIKAZE_DAMAGE, NULL,
		KAMIKAZE_DAMAGE_RADIUS, mod);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	// If that didn't kill them, die anyway.  (This takes care of any reason
	// they wouldn't die, like an invulnerability powerup, or god mode.)
	if (!self->deadflag)
		player_die (self, self, self, KAMIKAZE_DAMAGE, self->s.origin);
}
