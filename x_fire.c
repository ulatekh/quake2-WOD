/*==========================================================================
//  x_fire.c -- by Patrick Martin               Last updated:  2-19-1998
//--------------------------------------------------------------------------
//  This file contains code that produces fire that burns entities.
//
//  Fire inflicts damage to entities near it.  In addition, there is a
//  chance that an affected entity will catch on fire.  If this happens,
//  the fire will burn the target for some time and inflict more damage.
//  Fire will be extinguished if the burning entity enters at least
//  waist-deep into liquid.  Fire can also spread from entity to entity.
//  However, all frags (and anger) are given only to the entity that
//  originally spawned the fire.
//
//  NOTES:  The following edicts fields defined in 'g_local.h'
//  are used differently here than in other files:
//
//  * count = Chance that a flame will be created and dropped
//      from an exploding fireball.
//  * dmg = Chance that a dropped flame will explode on impact.
//  * pos1 = Fire damage vector for direct hits.  {die, base, chance}.
//      Damage is 1d(die) + base.  Target has chance% of igniting.
//  * pos2 = Same as pos1 except this is used for splash damage.
//  * timestamp = Time when burning flames will damage nearby entities.
//
//  This code uses my custom flame model.  It is similar to
//  (but NOT the same as) the flame2.mdl in Quake 1.
//
//  Frames:
//     0 -  5  Small fire (used for fireballs and the like)
//     6 - 15  Large fire (used for burning entities)
//         16  Tiny spark (near invisible; used for smoke)
//         17  Base frame (NOT used)
//
//  Skin:
//     0  Orange Bunsen burner style flame
//
//========================================================================*/

#include "g_local.h"
#include "x_fire.h"



/*============================/  Fire Dodge  /============================*/

