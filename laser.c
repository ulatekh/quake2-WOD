#include "g_local.h"

static void weapon_laser_think (edict_t *self)
{
	// If the laser has expired, get rid of it.
	if (level.time > self->delay)
	{ 
		// bit of damage 
		T_RadiusDamage (self, self, LASER_MOUNT_DAMAGE_RADIUS, NULL,
			LASER_MOUNT_DAMAGE, MOD_TRIPWIRE);
		
		// BANG ! 
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_EXPLOSION1);
		gi.WritePosition(self -> s.origin);
		gi.multicast (self->s.origin, MULTICAST_PVS);
		
		// bye bye laser
		G_FreeEdict (self);
		return;
	}

	// Now do the rest of the laser stuff.
	target_laser_think (self);
}

static void pre_weapon_laser_think (edict_t *self)
{
	target_laser_on (self);
	self->think = weapon_laser_think;
}

void
PlaceLaser (edict_t *ent)
{
	edict_t *self, *grenade;
	vec3_t forward, wallp;
	trace_t tr;


	int  laser_colour[] =
	{
		0xf2f2f0f0,             // red
		0xd0d1d2d3,             // green
		0xf3f3f1f1,             // blue
		0xdcdddedf,             // yellow
		0xe0e1e2e3              // bitty yellow strobe
	};

	// valid ent ?
	if ((!ent->client) || (ent->health <= 0))
		return;

	// cells for laser ?
	if (ent->client->pers.inventory[ITEM_INDEX(&gI_ammo_cells)] < CELLS_FOR_LASER)
	{
		gi.cprintf(ent, PRINT_HIGH, "Not enough cells to set the Laser Tripwire.\n");
		return;
	}

	// Setup "little look" to close wall
	VectorCopy(ent->s.origin,wallp);         

	// Cast along view angle
	AngleVectors (ent->client->v_angle, forward, NULL, NULL);

	// Setup end point
	wallp[0]=ent->s.origin[0]+forward[0]*50;
	wallp[1]=ent->s.origin[1]+forward[1]*50;
	wallp[2]=ent->s.origin[2]+forward[2]*50;  

	// trace
	tr = gi.trace (ent->s.origin, NULL, NULL, wallp, ent, MASK_SOLID);

	// Line complete ? (ie. no collision)
	if (tr.fraction == 1.0)
	{
		gi.cprintf (ent, PRINT_HIGH, "Too far from wall.\n");
		return;
	}

	// Hit sky ?
	if (tr.surface && tr.surface->flags & SURF_SKY)
		return;

	// Ok, lets stick one on then ...
	gi.cprintf (ent, PRINT_HIGH, "Laser Tripwire attached.\n");

	// Deduct the ammo.
	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ITEM_INDEX(&gI_ammo_cells)] -= CELLS_FOR_LASER;
	
	// -----------
	// Setup laser
	// -----------
	self = G_Spawn();

	self -> movetype                = MOVETYPE_NONE;
	self -> solid                   = SOLID_NOT;
	self -> s.renderfx              = RF_BEAM|RF_TRANSLUCENT;
	self -> s.modelindex            = 1;                    // must be non-zero
	self -> s.sound                 = gi.soundindex ("world/laser.wav");
	self -> classname               = "weapon_triplaser";
	self -> s.frame                 = 2;    // beam diameter
	self -> owner                   = self;
	self -> s.skinnum               = laser_colour[((int) (random() * 1000)) % 5];
	self -> dmg                     = 50;
	self -> think                   = pre_weapon_laser_think;
	self -> delay                   = level.time + LASER_TIME;

	// Set orgin of laser to point of contact with wall
	VectorCopy(tr.endpos,self->s.origin);

	// convert normal at point of contact to laser angles
	vectoangles(tr.plane.normal,self -> s.angles);

	// setup laser movedir (projection of laser)
	G_SetMovedir (self->s.angles, self->movedir);

	VectorSet (self->mins, -8, -8, -8);
	VectorSet (self->maxs, 8, 8, 8);

	// link to world
	gi.linkentity (self);

	// start off ...
	target_laser_off (self);

	// ... but make automatically come on
	self -> nextthink = level.time + 2;
	grenade = G_Spawn();

	VectorClear (grenade->mins);
	VectorClear (grenade->maxs);
	VectorCopy (tr.endpos, grenade->s.origin);
	vectoangles(tr.plane.normal,grenade -> s.angles);

	grenade -> movetype         = MOVETYPE_NONE;
	grenade -> clipmask         = MASK_SHOT;
	grenade -> solid            = SOLID_NOT;
	grenade -> s.modelindex     = gi.modelindex ("models/objects/grenade2/tris.md2");
	grenade -> owner            = self;
	grenade -> nextthink        = level.time + LASER_TIME;
	grenade -> think            = G_FreeEdict;

	gi.linkentity (grenade);
}
