#include "g_local.h"


/*
=================
check_dodge

This is a support routine used when a client is firing
a non-instant attack weapon.  It checks to see if a
monster's dodge function should be called.
=================
*/
static void check_dodge (edict_t *self, vec3_t start, vec3_t dir, int speed)
{
	vec3_t	end;
	vec3_t	v;
	trace_t	tr;
	float	eta;

	// easy mode only ducks one quarter the time
	if (skill->value == 0)
	{
		if (random() > 0.25)
			return;
	}
	VectorMA (start, 8192, dir, end);
	tr = gi.trace (start, NULL, NULL, end, self, MASK_SHOT);
	if ((tr.ent) && (tr.ent->svflags & SVF_MONSTER) && (tr.ent->health > 0) && (tr.ent->monsterinfo.dodge) && infront(tr.ent, self))
	{
		VectorSubtract (tr.endpos, start, v);
		eta = (VectorLength(v) - tr.ent->maxs[0]) / speed;
		tr.ent->monsterinfo.dodge (tr.ent, self, eta);
	}
}


/*
=================
fire_hit

Used for all impact (hit/punch/slash) attacks
=================
*/
qboolean fire_hit (edict_t *self, vec3_t aim, int damage, int kick)
{
	trace_t		tr;
	vec3_t		forward, right, up;
	vec3_t		v;
	vec3_t		point;
	float		range;
	vec3_t		dir;

	//see if enemy is in range
	VectorSubtract (self->enemy->s.origin, self->s.origin, dir);
	range = VectorLength(dir);
	if (range > aim[0])
		return false;

	if (aim[1] > self->mins[0] && aim[1] < self->maxs[0])
	{
		// the hit is straight on so back the range up to the edge of their bbox
		range -= self->enemy->maxs[0];
	}
	else
	{
		// this is a side hit so adjust the "right" value out to the edge of their bbox
		if (aim[1] < 0)
			aim[1] = self->enemy->mins[0];
		else
			aim[1] = self->enemy->maxs[0];
	}

	VectorMA (self->s.origin, range, dir, point);

	tr = gi.trace (self->s.origin, NULL, NULL, point, self, MASK_SHOT);
	if (tr.fraction < 1)
	{
		if (!tr.ent->takedamage)
			return false;
		// if it will hit any client/monster then hit the one we wanted to hit
		if ((tr.ent->svflags & SVF_MONSTER) || (tr.ent->client))
			tr.ent = self->enemy;
	}

	AngleVectors(self->s.angles, forward, right, up);
	VectorMA (self->s.origin, range, forward, point);
	VectorMA (point, aim[1], right, point);
	VectorMA (point, aim[2], up, point);
	VectorSubtract (point, self->enemy->s.origin, dir);

	// do the damage
	T_Damage (tr.ent, self, self, dir, point, vec3_origin, damage, kick/2, DAMAGE_NO_KNOCKBACK, MOD_HIT);

	if (!(tr.ent->svflags & SVF_MONSTER) && (!tr.ent->client))
		return false;

	// do our special form of knockback here
	VectorMA (self->enemy->absmin, 0.5, self->enemy->size, v);
	VectorSubtract (v, point, v);
	VectorNormalize (v);
	VectorMA (self->enemy->velocity, kick, v, self->enemy->velocity);
	if (self->enemy->velocity[2] > 0)
		self->enemy->groundentity = NULL;
	return true;
}


/*
=================
fire_lead

This is an internal support routine used for bullet/pellet based weapons.
=================
*/
static void fire_lead (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int hspread, int vspread, int mod)
{
	trace_t		tr;
	vec3_t		dir;
	vec3_t		forward, right, up;
	vec3_t		end;
	float		r;
	float		u;
	vec3_t		water_start;
	qboolean	water = false;
	int			content_mask = MASK_SHOT | MASK_WATER;

	tr = gi.trace (self->s.origin, NULL, NULL, start, self, MASK_SHOT);
	if (!(tr.fraction < 1.0))
	{
		vectoangles (aimdir, dir);
		AngleVectors (dir, forward, right, up);

		r = crandom()*hspread;
		u = crandom()*vspread;
		VectorMA (start, 8192, forward, end);
		VectorMA (end, r, right, end);
		VectorMA (end, u, up, end);

		if (gi.pointcontents (start) & MASK_WATER)
		{
			water = true;
			VectorCopy (start, water_start);
			content_mask &= ~MASK_WATER;
		}

		tr = gi.trace (start, NULL, NULL, end, self, content_mask);

		// see if we hit water
		if (tr.contents & MASK_WATER)
		{
			int		color;

			water = true;
			VectorCopy (tr.endpos, water_start);

			if (!VectorCompare (start, tr.endpos))
			{
				if (tr.contents & CONTENTS_WATER)
				{
					if (strcmp(tr.surface->name, "*brwater") == 0)
						color = SPLASH_BROWN_WATER;
					else
						color = SPLASH_BLUE_WATER;
				}
				else if (tr.contents & CONTENTS_SLIME)
					color = SPLASH_SLIME;
				else if (tr.contents & CONTENTS_LAVA)
					color = SPLASH_LAVA;
				else
					color = SPLASH_UNKNOWN;

				if (color != SPLASH_UNKNOWN)
				{
					gi.WriteByte (svc_temp_entity);
					gi.WriteByte (TE_SPLASH);
					gi.WriteByte (8);
					gi.WritePosition (tr.endpos);
					gi.WriteDir (tr.plane.normal);
					gi.WriteByte (color);
					gi.multicast (tr.endpos, MULTICAST_PVS);
				}

				// change bullet's course when it enters water
				VectorSubtract (end, start, dir);
				vectoangles (dir, dir);
				AngleVectors (dir, forward, right, up);
				r = crandom()*hspread*2;
				u = crandom()*vspread*2;
				VectorMA (water_start, 8192, forward, end);
				VectorMA (end, r, right, end);
				VectorMA (end, u, up, end);
			}

			// re-trace ignoring water this time
			tr = gi.trace (water_start, NULL, NULL, end, self, MASK_SHOT);
		}
	}

	// send gun puff / flash
	if (!((tr.surface) && (tr.surface->flags & SURF_SKY)))
	{
		if (tr.fraction < 1.0)
		{
			if (tr.ent->takedamage)
			{
				T_Damage (tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, kick, DAMAGE_BULLET, mod);
			}
			else
			{
				if (strncmp (tr.surface->name, "sky", 3) != 0)
				{
					gi.WriteByte (svc_temp_entity);
					gi.WriteByte (te_impact);
					gi.WritePosition (tr.endpos);
					gi.WriteDir (tr.plane.normal);
					gi.multicast (tr.endpos, MULTICAST_PVS);

					if (self->client)
						PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
				}
			}
		}
	}

	// if went through water, determine where the end and make a bubble trail
	if (water)
	{
		vec3_t	pos;

		VectorSubtract (tr.endpos, water_start, dir);
		VectorNormalize (dir);
		VectorMA (tr.endpos, -2, dir, pos);
		if (gi.pointcontents (pos) & MASK_WATER)
			VectorCopy (pos, tr.endpos);
		else
			tr = gi.trace (pos, NULL, NULL, water_start, tr.ent, MASK_WATER);

		VectorAdd (water_start, tr.endpos, pos);
		VectorScale (pos, 0.5, pos);

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BUBBLETRAIL);
		gi.WritePosition (water_start);
		gi.WritePosition (tr.endpos);
		gi.multicast (pos, MULTICAST_PVS);
	}
}


/*
=================
fire_bullet

Fires a single round.  Used for machinegun and chaingun.  Would be fine for
pistols, rifles, etc....
=================
*/
void fire_bullet (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int mod)
{
	fire_lead (self, start, aimdir, damage, kick, TE_GUNSHOT, hspread, vspread, mod);
}


/*
=================
fire_shotgun

Shoots shotgun pellets.  Used by shotgun and super shotgun.
=================
*/
void fire_shotgun (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int count, int mod)
{
	int		i;

	for (i = 0; i < count; i++)
		fire_lead (self, start, aimdir, damage, kick, TE_SHOTGUN, hspread, vspread, mod);
}