/*-------------------------------------------------------- Mirror Code -----
//  This is a copy of 'check_dodge' found in 'g_weapons.c'.
//  Because that function is static, and I do not want to
//  modify that file, a copy is placed here.
//------------------------------------------------------------------------*/
void check_firedodge (edict_t *self, vec3_t start, vec3_t dir, int speed)
{
	vec3_t	end;
	vec3_t	v;
	trace_t	tr;
	float	eta;

	/* easy mode only ducks one quarter the time */
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


/*========================/  Custom Explosions  /========================*/

/*-------------------------------------------------------- New Code --------
//  This transforms the fire into smoke.
//------------------------------------------------------------------------*/
void PBM_BecomeSmoke (edict_t *self)
{
/* Make some steam. */
	self->s.modelindex = MD2_FIRE;
	self->s.frame      = FRAME_FLAMEOUT;
	self->touch        = NULL;
	self->s.effects    = EF_GRENADE;
	self->movetype     = MOVETYPE_FLY;
	VectorSet(self->velocity, 0, 0, 400);
	self->nextthink    = level.time + 0.1;
	self->think        = G_FreeEdict;
}

/*-------------------------------------------------------- New Code --------
//  This transforms the fire into steam.
//------------------------------------------------------------------------*/
void PBM_BecomeSteam (edict_t *self)
{
/* fire/steam.wav is a custom sound. //
	gi.sound(self, CHAN_BODY, gi.soundindex("fire/steam.wav"), 1, ATTN_NORM, 0);
*/

/* world/steam*.wav are standard Q2 sounds. */
	if (rand() & 1)
		gi.sound(self, CHAN_BODY, gi.soundindex("world/steam1.wav"), 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_BODY, gi.soundindex("world/steam2.wav"), 1, ATTN_NORM, 0);

	PBM_BecomeSmoke(self);
}

/*-------------------------------------------------------- New Code --------
//  This animates the small explosion.  The magic numbers in this
//  function represent the frame numbers in id's r_explode md2.
//------------------------------------------------------------------------*/
void PBM_SmallExplodeThink (edict_t *self)
{
	if (++self->s.frame > 7)
	{	G_FreeEdict (self);
		return;
	}

	self->s.skinnum++;

	if (self->s.frame == 7)
		self->s.renderfx |= RF_TRANSLUCENT;
	else if (self->s.frame == 3)
		self->s.effects &= ~EF_HYPERBLASTER;

	self->nextthink = level.time + FRAMETIME;
}

/*-------------------------------------------------------- New Code --------
//  This transforms an entity into an explosion.  If entity
//  is underwater, steam is produced instead.
//------------------------------------------------------------------------*/
void PBM_BecomeNewExplosion (edict_t *self)
{
	if (gi.pointcontents(self->s.origin) & MASK_WATER)
	{	PBM_BecomeSteam (self);
		return;
	}

	gi.sound(self, CHAN_AUTO, gi.soundindex("weapons/grenlx1a.wav"), 1, ATTN_NORM, 0);
	self->s.modelindex = gi.modelindex("models/objects/r_explode/tris.md2");
	self->s.frame      = 1;
	self->s.skinnum    = 0;
	self->touch        = NULL;
	self->s.effects    = EF_HYPERBLASTER;
	self->s.renderfx   = RF_FULLBRIGHT;
	self->movetype     = MOVETYPE_NONE;
	VectorClear(self->velocity);
	VectorClear(self->s.angles);
	self->s.angles[1]  = rand() % 360;
	self->nextthink    = level.time + FRAMETIME;
	self->think        = PBM_SmallExplodeThink;
}


/*========================/  General Fire Code  /========================*/

/*-------------------------------------------------------- New Code --------
//  This checks if 'ent' (the entity on fire) found some water.
//
//  NOTE:  Checking the entity rather than the flame itself
//         works to the entity's benefit, because the flame
//         is (usually) set higher than ent->s.origin.
//------------------------------------------------------------------------*/
qboolean PBM_InWater (edict_t *ent)
{
/* For players and monsters. */
	if (ent->waterlevel >= 2)
		return true;

/* All-purpose check. */
	if (gi.pointcontents(ent->s.origin) & MASK_WATER)
		return true;
	else
		return false;
}

/*-------------------------------------------------------- New Code --------
//  This checks if 'ent' is vulnerable to fire.  Entities that are
//  immune to fire return false.  Entities that can burn return true.
//------------------------------------------------------------------------*/
qboolean PBM_Flammable (edict_t *ent)
{
/* Thou canst not burn what doth not exist. */
	if (!ent)  return false;

/* Any entity that is impervious to damage is inflammable. */
	if (!ent->takedamage)  return false;

/* Dead and gibbed entities cannot burn. */
	if (ent->health <= ent->gib_health)  return false;

/* Any entity that is immune to lava cannot burn. */
	if (ent->flags & FL_IMMUNE_LAVA)  return false;

/* If entity is a client, check for powerups. */
	if (ent->client)
	{
	/* Invulnerability protects entity from burning. */
		if (ent->client->invincible_framenum > level.framenum)
			return false;

	/* Bio-suit offers partial immunity to fire. */
		if ((ent->client->enviro_framenum > level.framenum) && (random() < 0.75))
			return false;
	}

/* The entity is flammable. */
	 return true;
}

/*-------------------------------------------------------- New Code --------
//  This checks if 'ent' can be set on fire.  If so, false is returned.
//------------------------------------------------------------------------*/
qboolean PBM_FireResistant (edict_t *ent)
{
/* An entity immune to damage (or fire) obviously can resist fire. */
	if (!PBM_Flammable(ent))
		return true;

/* Only creatures or barrels can be set on fire. */
	if ( (!ent->client) && (!(ent->svflags & SVF_MONSTER)) &&
		  (strcmp(ent->classname, "misc_explobox")) )
		return true;

/* Power armor halves the chance of igniting. */
	if ((ent->flags & FL_POWER_ARMOR) && (rand() & 1))
		return true;

/* The following monster types are resistant to fire. */
	if ( (!strcmp(ent->classname, "monster_tank")) ||
		  (!strcmp(ent->classname, "monster_tank_commander")) ||
		  (!strcmp(ent->classname, "monster_supertank")) ||
		  (!strcmp(ent->classname, "monster_boss2")) ||
		  (!strcmp(ent->classname, "monster_makron")) ||
		  (!strcmp(ent->classname, "monster_jorg")) )
		return true;

/* Entity can be set on fire. */
	return false;
}

/*-------------------------------------------------------- New Code --------
//  This damages a single entity and possibly sets it on fire.
//
//  NOTES:  Damage = 1d(die) + base.  (This is D&D notation)
//          Chance is expressed as a percentange.  (e.g., 20 = 20%.)
//------------------------------------------------------------------------*/
void PBM_BurnDamage
(edict_t *victim, edict_t *fire, vec3_t damage)
{
	int   points;
	int   die = (int)damage[0];
	int   base = (int)damage[1];
	int   chance = (int)damage[2];

/* Calculate damage. */
	if (die > 0)
		points = (rand() % die) + base + 1;
	else
		points = base;

/* FIXME:  Check for quad damage bonus. */
	if (fire->owner->client)
		if (fire->owner->client->quad_framenum > level.framenum)
		{	chance *= 4;
			points *= 4;
		}

/* Check if entity will catch on fire. */
	if ((rand() % 40) < chance)
		PBM_Ignite(victim, fire->owner);

/* Inflict some burn damage. */
	T_Damage(victim, fire, fire->owner, vec3_origin, victim->s.origin, vec3_origin, points, 0, 0, MOD_RAILGUN);
}

/*-------------------------------------------------------- New Code --------
//  This damages entities within a certain radius.  There is
//  also a chance that affected entities will catch on fire.
//
//  NOTES:  Damage = 1d(die) + base.  Chance is expressed as percentange.
//------------------------------------------------------------------------*/
void PBM_BurnRadius
(edict_t *fire, float radius, vec3_t damage, edict_t *ignore)
{
	edict_t   *ent = NULL;

	while ((ent = findradius(ent, fire->s.origin, radius)) != NULL)
	{	if (CanDamage (ent, fire))
			if (PBM_Flammable(ent))
				if (ent != ignore)
					PBM_BurnDamage(ent, fire, damage);
	}
}

/*-------------------------------------------------------- New Code --------
//  This returns the best location on the entity for where the fire
//  should burn.
//------------------------------------------------------------------------*/
void PBM_FireSpot (vec3_t spot, edict_t *ent)
{
	vec3_t   forward, right, up;

/* Get the entity's forward direction. */
	AngleVectors(ent->s.angles, forward, right, up);
	VectorNormalize(forward);
	VectorScale(forward, 4, forward);

/* Put the flame at the best spot on an entity. */
	VectorCopy(ent->s.origin, spot);
	VectorAdd(spot, forward, spot);

/* Adjust elevation of flame depending on the entity. */
	if (ent->client)
		spot[2] = spot[2] + ent->viewheight - 18;
	else
	{
		if (ent->health > 0)
			spot[2] += 18;
		else
			spot[2] -= 18;

	/* Extra adjustments for certain entities. */
		if (!strcmp(ent->classname, "monster_chick"))
			spot[2] += 24;
		else if (!strcmp(ent->classname, "misc_explobox"))
			spot[2] += 32;
		else if (ent->health > 0)
		{  if (!strcmp(ent->classname, "misc_insane"))
				spot[2] -= 12;
			else if (!strcmp(ent->classname, "monster_flyer"))
				spot[2] -= 16;
			else if (!strcmp(ent->classname, "monster_mutant"))
				spot[2] += 24;
		}
	}
}

/*-------------------------------------------------------- New Code --------
//  This checks if the fire burning on an entity should die.
//------------------------------------------------------------------------*/
qboolean PBM_FlameOut (edict_t *self)
{
/* If entity suddenly disappears from Quake2, remove the fire. */
	if (!self->enemy)
	{	PBM_BecomeNewExplosion (self);
		return true;
	}

/* If no burnout time is specified, entity should not be burning. */
	if (!self->enemy->burnout)
	{	PBM_BecomeNewExplosion (self);
		return true;
	}

/* Explode as soon as the entity is gibbed. */
	if (self->enemy->health <= self->enemy->gib_health)
	{	self->enemy->burnout = 0;
		BecomeExplosion2 (self);
		return true;
	}

/* The fire burns itself out after a period of time. */
	if (self->enemy->burnout < level.time)
	{	self->enemy->burnout = 0;
		PBM_BecomeNewExplosion (self);
		return true;
	}

/* Check if the victim found some water. */
	if (PBM_InWater(self->enemy))
	{	self->enemy->burnout = 0;
		PBM_BecomeSteam (self);
		return true;
	}

/* Gaining invulnerability kills the fire immediately. */
	if (self->enemy->client)
		if (self->enemy->client->invincible_framenum > level.framenum)
		{	self->enemy->burnout = 0;
			PBM_BecomeNewExplosion (self);
			return true;
		}

/* Keep on burning! */
		return false;
}

/*-------------------------------------------------------- New Code --------
//  This keeps the fire on an entity burning.
//------------------------------------------------------------------------*/
void PBM_Burn (edict_t *self)
{
	vec3_t   spot;

/* Check if fire should be extinguished. */
	if (PBM_FlameOut(self))
		return;

/* Burn 'em all!  Make fire stick to target. */
	PBM_FireSpot(spot, self->enemy);
	VectorCopy(spot, self->s.origin);
	gi.linkentity(self);

/* Animate the fire. */
	if (++self->s.frame > FRAME_LAST_LARGEFIRE)
		self->s.frame = FRAME_FIRST_LARGEFIRE;

/* Spread the fire! */
	if (self->timestamp < level.time)
	{  PBM_BurnRadius (self, 70, self->pos2, self->enemy);
		PBM_BurnDamage (self->enemy, self, self->pos1);
		self->timestamp = level.time + 1;
	}

	self->nextthink  = level.time + FRAMETIME;
}

/*-------------------------------------------------------- New Code --------
//  This sets the victim on fire.
//------------------------------------------------------------------------*/
void PBM_Ignite (edict_t *victim, edict_t *attacker)
{
	edict_t  *fire;
	vec3_t   spot;
	vec3_t	burn_damage = {10, 5, 0};
	vec3_t	radius_damage = {0, 5, 50};

/* Some entities vulnerable to fire damage can resist burning. */
	if (PBM_FireResistant(victim))  return;

/* If an entity is already burning, replace flame. */
	if (victim->burnout > level.time)
	{  victim->burner->nextthink = level.time + FRAMETIME;
		victim->burner->think = PBM_BecomeNewExplosion;
	}

/* Entity will burn for a period of time. */
	victim->burnout = level.time + 4;

/* Create the fire. */
	PBM_FireSpot(spot, victim);

	fire = G_Spawn();
	fire->s.modelindex = MD2_FIRE;
	fire->s.frame      = FRAME_FIRST_LARGEFIRE;
	VectorClear (fire->mins);
	VectorClear (fire->maxs);
	VectorCopy (spot, fire->s.origin);
	fire->solid        = SOLID_NOT;
	fire->takedamage   = DAMAGE_NO;
	fire->movetype     = MOVETYPE_NONE;
	fire->clipmask     = 0;
	fire->s.effects    = EF_ROCKET;
	fire->s.renderfx   = RF_FULLBRIGHT;
	fire->s.sound      = gi.soundindex ("player/fry.wav");
	fire->owner        = attacker;
	fire->enemy        = victim;
	fire->classname    = "fire";
	fire->timestamp    = level.time + 1;
	fire->nextthink    = level.time + FRAMETIME;
	fire->think        = PBM_Burn;
	VectorCopy (burn_damage, fire->pos1);
	VectorCopy (radius_damage, fire->pos2);

	gi.linkentity(fire);

/* Link victim to fire. */
	victim->burner = fire;
}


/*===========================/  Small Flames  /===========================*/

/*-------------------------------------------------------- New Code --------
//  This checks when to put out the fire.  The fire is put out when
//  the burning entity finds water or when the fire is done burning.
//  The fire can (but does not need to) damage/ignite nearby entities.
//  This is called on every frame.  Used for small flames only.
//------------------------------------------------------------------------*/
void PBM_CheckFire (edict_t *self)
{
/* Put out the fire if it hits water. */
	if (PBM_InWater(self))
	{  PBM_BecomeSteam(self);
		return;
	}

/* The fire burns itself out after a period of time. */
	if (self->burnout < level.time)
	{  PBM_BecomeSmoke(self);
		return;
	}

/* Animate the fire. */
	if (++self->s.frame > FRAME_LAST_SMALLFIRE)
		self->s.frame = FRAME_FIRST_SMALLFIRE;

/* The fire may inflict a small amount of burn damage. */
	if (self->timestamp)
		if (self->timestamp < level.time)
		{
			vec3_t	damage = {3, 0, 5};

			PBM_BurnRadius(self, 50, damage, NULL);
			self->timestamp = level.time + random();
		}

/* Fire is still burning, check again. */
	self->nextthink  = level.time + FRAMETIME;
}

/*-------------------------------------------------------- New Code --------
//  Fire that drops onto an entity may set it on fire (if possible)
//  and explode to damage and possibly ignite nearby targets.
//------------------------------------------------------------------------*/
void PBM_FireDropTouch
(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
/* Explode and ignite. */
	if ( (other->solid != SOLID_BSP) || ((rand() % 100) < self->dmg) )
	{  if (other->takedamage)
			PBM_BurnDamage(other, self, self->pos1);
		PBM_BurnRadius(self, self->dmg_radius, self->pos2, other);

		PBM_BecomeNewExplosion (self);
		return;
	}

/* The fire remains burning. */
	 VectorClear (self->velocity);
	 self->solid = SOLID_NOT;
	 VectorSet (self->mins, -4, -4, 0);
	 VectorSet (self->maxs, 4, 4, 24);
	 gi.linkentity(self);
}

/*-------------------------------------------------------- New Code --------
//  This creates a flame that drops straight down.
//------------------------------------------------------------------------*/
void PBM_FireDrop (edict_t *self)
{
	edict_t *fire;

	fire = G_Spawn();
	fire->s.modelindex = MD2_FIRE;
	fire->s.frame      = FRAME_FIRST_SMALLFIRE;
	VectorClear (fire->mins);
	VectorClear (fire->maxs);
	VectorCopy (self->s.origin, fire->s.origin);
	VectorClear (fire->s.angles);
	VectorClear (fire->velocity);
	fire->movetype     = MOVETYPE_TOSS;
	fire->clipmask     = MASK_SHOT;
	fire->solid        = SOLID_BBOX;
	fire->takedamage   = DAMAGE_NO;
	fire->s.effects    = 0;
	fire->s.renderfx   = RF_FULLBRIGHT;
	fire->s.sound      = gi.soundindex ("player/fry.wav");
	fire->owner        = self->owner;
	fire->classname    = "fire";
	fire->touch        = PBM_FireDropTouch;
	fire->burnout      = level.time + 2 + random() * 3;
	fire->timestamp    = level.time;
	fire->nextthink    = level.time + FRAMETIME;
	fire->think        = PBM_CheckFire;
	VectorCopy (self->pos1, fire->pos1);
	VectorCopy (self->pos1, fire->pos2);
	fire->dmg_radius   = self->dmg_radius;
	fire->dmg          = self->dmg;

	gi.linkentity (fire);
}


/*===========================/  Flame Clouds  /===========================*/

/*--------------------------------------------------------- New Code -------
//  This causes part of the flame cloud to explode and drop flames.
//------------------------------------------------------------------------*/
void PBM_CloudBurst (edict_t *self)
{
	if ((rand() % 100) < self->count)
		PBM_FireDrop(self);

	PBM_BecomeNewExplosion (self);
}

/*--------------------------------------------------------- New Code -------
//  This causes part of the flame cloud to damage as it erupts.
//------------------------------------------------------------------------*/
void PBM_CloudBurstDamage (edict_t *self)
{
	PBM_BurnRadius(self, self->dmg_radius, self->pos2, NULL);
	PBM_CloudBurst(self);
}

/*--------------------------------------------------------- New Code -------
//  This creates a flaming cloud that can rain fire.
//------------------------------------------------------------------------*/
void PBM_FlameCloud
(edict_t *attacker, vec3_t start, vec3_t cloud, vec3_t timer, qboolean deadly, float radius, vec3_t damage, vec3_t radius_damage, int rain_chance, int blast_chance)
{
	vec3_t  spot;
	edict_t  *smoke;
	int  quota = (int)cloud[0];
	int  limit = quota * 4;   /* Max. number of attempts. */
	int  tally = 0;           /* Number of flames created. */
	int  trials;              /* Number of attempts. */

	for (trials = 0; (trials < limit) && (tally < quota); trials++)
	{
		VectorCopy (start, spot);
		spot[0] += ((random() - 0.5) * cloud[1]);
		spot[1] += ((random() - 0.5) * cloud[1]);
		spot[2] += ((random() - 0.5) * cloud[2]);

		if (gi.pointcontents(spot) != MASK_SOLID)
		{
			smoke = G_Spawn();
			/* smoke->s.modelindex = NULL; */
			VectorClear (smoke->mins);
			VectorClear (smoke->maxs);
			VectorCopy (spot, smoke->s.origin);
			smoke->movetype   = MOVETYPE_NONE;
			smoke->solid      = SOLID_NOT;
			smoke->takedamage = DAMAGE_NO;
			smoke->clipmask   = 0;
			smoke->owner      = attacker;
			smoke->enemy      = NULL;
			smoke->classname  = "fire";
			smoke->touch      = NULL;
			smoke->nextthink  = level.time + timer[0] + random() * timer[1];
			if (deadly)
				smoke->think      = PBM_CloudBurstDamage;
			else
				smoke->think      = PBM_CloudBurst;
			VectorCopy (damage, smoke->pos1);
			VectorCopy (radius_damage, smoke->pos2);
			smoke->dmg_radius = radius;

			smoke->count      = rain_chance;
			smoke->dmg        = blast_chance;

			gi.linkentity (smoke);

			tally++;

			if (PBM_InWater(smoke))
				PBM_BecomeSteam (smoke);
		}
	}
}


/*===========================/  Projectiles  /===========================*/

/*--------------------------------------------------------- New Code -------
//  The fireball impacts and delivers pain and flames.
//------------------------------------------------------------------------*/
void PBM_FireballTouch
(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t origin;

	if (PBM_InWater(self))
	{  PBM_BecomeSteam(self);
		return;
	}

	if (surf && (surf->flags & SURF_SKY))
	{  G_FreeEdict (self);
		return;
	}

/* Burn! */
	PBM_BurnRadius(self, self->dmg_radius, self->pos2, other);

	/* calculate position for the explosion entity */
	VectorMA (self->s.origin, -0.02, self->velocity, origin);

	if (other->takedamage)
	{
		if (other->flags & FL_IMMUNE_LAVA)
		{	vec3_t	reduced = {0, 0, 0};
			reduced[1] = self->pos1[1];
			PBM_BurnDamage(other, self, reduced);
		}
		else
			PBM_BurnDamage(other, self, self->pos1);
	}
	else
	{  if ((rand() % 100) < self->count)
			PBM_FireDrop(self);
	}

	PBM_BecomeNewExplosion (self);
}

/*--------------------------------------------------------- New Code -------
//  Create and launch a fireball.
//------------------------------------------------------------------------*/
void PBM_FireFlamer
(edict_t *self, vec3_t start, vec3_t dir, int speed, float radius, vec3_t damage, vec3_t radius_damage, int rain_chance, int blast_chance)
{
	edict_t  *fireball;
	trace_t	tr;

	fireball = G_Spawn();
	fireball->s.modelindex = MD2_FIRE;
	fireball->s.frame      = FRAME_FIRST_SMALLFIRE;
	VectorClear (fireball->mins);
	VectorClear (fireball->maxs);
	VectorCopy (start, fireball->s.origin);
	vectoangles (dir, fireball->s.angles);
	VectorScale (dir, speed, fireball->velocity);
	fireball->s.angles[0] -= 90;
	fireball->movetype     = MOVETYPE_FLYMISSILE;
	fireball->clipmask     = MASK_SHOT;
	fireball->solid        = SOLID_BBOX;
	fireball->takedamage   = DAMAGE_NO;
	fireball->s.effects    = 0;
	fireball->s.renderfx   = RF_FULLBRIGHT;
	fireball->s.sound      = gi.soundindex ("player/fry.wav");
	fireball->owner        = self;
	fireball->classname    = "fire";
	fireball->touch        = PBM_FireballTouch;
	fireball->burnout      = level.time + 6;
	fireball->timestamp    = 0;
	fireball->nextthink    = level.time + FRAMETIME;
	fireball->think        = PBM_CheckFire;
	VectorCopy(damage, fireball->pos1);
	VectorCopy(radius_damage, fireball->pos2);
	fireball->dmg_radius   = radius;
	fireball->count        = rain_chance;
	fireball->dmg          = blast_chance;

	if (self->client)
		check_firedodge (self, fireball->s.origin, dir, speed);

	gi.linkentity (fireball);

	if (PBM_InWater(fireball))
	{	PBM_BecomeSteam(fireball);
		return;
	}

	tr = gi.trace (self->s.origin, NULL, NULL, fireball->s.origin, fireball, MASK_SHOT);
	if (tr.fraction < 1.0)
	{	VectorMA (fireball->s.origin, -10, dir, fireball->s.origin);
		fireball->touch (fireball, tr.ent, NULL, NULL);
	}

}

/*===========================/  END OF FILE  /===========================*/

