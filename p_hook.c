#include "g_local.h"

// constants
#define MIN_CHAIN_LEN		40		// minimum chain length
#define MAX_CHAIN_LEN		1000	// maximum chain length
#define CHAIN_LINK_LEN		55		// length between chain link origins
#define GROW_SHRINK_RATE	40		// units of lengthen/shrink chain in 0.1 sec

// edict->hookstate constants
#define HOOK_ON		0x00000001		// set if hook command is active
#define HOOK_IN		0x00000002		// set if hook has attached
#define SHRINK_ON	0x00000004		// set if shrink chain is active 
#define GROW_ON		0x00000008		// set if grow chain is active


// this is the same as function P_ProjectSource in p_weapons.c except it projects
// the offset distance in reverse since hook is launched with player's free hand
void P_ProjectSource_Reverse (gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
	vec3_t	_distance;

	VectorCopy (distance, _distance);
	if (client->pers.hand == RIGHT_HANDED)
		_distance[1] *= -1;
	else if (client->pers.hand == CENTER_HANDED)
		_distance[1] = 0;
	G_ProjectSource (point, _distance, forward, right, result);
}


void DropHook (edict_t *ent)
{
	// remove all hook flags
	ent->owner->client->hookstate = 0;

	gi.sound (ent->owner, CHAN_AUTO, gi.soundindex("weapons/sshotr1b.wav"), 1, ATTN_IDLE, 0);
	
	// removes hook
	G_FreeEdict (ent);
}


void MaintainLinks (edict_t *ent)
{
	vec3_t pred_hookpos;	// predicted future hook origin
	float multiplier;		// prediction multiplier
	vec3_t norm_hookvel;	// normalized hook velocity

	vec3_t	offset, start;
	vec3_t	forward, right;

// FIXME: add this and use it to make chain not clip in players view
//	vec3_t chainvec;		// vector of the chain 
//	vec3_t chainunit; 		// vector of chain with distance of 1
//	float chainlen;			// length of chain

	// predicts hook's future position since chain links fall behind
	VectorClear (norm_hookvel);
	multiplier = VectorLength(ent->velocity) / 22;
	VectorNormalize2 (ent->velocity, norm_hookvel); 
	VectorMA (ent->s.origin, multiplier, norm_hookvel, pred_hookpos);

	// derive start point of chain
	AngleVectors (ent->owner->client->v_angle, forward, right, NULL);
	VectorSet (offset, 8, 8, ent->owner->viewheight-8);
	P_ProjectSource_Reverse (ent->owner->client, ent->owner->s.origin, offset, forward, right, start);

// FIXME: add this and use it to make chain not clip in players view
	// get info about chain
//	_VectorSubtract (pred_hookpos,start,chainvec);
//	VectorNormalize2 (chainvec, chainunit);
//	VectorMA (chainvec, -18, chainunit, chainvec);
//	chainlen = VectorLength (chainvec);

	// create temp entity chain
	gi.WriteByte (svc_temp_entity);
	if (ctf->value)
	{
		gi.WriteByte (TE_GRAPPLE_CABLE);
		gi.WriteShort (ent->owner - g_edicts);
		gi.WritePosition (ent->owner->s.origin);
		gi.WritePosition (pred_hookpos);
		VectorSubtract (start, ent->owner->s.origin, offset);
		gi.WritePosition (offset);
	}
	else
	{
		gi.WriteByte (TE_MEDIC_CABLE_ATTACK);
		gi.WriteShort (ent - g_edicts);
		gi.WritePosition (pred_hookpos);
		gi.WritePosition (start);
	}
	gi.multicast (ent->s.origin, MULTICAST_PVS);
}