/*
=================
fire_blaster

Fires a single blaster bolt.  Used by the blaster and hyper blaster.
=================
*/
void blaster_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == self->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (self);
		return;
	}

	if (self->owner->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	if (other->takedamage)
	{
		int mod;

		// Set up the means of death.
		mod = MOD_BLASTER;
		if ((int)fragban->value & WFB_BLASTER)
			mod |= MOD_NOFRAG;

		T_Damage (other, self, self->owner, self->velocity, self->s.origin,
			plane->normal, self->dmg, 1, DAMAGE_ENERGY, mod);
	}
	else
	{
		// return;
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BLASTER);
		gi.WritePosition (self->s.origin);
		if (!plane)
			gi.WriteDir (vec3_origin);
		else
			gi.WriteDir (plane->normal);
		gi.multicast (self->s.origin, MULTICAST_PVS);
	}

	G_FreeEdict (self);
}

void fire_blaster (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int effect)
{
	edict_t *bolt;
	trace_t tr;
	int laser_colour[] = {
		0xf2f2f0f0,	     // red
		0xf3f3f1f1,	     // blue
	};

	VectorNormalize (dir);

	// Only change hand blaster effect
	if (effect & EF_BLASTER)
	{
		bolt = G_Spawn();
		bolt->svflags = SVF_DEADMONSTER;
		VectorCopy (start, bolt->s.origin);
		vectoangles (dir, bolt->s.angles);
		VectorScale (dir, speed, bolt->velocity);
		VectorAdd (start, bolt->velocity, bolt->s.old_origin);
		bolt->clipmask = MASK_SHOT;

		bolt->movetype = MOVETYPE_FLYMISSILE;
		bolt->solid = SOLID_BBOX;
		bolt->s.renderfx |= RF_BEAM|RF_TRANSLUCENT;
		bolt->s.modelindex = 1;       
		bolt->owner = self;

		bolt->s.frame = 3;

		if (ctf->value && self->client)
		{
			// Team colors for blaster lasers.
			if (self->client->resp.ctf_team == CTF_TEAM1)
				bolt->s.skinnum = laser_colour[0];
			else
				bolt->s.skinnum = laser_colour[1];
		}
		else
			bolt->s.skinnum = laser_colour[((int) (random() * 1500)) % 2];

		VectorClear (bolt->mins);
		VectorClear (bolt->maxs);

		bolt->touch = blaster_touch;
		bolt->nextthink = level.time + 4;
		bolt->think = G_FreeEdict;
		bolt->dmg = damage;

		gi.linkentity (bolt);

		if (self->client)
		check_dodge (self, bolt->s.origin, dir, speed);

		tr = gi.trace (self->s.origin, NULL, NULL, bolt->s.origin, bolt, MASK_SHOT);
		if (tr.fraction < 1.0)
		{
			VectorMA (bolt->s.origin, -10, dir, bolt->s.origin);
			bolt->touch (bolt, tr.ent, NULL, NULL);
		}

		return;
	}

	bolt = G_Spawn();
	bolt->svflags = SVF_DEADMONSTER;
	VectorCopy (start, bolt->s.origin);
	VectorCopy (start, bolt->s.old_origin);
	vectoangles (dir, bolt->s.angles);
	VectorScale (dir, speed, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	bolt->s.effects |= effect;
	VectorClear (bolt->mins);
	VectorClear (bolt->maxs);
	bolt->s.modelindex = gi.modelindex ("models/objects/laser/tris.md2");
	bolt->s.sound = gi.soundindex ("misc/lasfly.wav");
	bolt->owner = self;
	bolt->touch = blaster_touch;
	bolt->nextthink = level.time + 2;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	gi.linkentity (bolt);

	if (self->client)
		check_dodge (self, bolt->s.origin, dir, speed);

	tr = gi.trace (self->s.origin, NULL, NULL, bolt->s.origin, bolt, MASK_SHOT);
	if (tr.fraction < 1.0)
	{
		VectorMA (bolt->s.origin, -10, dir, bolt->s.origin);
		bolt->touch (bolt, tr.ent, NULL, NULL);
	}
}

void sniper_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == self->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (self);
		return;
	}

	if (self->owner->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	if (other->takedamage)
	{
		int mod;

		// Set up the means of death.
		mod = MOD_SNIPER;
		if ((int)fragban->value & WB_SNIPERGUN)
			mod |= MOD_NOFRAG;

		T_Damage (other, self, self->owner, self->velocity, self->s.origin,
			plane->normal, self->dmg, 1, 0, mod);
	}
	else
	{
		// return;
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_SHOTGUN);
		gi.WritePosition (self->s.origin);
		if (!plane)
			gi.WriteDir (vec3_origin);
		else
			gi.WriteDir (plane->normal);
		gi.multicast (self->s.origin, MULTICAST_PVS);
	}

	G_FreeEdict (self);
}

void fire_sniper (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int effect)
{
	edict_t *bolt;
	trace_t tr;


	VectorNormalize (dir);	

	bolt = G_Spawn();
	VectorCopy (start, bolt->s.origin);
	VectorCopy (start, bolt->s.old_origin);
	vectoangles (dir, bolt->s.angles);
	VectorScale (dir, speed, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	bolt->s.effects |= 0;
	VectorClear (bolt->mins);
	VectorClear (bolt->maxs);
	bolt->s.modelindex = gi.modelindex ("models/objects/debris2/tris.md2");
	bolt->s.sound = gi.soundindex ("misc/lasfly.wav");
	bolt->owner = self;
	bolt->touch = sniper_touch;
	bolt->nextthink = level.time + 2;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	gi.linkentity (bolt);

	if (self->client)
		check_dodge (self, bolt->s.origin, dir, speed);

	tr = gi.trace (self->s.origin, NULL, NULL, bolt->s.origin, bolt, MASK_SHOT);
	if (tr.fraction < 1.0)
	{
		VectorMA (bolt->s.origin, -10, dir, bolt->s.origin);
		bolt->touch (bolt, tr.ent, NULL, NULL);
	}
}

// plasma begin
void plasma_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == self->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (self);
		return;
	}

	if (self->owner->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	if (other->takedamage)
	{
		int mod;

		// Set up the means of death.
		mod = MOD_PLASMAGUN;
		if ((int)fragban->value & WB_PLASMARIFLE)
			mod |= MOD_NOFRAG;

		T_Damage (other, self, self->owner, self->velocity, self->s.origin,
			plane->normal, self->dmg, 1, DAMAGE_ENERGY, mod);
	}
	else
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BLASTER);
		gi.WritePosition (self->s.origin);
		if (!plane)
			gi.WriteDir (vec3_origin);
		else
			gi.WriteDir (plane->normal);
		gi.multicast (self->s.origin, MULTICAST_PVS);
	}

	G_FreeEdict (self);
}

void fire_plasma (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int effect)
{
	edict_t *bolt;
	trace_t tr;


	VectorNormalize (dir);	

	bolt = G_Spawn();
	VectorCopy (start, bolt->s.origin);
	VectorCopy (start, bolt->s.old_origin);
	vectoangles (dir, bolt->s.angles);
	VectorScale (dir, speed, bolt->velocity);

	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	//bolt->s.effects |= EF_COLOR_SHELL;
	VectorClear (bolt->mins);
	VectorClear (bolt->maxs);
	// bolt->s.modelindex = gi.modelindex ("sprites/s_bfg2.sp2");
	// bolt->s.frame = 2;
	bolt->s.modelindex = gi.modelindex ("models/objects/laser/tris.md2");
	bolt->s.frame = 0;
	// bolt->s.sound = gi.soundindex ("weapons/bfg__l1a.wav");
	bolt->s.sound = gi.soundindex ("misc/lasfly.wav");
	bolt->s.renderfx |= RF_SHELL_GREEN;

	bolt->owner = self;
	bolt->touch = plasma_touch;
	bolt->nextthink = level.time + 2;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	gi.linkentity (bolt);

	if (self->client)
		check_dodge (self, bolt->s.origin, dir, speed);

	tr = gi.trace (self->s.origin, NULL, NULL, bolt->s.origin, bolt, MASK_SHOT);
	if (tr.fraction < 1.0)
	{
		VectorMA (bolt->s.origin, -10, dir, bolt->s.origin);
		bolt->touch (bolt, tr.ent, NULL, NULL);
	}
}

void fire_bolt (edict_t *self, vec3_t start, vec3_t aimdir, int damage)
{
	vec3_t		from;
	vec3_t		end;
	trace_t		tr;

	VectorNormalize (aimdir);	

	VectorMA (start, 8192, aimdir, end);
	VectorCopy (start, from);
	tr = gi.trace (from, NULL, NULL, end, self, MASK_SHOT|MASK_WATER);

	if ((tr.ent != self) && (tr.ent->takedamage))
	{
		int mod;

		// Set up the means of death.
		mod = MOD_PLASMAGUN;
		if ((int)fragban->value & WB_PLASMARIFLE)
			mod |= MOD_NOFRAG;

		T_Damage (tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal,
			damage, 0, DAMAGE_ENERGY, mod);
	}

	VectorCopy (tr.endpos, from);

	// draw bolt
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_BFG_LASER);
	gi.WritePosition (start);
	gi.WritePosition (tr.endpos);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	if (self->client)
		PlayerNoise (self, tr.endpos, PNOISE_IMPACT);
}

//end plasma

/*
=================
fire_grenade
=================
*/
static void Grenade_Touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (ent);
		return;
	}

	if (!other->takedamage)
	{
		if (ent->spawnflags & 1)
		{
			if (random() > 0.5)
				gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/hgrenb1a.wav"), 1, ATTN_NORM, 0);
			else
				gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/hgrenb2a.wav"), 1, ATTN_NORM, 0);
		}
		else
		{
			gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/grenlb1b.wav"), 1, ATTN_NORM, 0);
		}
		return;
	}

	ent->enemy = other;
	Grenade_Explode (ent);
}
// darKMajick: ///----------------------------------------------------->>
//

void plasma_explode (edict_t *ent)
{
	ent->nextthink = level.time + FRAMETIME;
	ent->s.frame++;
	if (ent->s.frame == 5)
		ent->think = G_FreeEdict;
}

void Flame_Touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (ent);
		return;
	}

	if (other->takedamage)
	{
		int mod;

		// Set up the means of death.
		mod = MOD_NAPALM;
		if ((int)fragban->value & WB_NAPALMGRENADE)
			mod |= MOD_NOFRAG;

		T_Damage (other, ent, ent->owner, ent->velocity, ent->s.origin,
			plane->normal, ent->dmg, 0, 0, mod);
		G_FreeEdict (ent);
	}
}

void Flame_Burn (edict_t *ent)
{
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_GRENADE_EXPLOSION);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	{
		int mod;

		// Set up the means of death.
		mod = MOD_NAPALM;
		if ((int)fragban->value & WB_NAPALMGRENADE)
			mod |= MOD_NOFRAG;

		T_RadiusDamage (ent, ent->owner, 120, NULL, 120, mod);
	}

	ent->nextthink = level.time + 0.8 + random();
	ent->delay = ent->delay - 1;
	if(ent->delay <= 0)
		G_FreeEdict (ent);
}

void Cata_Explode (edict_t *ent)
{
	ent->s.frame+=2;
	if(ent->s.frame > 12)
		ent->s.frame = 0;
	ent->nextthink = level.time + FRAMETIME;
	ent->delay = ent->delay - FRAMETIME;
	if(ent->delay <= 0)
		G_FreeEdict (ent);
}

void Cata_Explode_Touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	edict_t	 *head = NULL;
	
	if (other->classname == ent->classname)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (ent);
		return;
	}

	{
		int mod;

		// Set up the means of death.
		mod = MOD_CATA;
		if ((int)fragban->value & WB_CATACLYSMDEVICE)
			mod |= MOD_NOFRAG;

		T_RadiusDamage (ent, ent->owner, 250, NULL, 400, mod);
	}

	// Work out screen shakes:
	while ((head = findradius(head, ent->s.origin, 1024)) != NULL)
	{
		if (!head->client)
			continue;
		if (!CanDamage (head, ent))
			continue;
		head->client->v_dmg_pitch = 8 * crandom();
		head->client->v_dmg_roll = 8 * crandom();
		head->client->v_dmg_time = level.time + 1;
	}

	/*gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_GRENADE_EXPLOSION);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);*/
	gi.sound (ent, CHAN_WEAPON, gi.soundindex("weapons/grenlx1a.wav"), 1, ATTN_NORM, 0);

	G_FreeEdict (ent);
}

void Shrap_Explode (edict_t *ent)
{
	ent->s.frame+=0;
	if(ent->s.frame > 12)
		ent->s.frame = 0;
	ent->nextthink = level.time + FRAMETIME;
	ent->delay = ent->delay - FRAMETIME;
	if(ent->delay <= 0)
		G_FreeEdict (ent);
}

void Shrap_Explode_Touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	edict_t *head = NULL;
  
	if (other->classname == ent->classname)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (ent);
		return;
	}

	{
		int mod;

		// Set up the means of death.
		mod = MOD_SHRAPG;
		if ((int)fragban->value & WB_SHRAPNELGRENADE)
			mod |= MOD_NOFRAG;

		T_RadiusDamage (ent, ent->owner, 50, NULL, 50, mod);
	}

	// Work out screen shakes:
	while ((head = findradius(head, ent->s.origin, 1024)) != NULL)
	{
		if (!head->client)
			continue;
		if (!CanDamage (head, ent))
			continue;
		head->client->v_dmg_pitch = 2 * crandom();
		head->client->v_dmg_roll = 2 * crandom();
		head->client->v_dmg_time = level.time + 1;
	}

	/*gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_GRENADE_EXPLOSION);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);*/
	gi.sound (ent, CHAN_WEAPON, gi.soundindex("weapons/grenlx1a.wav"), 1, ATTN_NORM, 0);

	G_FreeEdict (ent);
}