void HookBehavior(edict_t *ent)
{
	vec3_t	offset, start;
	vec3_t	forward, right;
	vec3_t	chainvec;		// chain's vector
	float chainlen;			// length of extended chain
	vec3_t velpart;			// player's velocity component moving to or away from hook
	float f1, f2;			// restrainment forces
	float framestep;		// grow or shrink step per frame
		
	// decide when to disconnect hook
	if ( (!(ent->owner->client->hookstate & HOOK_ON)) ||// if hook has been retracted
	     (ent->enemy->solid == SOLID_NOT) ||			// if target is no longer solid (i.e. hook broke glass; exploded barrels, gibs) 
	     (ent->owner->deadflag) ||						// if player died
	     (ent->owner->s.event == EV_PLAYER_TELEPORT) )	// if player goes through teleport
	{
		DropHook(ent);
		return;
	}

	// gives hook same velocity as the entity it is stuck in
	VectorCopy (ent->enemy->velocity,ent->velocity);

// chain sizing 

	// grow the length of the chain
	if ((ent->owner->client->hookstate & GROW_ON) && (ent->angle < MAX_CHAIN_LEN))
	{
		if (level.time - ent->wait > 0.1) ent->wait = level.time - 0.1;
		framestep = 10 * (level.time - ent->wait) * GROW_SHRINK_RATE;
		ent->angle += framestep;
		if (ent->angle > MAX_CHAIN_LEN) ent->angle = MAX_CHAIN_LEN;
		ent->wait = level.time;

		// trigger climb sound
		if (level.time - ent->delay >= 0.1)
		{
			gi.sound (ent->owner, CHAN_AUTO, gi.soundindex("world/turbine1.wav"), 0.8, ATTN_IDLE, 0);
			ent->delay = level.time;
		}
	}

	// shrink the length of the chain
    if ((ent->owner->client->hookstate & SHRINK_ON) && (ent->angle > MIN_CHAIN_LEN))
	{
		if (level.time - ent->wait > 0.1) ent->wait = level.time - 0.1;
		framestep = 10 * (level.time - ent->wait) * GROW_SHRINK_RATE;
		ent->angle -= framestep;
		if (ent->angle < MIN_CHAIN_LEN) ent->angle = MIN_CHAIN_LEN;
		ent->wait = level.time;

		// trigger slide sound
		if (level.time - ent->delay >= 0.1)		
		{
			gi.sound (ent->owner, CHAN_AUTO, gi.soundindex("world/turbine1.wav"), 0.8, ATTN_IDLE, 0);
			ent->delay = level.time;
		}
	}

// chain physics

	// derive start point of chain
	AngleVectors (ent->owner->client->v_angle, forward, right, NULL);
	VectorSet(offset, 8, 8, ent->owner->viewheight-8);
	P_ProjectSource_Reverse (ent->owner->client, ent->owner->s.origin, offset, forward, right, start);

	// get info about chain
	_VectorSubtract (ent->s.origin, start, chainvec);
	chainlen = VectorLength (chainvec);

	// if player's location is beyond the chain's reach
	if (chainlen > ent->angle)	
	{	 
		// determine player's velocity component of chain vector
		VectorScale (chainvec, _DotProduct (ent->owner->velocity, chainvec) / _DotProduct (chainvec, chainvec), velpart);
		
		// restrainment default force 
		f2 = (chainlen - ent->angle) * 5;

		// if player's velocity heading is away from the hook
		if (_DotProduct (ent->owner->velocity, chainvec) < 0)
		{
			// if chain has streched for 25 units
			if (chainlen > ent->angle + 25)
				// remove player's velocity component moving away from hook
				_VectorSubtract(ent->owner->velocity, velpart, ent->owner->velocity);
			f1 = f2;
		}
		else  // if player's velocity heading is towards the hook
		{
			if (VectorLength (velpart) < f2)
				f1 = f2 - VectorLength (velpart);
			else		
				f1 = 0;
		}
	}
	else
		f1 = 0;
	
    // applys chain restrainment 
	VectorNormalize (chainvec);
	VectorMA (ent->owner->velocity, f1, chainvec, ent->owner->velocity);
	
	MaintainLinks (ent);

	// prep for next think
	ent->nextthink = level.time + FRAMETIME;
}


void HookTouch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t	offset, start;
	vec3_t	forward, right;
	vec3_t	chainvec;		// chain's vector

	// derive start point of chain
	AngleVectors (ent->owner->client->v_angle, forward, right, NULL);
	VectorSet(offset, 8, 8, ent->owner->viewheight-8);
	P_ProjectSource_Reverse (ent->owner->client, ent->owner->s.origin, offset, forward, right, start);

	// member angle is used to store the length of the chain
	_VectorSubtract(ent->s.origin,start,chainvec);
	ent->angle = VectorLength (chainvec);	

	// don't attach hook to sky
	if (surf && (surf->flags & SURF_SKY))
	{
		DropHook (ent);
		return;
	}

	// inflict damage on damageable items
	if (other->takedamage)
	{
		int mod;

		// Set up the means of death.
		mod = MOD_GRAPPLE;
		if ((int)fragban->value & WFB_HOOK)
			mod |= MOD_NOFRAG;

		T_Damage (other, ent, ent->owner, ent->velocity, ent->s.origin,
			plane->normal, ent->dmg, 100, 0, mod);
	}

	if (other->solid == SOLID_BBOX)
	{
		if ((other->svflags & SVF_MONSTER) || (other->client))
			gi.sound (ent, CHAN_VOICE, gi.soundindex("flyer/flyatck2.wav"), 1, ATTN_IDLE, 0);

		DropHook(ent);
		return;
	}
	
	if (other->solid == SOLID_BSP)
	{
		// create puff of smoke
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_SHOTGUN);
		gi.WritePosition (ent->s.origin);
		if (!plane)
			gi.WriteDir (vec3_origin);
		else
			gi.WriteDir (plane->normal);
		gi.multicast (ent->s.origin, MULTICAST_PVS);

		gi.sound (ent, CHAN_VOICE, gi.soundindex("flyer/flyatck1.wav"), 1, ATTN_IDLE, 0);
		VectorClear (ent->avelocity);
	}
	else if (other->solid == SOLID_TRIGGER)
	{
		// debugging line; don't know if this will ever happen 
		gi.cprintf (ent->owner, PRINT_HIGH, "Hook touched a SOLID_TRIGGER\n");
	}
	
	// hook gets the same velocity as the item it attached to
	VectorCopy (other->velocity,ent->velocity);

	// flags hook as being attached to something
	ent->owner->client->hookstate |= HOOK_IN;

	ent->enemy = other;
	ent->touch = NULL;
	ent->think = HookBehavior;
	ent->nextthink = level.time + FRAMETIME;
}