void Grenade_Explode_dM (edict_t *ent)
{
	vec3_t		origin;
	vec3_t		grenade_angs;
	vec3_t		forward, right, up;
	int			n;
	edict_t		*flame, *head = NULL;
	int mod;

	// If we somehow don't have an owner any more, just die.
	// (Why wouldn't we have an owner?)
	if (!(ent->owner))
	{
		gi.dprintf ("WTF: dM grenade without an owner\n");
		G_FreeEdict (ent);
		return;
	}

	if (ent->owner->client)
		PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

	VectorMA (ent->s.origin, -0.02, ent->velocity, origin);
	
	if (ent->dmg == 0)
	{
		// ///------>> Standard:
		// dmg_radius is really the damage:

		// Set up the means of death.
		mod = MOD_HANDGRENADE;
		if ((int)fragban->value & WB_GRENADE)
			mod |= MOD_NOFRAG;

		T_RadiusDamage (ent, ent->owner, ent->dmg_radius, NULL,
			ent->dmg_radius + 40, mod);

		gi.WriteByte (svc_temp_entity);
		if (ent->waterlevel)
		{
			if (ent->groundentity)
				gi.WriteByte (TE_GRENADE_EXPLOSION_WATER);
			else
				gi.WriteByte (TE_ROCKET_EXPLOSION_WATER);
		}
		else
		{
			if (ent->groundentity)
				gi.WriteByte (TE_GRENADE_EXPLOSION);
			else
				gi.WriteByte (TE_ROCKET_EXPLOSION);
		}

		gi.WritePosition (origin);
		gi.multicast (ent->s.origin, MULTICAST_PVS);
	}
	else if (ent->dmg == 1)
	{
		// ///------>> Cluster:

		// Set up the means of death.
		mod = MOD_CLUSTER;
		if ((int)fragban->value & WB_CLUSTERGRENADE)
			mod |= MOD_NOFRAG;

		T_RadiusDamage (ent, ent->owner, 120, NULL, 120, mod);
		
		VectorMA (ent->s.origin, -0.02, ent->velocity, origin);
		gi.WriteByte (svc_temp_entity);
		if (ent->waterlevel)
		{
			if (ent->groundentity)
				gi.WriteByte (TE_GRENADE_EXPLOSION_WATER);
			else
				gi.WriteByte (TE_ROCKET_EXPLOSION_WATER);
		}
		else
		{
			if (ent->groundentity)
				gi.WriteByte (TE_GRENADE_EXPLOSION);
			else
				gi.WriteByte (TE_ROCKET_EXPLOSION);
		}
		gi.WritePosition (origin);
		gi.multicast (ent->s.origin, MULTICAST_PVS);

		// Cluster grenades:
		for (n = 0; n < 12; n++)
		{
			grenade_angs[0] = -45;
			grenade_angs[1] = n * 30;
			grenade_angs[2] = 0;
			AngleVectors (grenade_angs, forward, right, up);
			fire_grenade (ent->owner, origin, forward, 60, 600, 2.0, 120);
		}
	}
	else if(ent->dmg == 2)
	{
		// ///------>> RailBomb:

		// Set up the means of death.
		mod = MOD_RAILBOMB;
		if ((int)fragban->value & WB_RAILBOMB)
			mod |= MOD_NOFRAG;

		T_RadiusDamage(ent, ent->owner, ent->dmg_radius - 50, NULL,
			ent->dmg_radius - 40, mod);
		
		VectorMA (ent->s.origin, -0.02, ent->velocity, origin);
		gi.WriteByte (svc_temp_entity);
		if (ent->waterlevel)
		{
			if (ent->groundentity)
				gi.WriteByte (TE_GRENADE_EXPLOSION_WATER);
			else
				gi.WriteByte (TE_ROCKET_EXPLOSION_WATER);
		}
		else
		{
			if (ent->groundentity)
				gi.WriteByte (TE_GRENADE_EXPLOSION);
			else
				gi.WriteByte (TE_ROCKET_EXPLOSION);
		}
		gi.WritePosition (origin);
		gi.multicast (ent->s.origin, MULTICAST_PVS);
		// Now trace the railgun shots:
		for (n = 0; n < 10; n++)
		{
			grenade_angs[0] = -5 + (crandom() * 2);
			grenade_angs[1] = n*36 + crandom()*2;
			grenade_angs[2] = 0;
			AngleVectors (grenade_angs, forward, right, up);
			fire_rail (ent->owner, origin, forward, 60, 120);
		}
	}
	else if (ent->dmg == 3)
	{
		// ///------>> Plasma:

		// Set up the means of death.
		mod = MOD_PLASMAG;
		if ((int)fragban->value & WB_PLASMAGRENADE)
			mod |= MOD_NOFRAG;

		ent->classname = "plasma explosion";
		T_RadiusDamage(ent, ent->owner, 300, NULL, 300, mod);

		// Kludge to get louder sound, since vol can't exceed 1.0
		gi.sound (ent, CHAN_WEAPON, gi.soundindex("weapons/bfg__x1b.wav"), 1, ATTN_NORM, 0);
		gi.sound (ent, CHAN_VOICE, gi.soundindex("weapons/bfg__x1b.wav"), 1, ATTN_NORM, 0);
		gi.sound (ent, CHAN_ITEM, gi.soundindex("weapons/bfg__x1b.wav"), 1, ATTN_NORM, 0);

		ent->s.modelindex = gi.modelindex ("sprites/s_bfg3.sp2");
		ent->s.frame = 0;
		ent->think = plasma_explode;
		ent->nextthink = level.time + FRAMETIME;
		ent->movetype = MOVETYPE_NONE;
		ent->s.renderfx |= RF_TRANSLUCENT;
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BFG_BIGEXPLOSION);
		gi.WritePosition (ent->s.origin);
		gi.multicast (ent->s.origin, MULTICAST_PVS);
	}
	else if(ent->dmg == 4)
	{
		// ///------>> Napalm:

		// Set up the means of death.
		mod = MOD_NAPALM;
		if ((int)fragban->value & WB_NAPALMGRENADE)
			mod |= MOD_NOFRAG;

		T_RadiusDamage(ent, ent->owner, 50, NULL, 80, mod);
		
		VectorMA (ent->s.origin, -0.02, ent->velocity, origin);
		gi.WriteByte (svc_temp_entity);
		if (ent->waterlevel)
		{
			if (ent->groundentity)
				gi.WriteByte (TE_GRENADE_EXPLOSION_WATER);
			else
				gi.WriteByte (TE_ROCKET_EXPLOSION_WATER);
		}
		else
		{
			if (ent->groundentity)
				gi.WriteByte (TE_GRENADE_EXPLOSION);
			else
				gi.WriteByte (TE_ROCKET_EXPLOSION);
		}
		gi.WritePosition (origin);
		gi.multicast (ent->s.origin, MULTICAST_PVS);
		origin[2] = origin[2] + 4;
		// Throw napalm flames:
		for(n = 0; n < 8; n++)
		{
			flame = G_Spawn();
			VectorCopy (origin, flame->s.origin);
			VectorSet (flame->velocity, crandom()*500, crandom()*500, random()*400+50); // Velocity - FIXME/dM
			VectorSet (flame->avelocity, crandom()*400, crandom()*400, 0); // Avel - FIXME/dM
			flame->movetype = MOVETYPE_TOSS;
			flame->clipmask = MASK_SHOT;
			flame->solid = SOLID_TRIGGER;
			flame->s.modelindex = gi.modelindex ("models/objects/grenade2/tris.md2");
			flame->s.effects |= EF_COLOR_SHELL;
			flame->s.renderfx |= RF_SHELL_RED;
			flame->s.frame = 0;
			VectorClear (flame->mins);
			VectorClear (flame->maxs);
			flame->owner = ent->owner;
			flame->touch = Flame_Touch;
			flame->delay = 3 + random()*3;
			flame->think = Flame_Burn;
			flame->nextthink = level.time + random();
			flame->classname = "napalm";
		}
	}
	else if(ent->dmg == 5)
	{
		// ///------>> Shrapnel:

		// Big explosion effect:
		for(n = 0; n < 128; n+=32)
		{
			VectorMA (ent->s.origin, -0.02, ent->velocity, origin);
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_GRENADE_EXPLOSION);
			gi.WritePosition (origin);
			gi.multicast (origin, MULTICAST_PVS);
		}

		VectorMA (ent->s.origin, -0.02, ent->velocity, origin);
		origin[2] = origin[2] + 64;

		for (n = 0; n < 30; n++)
		{
			grenade_angs[0] = 0;
			grenade_angs[1] = n * 12;
			grenade_angs[2] = 0;
			AngleVectors (grenade_angs, forward, right, up);
			flame = G_Spawn();
			VectorCopy (origin, flame->s.origin);
			VectorClear (flame->velocity);
			VectorMA (flame->velocity, 550, forward, flame->velocity);
			flame->movetype = MOVETYPE_TOSS;
			flame->clipmask = MASK_SHOT;
			flame->solid = SOLID_TRIGGER;

			flame->s.modelindex = gi.modelindex ("models/objects/debris2/tris.md2");
			flame->s.frame = 0;
			flame->s.effects |= EF_ROCKET;
			flame->s.sound = gi.soundindex ("weapons/rocklx1a.wav");

			VectorSet (flame->mins, -3, -3, -3);
			VectorSet (flame->maxs, 3, 3, 3);
			flame->owner = ent->owner;
			flame->touch = Shrap_Explode_Touch;

			flame->delay = 5;
			flame->think = Shrap_Explode;
			flame->nextthink = level.time + FRAMETIME;

			flame->classname = "shrapnel";
		}
	}
	else if(ent->dmg == 6)
	{
		// ///------>> Cataclysm:

		// Big explosion effect:
		for(n = 0; n < 128; n+=32)
		{
			VectorMA (ent->s.origin, -0.02, ent->velocity, origin);
			origin[0] = origin[0] + 16*crandom();
			origin[1] = origin[1] + 16*crandom();
			origin[2] = origin[2] + n;
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_GRENADE_EXPLOSION);
			gi.WritePosition (origin);
			gi.multicast (origin, MULTICAST_PVS);
		}

		VectorMA (ent->s.origin, -0.02, ent->velocity, origin);
		origin[2] = origin[2] + 64;

		for (n = 0; n < 30; n++)
		{
			grenade_angs[0] = 0;
			grenade_angs[1] = n * 12;
			grenade_angs[2] = 0;
			AngleVectors (grenade_angs, forward, right, up);
			flame = G_Spawn();
			VectorCopy (origin, flame->s.origin);
			VectorClear (flame->velocity);
			VectorMA (flame->velocity, 550, forward, flame->velocity);
			flame->movetype = MOVETYPE_FLYMISSILE;
			flame->clipmask = MASK_SHOT;
			flame->solid = SOLID_TRIGGER;

			flame->s.modelindex = gi.modelindex ("sprites/s_explo2.sp2");
			flame->s.frame = random()*4;
			flame->s.effects |= EF_HYPERBLASTER;
			flame->s.sound = gi.soundindex ("weapons/bfg__l1a.wav");

			VectorSet (flame->mins, -3, -3, -3);
			VectorSet (flame->maxs, 3, 3, 3);
			flame->owner = ent->owner;
			flame->touch = Cata_Explode_Touch;

			flame->delay = 5;
			flame->think = Cata_Explode;
			flame->nextthink = level.time + FRAMETIME;

			flame->classname = "cataclysm explosion";
		}

		// Do the gamma:
		while ((head = findradius(head, ent->s.origin, 1024)) != NULL)
		{
			if (!head->client)
				continue;
			if (!CanDamage (head, ent))
				continue;
			head->client->v_dmg_pitch = 20 * crandom();
			head->client->v_dmg_roll = 20 * crandom();
			head->client->damage_blend[0] = 1;
			head->client->damage_blend[1] = 1;
			head->client->damage_blend[2] = 1;
			head->client->damage_alpha = 0.8;
			head->client->v_dmg_time = level.time + 1;
		}
	}
	else
	{
			gi.dprintf("unknown!\n");
	}

	G_FreeEdict (ent);
}

static void Grenade_Touch_dM (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (ent);
		return;
	}

	// Handle bazooka shots.
	if (ent->movetype == MOVETYPE_FLYMISSILE)
	{
		// No matter what we hit, explode.
		Grenade_Explode_dM (ent);
		return;
	}

	if (!other->takedamage)
	{
		if (ent->spawnflags & 1)
		{
			if (random() > 0.5)
				gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/hgrenb1a.wav"),
				1, ATTN_NORM, 0);
			else
				gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/hgrenb2a.wav"),
				1, ATTN_NORM, 0);
		}
		else
		{
			gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/grenlb1b.wav"),
				1, ATTN_NORM, 0);
		}
		return;
	}

	if ((ent->dmg == 0) || (ent->dmg == 1) || (ent->dmg == 2) || (ent->dmg == 3)
	|| (ent->dmg == 4))
		Grenade_Explode_dM (ent);
}

//	     ///----------------------------------------------------->>


void fire_grenade (edict_t *self, vec3_t start, vec3_t aimdir, int damage,
						 int speed, float timer, float damage_radius)
{
	edict_t	*grenade;
	vec3_t	dir;
	vec3_t	forward, right, up;

	vectoangles (aimdir, dir);
	AngleVectors (dir, forward, right, up);

	grenade = G_Spawn();
	VectorCopy (start, grenade->s.origin);
	VectorScale (aimdir, speed, grenade->velocity);
	VectorMA (grenade->velocity, 200 + crandom() * 10.0, up, grenade->velocity);
	VectorMA (grenade->velocity, crandom() * 10.0, right, grenade->velocity);
	VectorSet (grenade->avelocity, 300, 300, 300);
	grenade->movetype = MOVETYPE_BOUNCE;
	grenade->clipmask = MASK_SHOT;
	grenade->solid = SOLID_BBOX;
	grenade->s.effects |= EF_GRENADE;
	VectorClear (grenade->mins);
	VectorClear (grenade->maxs);
	grenade->s.modelindex = gi.modelindex ("models/objects/grenade/tris.md2");
	grenade->owner = self;
	grenade->touch = Grenade_Touch;
	grenade->nextthink = level.time + timer;
	grenade->think = Grenade_Explode;
	grenade->dmg = damage;
	grenade->dmg_radius = damage_radius;
	grenade->classname = "grenade";

	gi.linkentity (grenade);
}

#if 0

// Not used any more.

void fire_grenade2 (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, float timer, float damage_radius, qboolean held)
{
	edict_t	*grenade;
	vec3_t	dir;
	vec3_t	forward, right, up;

	vectoangles (aimdir, dir);
	AngleVectors (dir, forward, right, up);

	grenade = G_Spawn();
	VectorCopy (start, grenade->s.origin);
	VectorScale (aimdir, speed, grenade->velocity);
	VectorMA (grenade->velocity, 200 + crandom() * 10.0, up, grenade->velocity);
	VectorMA (grenade->velocity, crandom() * 10.0, right, grenade->velocity);
	VectorSet (grenade->avelocity, 300, 300, 300);
	grenade->movetype = MOVETYPE_BOUNCE;
	grenade->clipmask = MASK_SHOT;
	grenade->solid = SOLID_BBOX;
	grenade->s.effects |= EF_GRENADE;
	VectorClear (grenade->mins);
	VectorClear (grenade->maxs);
	grenade->s.modelindex = gi.modelindex ("models/objects/grenade2/tris.md2");
	grenade->owner = self;
	grenade->touch = Grenade_Touch;
	grenade->nextthink = level.time + timer;
	grenade->think = Grenade_Explode;
	grenade->dmg = damage;
	grenade->dmg_radius = damage_radius;
	grenade->classname = "hgrenade";
	if (held)
		grenade->spawnflags = 3;
	else
		grenade->spawnflags = 1;
	grenade->s.sound = gi.soundindex("weapons/hgrenc1b.wav");

	if (timer <= 0.0)
		Grenade_Explode (grenade);
	else
	{
		gi.sound (self, CHAN_WEAPON, gi.soundindex ("weapons/hgrent1a.wav"), 1, ATTN_NORM, 0);
		gi.linkentity (grenade);
	}
}
#endif

// darKMajick:
void fire_grenade_dM (edict_t *self, vec3_t start, vec3_t aimdir, int damage,
							 int speed, float timer, float damage_radius, int typ,
							 qboolean held, qboolean bazookad)
{
	edict_t	*grenade;
	vec3_t	dir;
	vec3_t	forward, right, up;

	vectoangles (aimdir, dir);
	AngleVectors (dir, forward, right, up);

	grenade = G_Spawn();
	VectorCopy (start, grenade->s.origin);
	VectorScale (aimdir, speed, grenade->velocity);
	if (bazookad)
	{
		grenade->movetype = MOVETYPE_FLYMISSILE;
		vectoangles (forward, grenade->s.angles);
	}
	else
	{
		VectorMA (grenade->velocity, 200 + crandom() * 10.0, up, grenade->velocity);
		VectorMA (grenade->velocity, crandom() * 10.0, right, grenade->velocity);
		VectorSet (grenade->avelocity, 300, 300, 300);
		grenade->movetype = MOVETYPE_BOUNCE;
	}
	grenade->clipmask = MASK_SHOT;
	grenade->solid = SOLID_BBOX;
	VectorClear (grenade->mins);
	VectorClear (grenade->maxs);
	if (held)
		grenade->s.modelindex = gi.modelindex ("models/objects/grenade2/tris.md2");
	else
		grenade->s.modelindex = gi.modelindex ("models/objects/grenade/tris.md2");
	if (typ == 2)
	{
		grenade->s.effects |= EF_COLOR_SHELL;
		grenade->s.renderfx |= RF_SHELL_BLUE;
		grenade->s.effects |= EF_GRENADE;
	}

	else if (typ == 3)
	{
		grenade->s.effects |= EF_COLOR_SHELL;
		grenade->s.renderfx |= RF_SHELL_GREEN;
		grenade->s.effects |= EF_BFG;
	}

	else if (typ == 4)
	{
		grenade->s.effects |= EF_COLOR_SHELL;
		grenade->s.renderfx |= RF_SHELL_RED;
		grenade->s.effects |= EF_ROCKET;
	}
	else if (typ == 6)
	{
		grenade->s.effects |= EF_COLOR_SHELL;
		grenade->s.renderfx |= RF_SHELL_BLUE;
		grenade->s.renderfx |= RF_SHELL_GREEN;
		grenade->s.renderfx |= RF_SHELL_RED;
		grenade->s.effects |= EF_ROCKET;
	}
	else
	{
		grenade->s.effects |= EF_GRENADE;
	}

	grenade->owner = self;
	if (bazookad)
	{
		grenade->nextthink = level.time + 8000/speed;
		grenade->think = G_FreeEdict;
	}
	else
	{
		grenade->nextthink = level.time + timer;
		grenade->think = Grenade_Explode_dM;
	}

	grenade->touch = Grenade_Touch_dM;
	// set dmg so grenade_explode knows what to do:
	grenade->dmg = typ;
	grenade->dmg_radius = damage + 40;
	grenade->classname = "grenade";
	if (held)
	{
		grenade->spawnflags = 1;
		grenade->s.sound = gi.soundindex("weapons/hgrenc1b.wav");
	}
	else if (bazookad)
		grenade->s.sound = gi.soundindex ("flyer/flyidle1.wav");

	if (timer <= 0.0)
	{
		Grenade_Explode_dM (grenade);
	}
	else
	{
		gi.sound (self, CHAN_WEAPON, gi.soundindex ("weapons/hgrent1a.wav"), 1, ATTN_NORM, 0);
		gi.linkentity (grenade);
	}
}

// End dm grenades