void HookAirborne (edict_t *ent)
{
    vec3_t chainvec;		// chain's vector
	float chainlen;			// length of extended chain

	// get info about chain
	_VectorSubtract (ent->s.origin, ent->owner->s.origin, chainvec);
	chainlen = VectorLength (chainvec);
	
	if ( (!(ent->owner->client->hookstate & HOOK_ON)) || (chainlen > MAX_CHAIN_LEN) )
	{
		DropHook(ent);
		return;
	}

	MaintainLinks (ent);	

	ent->nextthink = level.time + FRAMETIME;
}


void FireHook (edict_t *ent)
{
	edict_t *newhook;
	vec3_t	offset, start;
	vec3_t	forward, right;
	int		damage;

	// determine the damage the hook will inflict
	damage = 10;
	if (ent->client->quad_framenum > level.framenum)
		damage *= 4;
	
	// derive point of hook origin
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 8, 8, ent->viewheight-8);
	P_ProjectSource_Reverse (ent->client, ent->s.origin, offset, forward, right, start);

	// spawn hook
	newhook = G_Spawn();
	VectorCopy (start, newhook->s.origin);
	VectorCopy (forward, newhook->movedir);
	vectoangles (forward, newhook->s.angles);
	VectorScale (forward, 1000, newhook->velocity);
	VectorSet(newhook->avelocity,0,0,-800);
	newhook->movetype = MOVETYPE_FLYMISSILE;
	newhook->clipmask = MASK_SHOT;
	newhook->solid = SOLID_BBOX;
	VectorClear (newhook->mins);
	VectorClear (newhook->maxs);
	if (ctf->value)
		newhook->s.modelindex = gi.modelindex ("models/weapons/grapple/hook/tris.md2");
	else
		newhook->s.modelindex = gi.modelindex ("models/objects/debris2/tris.md2");
	newhook->owner = ent;
	newhook->dmg = damage;

	// wait used to regulate clib and slide rates; tracks time between frames 
	newhook->wait = level.time;  

	// delay used to keep track of how frequent chain noise should occur 
	newhook->delay = level.time;   
    
	// play hook launching sound
	gi.sound (ent, CHAN_AUTO, gi.soundindex ("medic/medatck2.wav"), 1, ATTN_IDLE, 0);
	
	// specify actions to follow 
	newhook->touch = HookTouch;
	newhook->think = HookAirborne;
	newhook->nextthink = level.time + FRAMETIME;
	
	gi.linkentity (newhook);
}


void Cmd_Hook_f (edict_t *ent)
{
	char *s;
	int *hookstate;

	// No grappling hook when you're dead.
	if (ent->deadflag)
		return;

	// Or when you're a ghost.
	if (ctf->value && ent->movetype == MOVETYPE_NOCLIP && ent->solid == SOLID_NOT)
		return;
	
	// Or when you're frozen.
	if (ent->frozen)
		return;

	// Or if the grappling hook has been banned.
	if ((int)featureban->value & FB_HOOK)
		return;

	// get the first hook argument
	s = gi.argv(1);

	// create intermediate value
	hookstate = &ent->client->hookstate;

	if ((!(*hookstate & HOOK_ON)) && (Q_stricmp(s, "action") == 0))
	{
		// flags hook as being active 
		*hookstate = HOOK_ON;   

		FireHook (ent);
		return;
	}

	if  (*hookstate & HOOK_ON)
	{
		// release hook	
		if (Q_stricmp(s, "action") == 0)
		{
			*hookstate = 0;
			return;
		}

// FIXME: put this in when I figure out where the jump key is handled
		// hop of chain and release hook when the following conditions apply
//		if (	(self.button2) && 					// jump is pressed
//				(self.flags & FL_JUMPRELEASED) &&	// previous jump cycle has finished
//				(self.hook & HOOK_IN) &&			// hook is attached
//				(!(self.flags & FL_ONGROUND)) &&	// player not on ground
//				(!(self.flags & FL_INWATER))	)	// player not in water
//		{
//			self.hook = self.hook - (self.hook & HOOK_ON);
//			self.velocity_z = self.velocity_z + 200;
//			sound (self, CHAN_BODY, "player/plyrjmp8.wav", 1, ATTN_NORM);
//			return;
//		}

		// deactivate chain growth or shrink
		if (Q_stricmp(s, "stop") == 0)
		{
			*hookstate -= *hookstate & (GROW_ON | SHRINK_ON);
			return;
		}

		// activate chain growth
		if (Q_stricmp(s, "grow") == 0)
		{
			*hookstate |= GROW_ON;
			*hookstate -= *hookstate & SHRINK_ON;
			return;
		}

		// activate chain shrinking
		if (Q_stricmp(s, "shrink") == 0)
		{
			*hookstate |= SHRINK_ON;		
			*hookstate -= *hookstate & GROW_ON;	
		}
	}
}