/*
=================
fire_rocket
=================
*/
void fire_rocket (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius, int radius_damage)
{
	edict_t	*rocket;

	rocket = G_Spawn();
	VectorCopy (start, rocket->s.origin);
	VectorCopy (dir, rocket->movedir);
	vectoangles (dir, rocket->s.angles);
	VectorScale (dir, speed, rocket->velocity);
	rocket->movetype = MOVETYPE_FLYMISSILE;
	rocket->clipmask = MASK_SHOT;
	rocket->solid = SOLID_BBOX;
	rocket->s.effects |= EF_ROCKET;
	VectorClear (rocket->mins);
	VectorClear (rocket->maxs);
	rocket->s.modelindex = gi.modelindex ("models/objects/rocket/tris.md2");
	rocket->owner = self;
	rocket->touch = rocket_touch;
	rocket->nextthink = level.time + 8000/speed;
	rocket->think = G_FreeEdict;
	rocket->dmg = damage;
	rocket->radius_dmg = radius_damage;
	rocket->dmg_radius = damage_radius;
	rocket->s.sound = gi.soundindex ("weapons/rockfly.wav");
	rocket->classname = "rocket";

	if (self->client)
		check_dodge (self, rocket->s.origin, dir, speed);

	gi.linkentity (rocket);
}

// machine rockets
/*
=================
Machine Gun Rockets 
=================
*/
void mr_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t	origin;
	int		mod;

	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (ent);
		return;
	}

	if (ent->owner->client)
		PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

	// calculate position for the explosion entity
	VectorMA (ent->s.origin, -0.02, ent->velocity, origin);

	// Set up the means of death.
	mod = MOD_MACHINEGUN;
	if ((int)fragban->value & WB_MACHINEROCKETGUN)
		mod |= MOD_NOFRAG;

	if (other->takedamage)
	{
		T_Damage (other, ent, ent->owner, ent->velocity, ent->s.origin,
			plane->normal, ent->dmg, 20, 20, mod);
	}

	T_RadiusDamage(ent, ent->owner, ent->radius_dmg, other, ent->dmg_radius, mod);

	gi.WriteByte (svc_temp_entity);
	if (ent->waterlevel)
		gi.WriteByte (TE_ROCKET_EXPLOSION_WATER);
	else
		gi.WriteByte (TE_ROCKET_EXPLOSION);
	gi.WritePosition (origin);
	// gi.multicast (ent->s.origin, MULTICAST_PHS);
	gi.multicast (ent->s.origin, MULTICAST_PVS);		// WI -- cut down mr impact on game

	G_FreeEdict (ent);
}
void fire_mr (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius, int radius_damage)
{
	edict_t	*rocket;

	rocket = G_Spawn();
	VectorCopy (start, rocket->s.origin);
	VectorCopy (dir, rocket->movedir);
	vectoangles (dir, rocket->s.angles);
	VectorScale (dir, speed, rocket->velocity);
	rocket->movetype = MOVETYPE_FLYMISSILE;
	rocket->clipmask = MASK_SHOT;
	rocket->solid = SOLID_BBOX;
	rocket->s.effects |= 0; // EF_GRENADE;
	VectorClear (rocket->mins);
	VectorClear (rocket->maxs);
	rocket->s.modelindex = gi.modelindex ("models/objects/rocket/tris.md2");
	rocket->owner = self;
	rocket->touch = mr_touch;
	rocket->nextthink = level.time + 8000/speed;
	rocket->think = G_FreeEdict;
	rocket->dmg = damage;
	rocket->radius_dmg = 40;
	rocket->dmg_radius = 40;
	rocket->s.sound = gi.soundindex ("weapons/rockfly.wav");
	rocket->classname = "rocket";

	if (self->client)
		check_dodge (self, rocket->s.origin, dir, speed);

	gi.linkentity (rocket);
}
// machine rockets end


/*
=================
fire_bfg
=================
*/
void bfg_explode (edict_t *self)
{
	edict_t	*ent;
	float	points;
	vec3_t	v;
	float	dist;
	int mod;

	// Set up the means of death.
	mod = MOD_BFG_EFFECT;
	if ((int)fragban->value & WB_BFG10K)
		mod |= MOD_NOFRAG;

	if (self->s.frame == 0)
	{
		// the BFG effect
		ent = NULL;
		while ((ent = findradius (ent, self->s.origin, self->dmg_radius)) != NULL)
		{
			if (!ent->takedamage)
				continue;
			if (ent == self->owner)
				continue;
			if (!CanDamage (ent, self))
				continue;
			if (!CanDamage (ent, self->owner))
				continue;

			VectorAdd (ent->mins, ent->maxs, v);
			VectorMA (ent->s.origin, 0.5, v, v);
			VectorSubtract (self->s.origin, v, v);
			dist = VectorLength(v);
			points = self->radius_dmg * (1.0 - sqrt(dist/self->dmg_radius));
			if (ent == self->owner)
				points = points * 0.5;

			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_BFG_EXPLOSION);
			gi.WritePosition (ent->s.origin);
			gi.multicast (ent->s.origin, MULTICAST_PHS);
			T_Damage (ent, self, self->owner, self->velocity, ent->s.origin,
				vec3_origin, (int)points, 0, DAMAGE_ENERGY, mod);
		}
	}

	self->nextthink = level.time + FRAMETIME;
	self->s.frame++;
	if (self->s.frame == 5)
		self->think = G_FreeEdict;
}

void bfg_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	int mod;

	if (other == self->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (self);
		return;
	}

	if (self->owner->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	// Set up the means of death.
	mod = MOD_BFG_BLAST;
	if ((int)fragban->value & WB_BFG10K)
		mod |= MOD_NOFRAG;

	// core explosion - prevents firing it into the wall/floor
	if (other->takedamage)
		T_Damage (other, self, self->owner, self->velocity, self->s.origin,
			plane->normal, 200, 0, 0, mod);
	T_RadiusDamage (self, self->owner, 200, other, 100, mod);

	gi.sound (self, CHAN_VOICE, gi.soundindex ("weapons/bfg__x1b.wav"), 1, ATTN_NORM, 0);
	self->solid = SOLID_NOT;
	self->touch = NULL;
	VectorMA (self->s.origin, -1 * FRAMETIME, self->velocity, self->s.origin);
	VectorClear (self->velocity);
	self->s.modelindex = gi.modelindex ("sprites/s_bfg2.sp2");
	self->s.frame = 0;
	self->s.sound = 0;
	self->s.effects &= ~EF_ANIM_ALLFAST;
	self->think = bfg_explode;
	self->nextthink = level.time + FRAMETIME;
	self->enemy = other;

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_BFG_BIGEXPLOSION);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PVS);
}


void bfg_think (edict_t *self)
{
	edict_t	*ent;
	edict_t	*ignore;
	vec3_t	point;
	vec3_t	dir;
	vec3_t	start;
	vec3_t	end;
	int		dmg;
	trace_t	tr;
	int mod;

	// Set up the means of death.
	mod = MOD_BFG_LASER;
	if ((int)fragban->value & WB_BFG10K)
		mod |= MOD_NOFRAG;

	if (deathmatch->value)
		dmg = 5;
	else
		dmg = 10;

	ent = NULL;
	while ((ent = findradius(ent, self->s.origin, 256)) != NULL)
	{
		if (ent == self)
			continue;

		if (ent == self->owner)
			continue;

		if (!ent->takedamage)
			continue;

		if (!(ent->svflags & SVF_MONSTER) && (!ent->client) && (strcmp(ent->classname, "misc_explobox") != 0))
			continue;

//ZOID
		//don't target teammates in CTF
		if (ctf->value && ent->client &&
			self->owner->client &&
			ent->client->resp.ctf_team == self->owner->client->resp.ctf_team)
			continue;
//ZOID

		VectorMA (ent->absmin, 0.5, ent->size, point);

		VectorSubtract (point, self->s.origin, dir);
		VectorNormalize (dir);

		ignore = self;
		VectorCopy (self->s.origin, start);
		VectorMA (start, 2048, dir, end);
		while(1)
		{
			tr = gi.trace (start, NULL, NULL, end, ignore, CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_DEADMONSTER);

			if (!tr.ent)
				break;

			// hurt it if we can
			if ((tr.ent->takedamage) && !(tr.ent->flags & FL_IMMUNE_LASER) && (tr.ent != self->owner))
				T_Damage (tr.ent, self, self->owner, dir, tr.endpos, vec3_origin,
					dmg, 1, DAMAGE_ENERGY, mod);

			// if we hit something that's not a monster or player we're done
			if (!(tr.ent->svflags & SVF_MONSTER) && (!tr.ent->client))
			{
				gi.WriteByte (svc_temp_entity);
				gi.WriteByte (TE_LASER_SPARKS);
				gi.WriteByte (4);
				gi.WritePosition (tr.endpos);
				gi.WriteDir (tr.plane.normal);
				gi.WriteByte (self->s.skinnum);
				gi.multicast (tr.endpos, MULTICAST_PVS);
				break;
			}

			ignore = tr.ent;
			VectorCopy (tr.endpos, start);
		}

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BFG_LASER);
		gi.WritePosition (self->s.origin);
		gi.WritePosition (tr.endpos);
		gi.multicast (self->s.origin, MULTICAST_PHS);
	}

	self->nextthink = level.time + FRAMETIME;
}


void fire_bfg (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius)
{
	edict_t	*bfg;

	bfg = G_Spawn();
	VectorCopy (start, bfg->s.origin);
	VectorCopy (dir, bfg->movedir);
	vectoangles (dir, bfg->s.angles);
	VectorScale (dir, speed, bfg->velocity);
	bfg->movetype = MOVETYPE_FLYMISSILE;
	bfg->clipmask = MASK_SHOT;
	bfg->solid = SOLID_BBOX;
	bfg->s.effects |= EF_BFG | EF_ANIM_ALLFAST;
	VectorClear (bfg->mins);
	VectorClear (bfg->maxs);
	bfg->s.modelindex = gi.modelindex ("sprites/s_bfg1.sp2");
	bfg->owner = self;
	bfg->touch = bfg_touch;
	bfg->nextthink = level.time + 8000/speed;
	bfg->think = G_FreeEdict;
	bfg->radius_dmg = damage;
	bfg->dmg_radius = damage_radius;
	bfg->classname = "bfg blast";
	bfg->s.sound = gi.soundindex ("weapons/bfg__l1a.wav");

	bfg->think = bfg_think;
	bfg->nextthink = level.time + FRAMETIME;
	bfg->teammaster = bfg;
	bfg->teamchain = NULL;

	if (self->client)
		check_dodge (self, bfg->s.origin, dir, speed);

	gi.linkentity (bfg);
}

/*
=================
fire_super

Fires a super blaster bolt.
=================
*/
void super_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	int		mod;

	if (other == self->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (self);
		return;
	}

	if (self->owner->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	if (other->takedamage)
	{
		if (self->spawnflags & 1)
			mod = MOD_HYPERBLASTER;
		else
			mod = MOD_SB;
		if ((int)fragban->value & WB_SUPERBLASTER)
			mod |= MOD_NOFRAG;
		T_Damage (other, self, self->owner, self->velocity, self->s.origin,
			plane->normal, self->dmg, 1, DAMAGE_ENERGY, mod);
	}
	else
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BLASTER);
		gi.WritePosition (self->s.origin);
		if (!plane)
			gi.WriteDir (vec3_origin);
		else
			gi.WriteDir (plane->normal);
		gi.multicast (self->s.origin, MULTICAST_PVS);
	}

	G_FreeEdict (self);
}

void fire_super (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int effect)
{
	edict_t *bolt;
	trace_t tr;

	VectorNormalize (dir);


	// Only change hand blaster effect
	if (effect & EF_BLASTER)
	{
		bolt = G_Spawn();
		VectorCopy (start, bolt->s.origin);
		vectoangles (dir, bolt->s.angles);
		VectorScale (dir, speed, bolt->velocity);
		VectorAdd (start, bolt->velocity, bolt->s.old_origin);
		bolt->clipmask = MASK_SHOT;

		bolt->movetype = MOVETYPE_FLYMISSILE;
		bolt->solid = SOLID_BBOX;
		bolt->s.renderfx |= RF_SHELL_RED;
		bolt->s.effects |= EF_COLOR_SHELL;
		bolt->s.effects |= EF_FLAG1;
		bolt->s.modelindex = gi.modelindex ("models/objects/laser/tris.md2");
		bolt->owner = self;

		bolt->s.frame = 0;

		VectorSet (bolt->mins, -8, -8, -8);
		VectorSet (bolt->maxs, 8, 8, 8);

		bolt->touch = super_touch;
		bolt->nextthink = level.time + 4;
		bolt->think = G_FreeEdict;
		bolt->dmg = damage;

		gi.linkentity (bolt);

		if (self->client)
			check_dodge (self, bolt->s.origin, dir, speed);

		tr = gi.trace (self->s.origin, NULL, NULL, bolt->s.origin, bolt,
			MASK_SHOT);
		if (tr.fraction < 1.0)
		{
			VectorMA (bolt->s.origin, -10, dir, bolt->s.origin);
			bolt->touch (bolt, tr.ent, NULL, NULL);
		}

		return;
	}

	bolt = G_Spawn();
	VectorCopy (start, bolt->s.origin);
	VectorCopy (start, bolt->s.old_origin);
	vectoangles (dir, bolt->s.angles);
	VectorScale (dir, speed, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	bolt->s.effects |= effect;
	VectorClear (bolt->mins);
	VectorClear (bolt->maxs);
	bolt->s.modelindex = gi.modelindex ("models/objects/laser/tris.md2");
	bolt->s.sound = gi.soundindex ("misc/lasfly.wav");
	bolt->owner = self;
	bolt->touch = blaster_touch;
	bolt->nextthink = level.time + 2;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	gi.linkentity (bolt);

	if (self->client)
		check_dodge (self, bolt->s.origin, dir, speed);

	tr = gi.trace (self->s.origin, NULL, NULL, bolt->s.origin, bolt, MASK_SHOT);
	if (tr.fraction < 1.0)
	{
		VectorMA (bolt->s.origin, -10, dir, bolt->s.origin);
		bolt->touch (bolt, tr.ent, NULL, NULL);
	}
}
/*
=================
fire_rail
=================
*/
void fire_rail (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick)
{
	vec3_t		from;
	vec3_t		end;
	trace_t		tr;
	edict_t		*ignore;
	int			mask;
	qboolean	water;
	int mod;

	// Set up the means of death.
	mod = MOD_RAILGUN2;
	if ((int)fragban->value & WB_RAILGUN)
		mod |= MOD_NOFRAG;

	VectorMA (start, 8192, aimdir, end);
	VectorCopy (start, from);
	ignore = self;
	water = false;
	mask = MASK_SHOT|CONTENTS_SLIME|CONTENTS_LAVA;
	while (ignore)
	{
		tr = gi.trace (from, NULL, NULL, end, ignore, mask);

		if (tr.contents & (CONTENTS_SLIME|CONTENTS_LAVA))
		{
			mask &= ~(CONTENTS_SLIME|CONTENTS_LAVA);
			water = true;
		}
		else
		{
			if ((tr.ent->svflags & SVF_MONSTER) || (tr.ent->client))
				ignore = tr.ent;
			else
				ignore = NULL;

			if ((tr.ent != self) && (tr.ent->takedamage))
				T_Damage (tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal,
					damage, kick, 0, mod);
		}

		VectorCopy (tr.endpos, from);
	}

	// send gun puff / flash
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_RAILTRAIL);
	gi.WritePosition (start);
	gi.WritePosition (tr.endpos);
	gi.multicast (self->s.origin, MULTICAST_PHS);
//	gi.multicast (start, MULTICAST_PHS);
	if (water)
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_RAILTRAIL);
		gi.WritePosition (start);
		gi.WritePosition (tr.endpos);
		gi.multicast (tr.endpos, MULTICAST_PHS);
	}

	if (self->client)
		PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
}


// homing
void
homing_think (edict_t *ent) 
{
	vec_t speed;
	vec3_t targetdir;

	// Nudge our direction toward our target.
	VectorSubtract (ent->enemy->s.origin, ent->s.origin, targetdir);
	targetdir[2] += 16;
	VectorNormalize (targetdir);
	VectorScale (targetdir, 0.2, targetdir);
	VectorAdd (targetdir, ent->movedir, targetdir); 
	VectorNormalize (targetdir);
	VectorCopy (targetdir, ent->movedir); 
	vectoangles (targetdir, ent->s.angles);
	speed = VectorLength(ent->velocity);
	VectorScale (targetdir, speed, ent->velocity);

	ent->nextthink = level.time + FRAMETIME;

	gi.linkentity (ent);
}

void
homing_explode (edict_t *ent)
{
	vec3_t		origin;
	int mod;

	// Set up the means of death.
	mod = MOD_H_SPLASH;
	if ((int)fragban->value & WB_HOMINGROCKETLAUNCHER)
		mod |= MOD_NOFRAG;

	if (ent->owner->client)
		PlayerNoise (ent->owner, ent->s.origin, PNOISE_IMPACT);

	// calculate position for the explosion entity
	VectorMA (ent->s.origin, -0.02, ent->velocity, origin);

	ent->takedamage = DAMAGE_NO;
	T_RadiusDamage (ent, ent->owner, ent->radius_dmg, ent->enemy,
		ent->dmg_radius, mod);

	gi.WriteByte (svc_temp_entity);
	if (ent->waterlevel)
		gi.WriteByte (TE_ROCKET_EXPLOSION_WATER);
	else
		gi.WriteByte (TE_ROCKET_EXPLOSION);
	gi.WritePosition (origin);
	gi.multicast (ent->s.origin, MULTICAST_PHS);

	G_FreeEdict (ent);
}


void
homing_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage,
				vec3_t point)
{
	// Don't take any more damage.
	self->takedamage = DAMAGE_NO;

	// Give them credit for shooting us out of the sky, by not hurting them.
	self->enemy = attacker;

	// Blow up.
	self->think = homing_explode;
	self->nextthink = level.time + FRAMETIME;
}


void
homing_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (ent);
		return;
	}

	if (other->takedamage)
	{
		int mod;

		// Set up the means of death.
		mod = MOD_HOMING;
		if ((int)fragban->value & WB_HOMINGROCKETLAUNCHER)
			mod |= MOD_NOFRAG;

		// How much damage we've received affects how much we dish out.
		ent->dmg = ent->health;

		T_Damage (other, ent, ent->owner, ent->velocity, ent->s.origin,
			plane->normal, ent->dmg, 120, 0, mod);
	}
	else
	{
		// don't throw any debris in net games
		if (!deathmatch->value && !coop->value)
		{
			if ((surf) && !(surf->flags & (SURF_WARP|SURF_TRANS33|SURF_TRANS66|SURF_FLOWING)))
			{
				int			n;

				n = rand() % 5;
				while(n--)
					ThrowDebris (ent, "models/objects/debris2/tris.md2", 2, ent->s.origin);
			}
		}
	}

	// Now make the rocket explode.
	ent->enemy = other;
	homing_explode (ent);
}


void
fire_homing (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed,
				 float damage_radius, int radius_damage)
{
	edict_t	*rocket;

	rocket = G_Spawn();
	VectorCopy (start, rocket->s.origin);
	VectorCopy (dir, rocket->movedir);
	vectoangles (dir, rocket->s.angles);
	VectorScale (dir, speed, rocket->velocity);
	rocket->movetype = MOVETYPE_FLYMISSILE;
	rocket->clipmask = MASK_SHOT;
	rocket->solid = SOLID_BBOX;
	rocket->s.effects |= EF_ROCKET;
	VectorClear (rocket->mins);
	VectorClear (rocket->maxs);
	rocket->model = "models/objects/rocket/tris.md2";
	rocket->s.modelindex = gi.modelindex (rocket->model);
	rocket->owner = self;
	rocket->touch = homing_touch;
	rocket->enemy = NULL;

	// Make homing rocket shootable.
	rocket->movetype = MOVETYPE_FLYMISSILE;
	rocket->takedamage = DAMAGE_YES;
	//rocket->svflags |= SVF_MONSTER;
	//rocket->clipmask |= CONTENTS_MONSTERCLIP;
	VectorSet (rocket->mins, -8, -8, -8);
	VectorSet (rocket->maxs, 8, 8, 8);
	rocket->mass = 10;
	rocket->health = damage;
	rocket->max_health = damage;
	rocket->die = homing_die;
	//rocket->monsterinfo.aiflags = AI_NOSTEP;

	rocket->dmg = damage;
	rocket->radius_dmg = 120;
	rocket->dmg_radius = 120;
	rocket->s.sound = gi.soundindex ("weapons/rockfly.wav");
	rocket->classname = "rocket";

	// Find a target to home in on.
	{
		edict_t *blip = NULL;
		vec3_t blipdir;
		float blipDot, targetDot;

		blip = NULL;
		while ((blip = findradius (blip, rocket->s.origin, 1024)) != NULL)
		{
			// See if this is the kind of blip we can home in on.
			if (!(blip->svflags & SVF_MONSTER) && !blip->client)
				continue;
			if (blip == rocket->owner)
				continue;
			if (!blip->takedamage)
				continue;
			if (blip->health <= 0)
				continue;
			if (ctf->value
			&& blip->client
			&& self->client->resp.ctf_team == blip->client->resp.ctf_team)
				continue;
			if (!visible (rocket, blip))
				continue;
			if (!infront (rocket, blip))
				continue;

			// Determine where the blip is in relation to us.
			VectorSubtract (blip->s.origin, rocket->s.origin, blipdir);
			blipdir[2] += 16;

			// Determine how "good" of a target it is.  (Currently, that's the
			// enemy that's most being aimed at, regardless of distance.)
			VectorNormalize (blipdir);
			blipDot = DotProduct (dir, blipdir);

			// Remember the "best" target so far.
			if (rocket->enemy == NULL || targetDot < blipDot)
			{
				rocket->enemy = blip;
				targetDot = blipDot;
			}
		}
	}

	// Did we find a target?
	if (rocket->enemy)
	{
		// Sound a warning for the targeted one.
		gi.sound (rocket->enemy, CHAN_AUTO, gi.soundindex ("misc/keytry.wav"), 1, ATTN_NORM, 0);
		
		// Keep tracking them.
		rocket->nextthink = level.time + FRAMETIME;
		rocket->think = homing_think;
	}
	else
	{
		// Let the inflictor know the rocket isn't going to home in on anyone.
		gi.sound (self, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1, ATTN_NORM, 0);
	}

	if (self->client)
		check_dodge (self, rocket->s.origin, dir, speed);

	gi.linkentity (rocket);
}


void freezer_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == self->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (self);
		return;
	}

	if (self->owner->client)
		PlayerNoise (self->owner, self->s.origin, PNOISE_IMPACT);

	if (other->takedamage)
	{
		int damage;
		int mod;

		// Set up the means of death.
		mod = MOD_FREEZE;
		if ((int)fragban->value & WB_FREEZEGUN)
			mod |= MOD_NOFRAG;

		// Hurt them, keep track of whether we could.
		damage = other->health;
		T_Damage (other, self, self->owner, self->velocity, self->s.origin,
			plane->normal, self->dmg, 1, DAMAGE_ENERGY, mod);
		damage -= other->health;
		gi.sound (self, CHAN_VOICE, gi.soundindex ("weapons/frozen.wav"), 1,
			ATTN_NORM, 0);

		// If we could hurt them, freeze them.
		if (damage > 0)
		{
			// Freeze them for this long.
			if (other->frozen)
				other->frozentime += 4.0;
			else
				other->frozentime = level.time + 4.0;

			// Freeze them.
			other->frozen = 1;
		}
	}
	else
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BLASTER);
		gi.WritePosition (self->s.origin);
		if (!plane)
			gi.WriteDir (vec3_origin);
		else
			gi.WriteDir (plane->normal);
		gi.multicast (self->s.origin, MULTICAST_PVS);
	}

	G_FreeEdict (self);
}

void fire_freezer (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int effect)
{
	edict_t	*bolt;
	trace_t	tr;

	VectorNormalize (dir);

	bolt = G_Spawn();
	VectorCopy (start, bolt->s.origin);
	VectorCopy (start, bolt->s.old_origin);
	vectoangles (dir, bolt->s.angles);
	VectorScale (dir, speed, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	bolt->s.effects |= EF_FLAG2;
   bolt->s.effects |= EF_COLOR_SHELL;
   bolt->s.renderfx |= RF_SHELL_BLUE;
	VectorClear (bolt->mins);
	VectorClear (bolt->maxs);
	bolt->s.modelindex = gi.modelindex ("models/objects/laser/tris.md2");
	bolt->s.sound = gi.soundindex ("misc/lasfly.wav");
	bolt->owner = self;
	bolt->touch = freezer_touch;
	bolt->nextthink = level.time + 8000/speed;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	gi.linkentity (bolt);

	if (self->client)
		check_dodge (self, bolt->s.origin, dir, speed);

	tr = gi.trace (self->s.origin, NULL, NULL, bolt->s.origin, bolt, MASK_SHOT);
	if (tr.fraction < 1.0)
	{
		VectorMA (bolt->s.origin, -10, dir, bolt->s.origin);
		bolt->touch (bolt, tr.ent, NULL, NULL);
	}
}	
// **************************

// EOF
