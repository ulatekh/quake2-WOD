#include "g_local.h" 
void ChasecamTrack (edict_t *ent);

/*  The ent is the owner of the chasecam  */
void ChasecamStart (edict_t *ent)
{
	/* This creates a tempory entity we can manipulate within this
	* function */
	edict_t *chasecam;

	/* Tell everything that looks at the toggle that our chasecam is on
	* and working */
	ent->client->chasetoggle = 1;

	/* Make out gun model "non-existent" so it's more realistic to the
	* player using the chasecam */
	ent->client->ps.gunindex = 0;

	chasecam = G_Spawn ();
	chasecam->owner = ent;
	chasecam->solid = SOLID_NOT;
	chasecam->movetype = MOVETYPE_FLYMISSILE;

	/* Now, make the angles of the player model, (!NOT THE HUMAN VIEW!) be
	* copied to the same angle of the chasecam entity */
	VectorCopy (ent->s.angles, chasecam->s.angles);

	/* Clear the size of the entity, so it DOES technically have a size,
	* but that of '0 0 0'-'0 0 0'. (xyz, xyz). mins = Minimum size,
	* maxs = Maximum size */           VectorClear (chasecam->mins);
	VectorClear (chasecam->maxs);

	/* Make the chasecam's origin (position) be the same as the player
	* entity's because as the camera starts, it will force itself out
	* slowly backwards from the player model */
	VectorCopy (ent->s.origin, chasecam->s.origin);        
	chasecam->classname = "chasecam";
	chasecam->nextthink = level.time + FRAMETIME;
	chasecam->think = ChasecamTrack;
	ent->client->chasecam = chasecam;     
	ent->client->oldplayer = G_Spawn();        
}

void ChasecamRestart (edict_t *ent)
{
	/* Keep thinking this function to check all the time whether the
	* player is out of the water */
	ent->nextthink = level.time + FRAMETIME;

	/* If the player is dead, the camera is not wanted... Kill me and stop
	* the function. (return;) */
	if (ent->owner->health <= 0)
	{
		G_FreeEdict (ent);
		return;
	}

	/* If the player is still underwater, break the routine */
	if (ent->owner->waterlevel)
		return;
	
	/* If the player is NOT under water, and not dead, then he's going to
	* want his camera back. Create a new camera, then remove the old one
	* that's not doing anything. We could quite easily 're-instate' the
	* old camera, but I'm lazy :) */
	ChasecamStart (ent->owner);
	G_FreeEdict (ent);
}
/* Here, the "ent" is referring to the client, the player that owns the
* chasecam, and the "opt" string is telling the function whether to
* totally get rid of the camera, or to put it into the background while
* it checks if the player is out of the water or not. The "opt" could
* have easily been a string, and might have used less memory, but it is
* easier to have a string as it is clearer to the reader */
void ChasecamRemove (edict_t *ent, char *opt)
{
	/* Stop the chasecam from moving */
	VectorClear (ent->client->chasecam->velocity);

	/* Make the weapon model of the player appear on screen for 1st
	* person reality and aiming */
	ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);
	
	/* Make our invisible appearance the same model as the display entity
	* that mimics us while in chasecam mode */
	ent->s.modelindex = ent->client->oldplayer->s.modelindex;
	if (!strcmp(opt, "background"))
	{
		ent->client->chasetoggle = 3;
		ent->client->chasecam->nextthink = level.time + FRAMETIME;
		ent->client->chasecam->think = ChasecamRestart;
	}
	else if (!strcmp(opt, "off"))
	{
		ent->client->chasetoggle = 0;
		G_FreeEdict (ent->client->oldplayer);
		G_FreeEdict (ent->client->chasecam);
	}
}
/* The "ent" is the chasecam */
void ChasecamTrack (edict_t *ent)
{
	/* Create tempory vectors and trace variables */
	trace_t tr;
	vec3_t spot1, spot2, dir;
	vec3_t forward, right, up;
	int distance;
	int tot;

	ent->nextthink = level.time + FRAMETIME;

	// Update the held weapon.
	if (ent->owner->client->ps.gunindex)
	{
		ent->client->ps.gunindex = ent->owner->client->ps.gunindex;
		ent->owner->client->ps.gunindex = 0;
	}

	/* if our owner is under water, run the remove routine to repeatedly
	* check for emergment from water */
	//  if (ent->owner->waterlevel)           {
	//     ChasecamRemove (ent, "background");              return;
	//  }
	
	/* get the CLIENT's angle, and break it down into direction vectors,
	* of forward, right, and up. VERY useful */
	AngleVectors (ent->owner->client->v_angle, forward, right, up);

	/* go starting at the player's origin, forward, ent->chasedist1
	* distance, and save the location in vector spot2 */
	VectorMA (ent->owner->s.origin, -ent->chasedist1, forward, spot2);

	/* make spot2 a bit higher, but adding 40 to the Z coordinate */
	spot2[2] = (spot2[2] + 40);

	/* if the client is looking down, do backwards up into the air, 0.6
	* to the ratio of looking down, so the crosshair is still roughly
	* aiming at where the player is aiming. */
	if (ent->owner->client->v_angle[0] < 0.000)
		VectorMA (spot2, -(ent->owner->client->v_angle[0] * 0.6), up, spot2);
	
		/* if the client is looking up, do the same, but do DOWN rather than
	* up, so the camera is behind the player aiming in a similar dir */
	else if (ent->owner->client->v_angle[0] > 0.000)
		VectorMA (spot2, (ent->owner->client->v_angle[0] * 0.6), up, spot2);
	
	/* make the tr traceline trace from the player model's position, to spot2,
	* ignoring the player, with no masks. */
	tr = gi.trace (ent->owner->s.origin, NULL, NULL, spot2, ent->owner, false);
	
	/* subtract the endpoint from the start point for length and
	* direction manipulation */
	VectorSubtract (tr.endpos, ent->owner->s.origin, spot1);
	
	/* in this case, length */
	ent->chasedist1 = VectorLength (spot1);        
	
	/* go, starting from the end of the trace, 2 points forward (client
	* angles) and save the location in spot2 */
	VectorMA (tr.endpos, 2, forward, spot2);
	
	/* make spot1 the same for tempory vector modification and make spot1
	* a bit higher than spot2 */           VectorCopy (spot2, spot1);
	spot1[2] += 64;
	
	/* another trace from spot2 to spot2, ignoring player, no masks */
	tr = gi.trace (spot2, NULL, NULL, spot1, ent->owner, false);
	
	/* if we hit something, copy the trace end to spot2 and lower spot2 */
	if (tr.fraction < 1.000)
	{
		VectorCopy (tr.endpos, spot2);
		spot2[2] -= 32;
	}

	/* subtract endpos spot2 from startpos the camera origin, saving it to
	* the dir vector, and normalize dir for a direction from the camera
	* origin, to the spot2 */
	VectorSubtract (spot2, ent->s.origin, dir);
	VectorNormalize (dir);        
	
	/* subtract the same things, but save it in spot1 for a temporary
	* length calculation */
	VectorSubtract (spot2, ent->s.origin, spot1);
	distance = VectorLength (spot1);        
	
	/* another traceline */
	tr = gi.trace (ent->s.origin, NULL, NULL, spot2, ent->owner, false);
	
	/* if we DON'T hit anyting, do some freaky stuff  */
	if (tr.fraction == 1.000)
	{
		/* subtract the endpos camera position, from the startpos, the
		* player, and save in spot1. Normalize spot1 for a direction, and
		* make that direction the angles of the chasecam for copying to the
		* clients view angle which is displayed to the client. (human) */
		VectorSubtract (ent->s.origin, ent->owner->s.origin, spot1);
		VectorNormalize (spot1);
		VectorCopy (spot1, ent->s.angles);        
		
		/* calculate the percentages of the distances, and make sure we're
		* not going too far, or too short, in relation to our panning
		* speed of the chasecam entity */
		tot = (distance * 0.400);
		
		/* if we're going too fast, make us top speed */
		if (tot > 5.200)
		{
			ent->velocity[0] = ((dir[0] * distance) * 5.2);
			ent->velocity[1] = ((dir[1] * distance) * 5.2);
			ent->velocity[2] = ((dir[2] * distance) * 5.2);
		}

		/* if we're NOT going top speed, but we're going faster than
		* 1, relative to the total, make us as fast as we're going */
		else if (tot > 1.000)
		{
			ent->velocity[0] = ((dir[0] * distance) * tot);
			ent->velocity[1] = ((dir[1] * distance) * tot);
			ent->velocity[2] = ((dir[2] * distance) * tot);        
		}
		else
		{
			/* if we're not going faster than one, don't accelerate our
			* speed at all, make us go slow to our destination */
			ent->velocity[0] = (dir[0] * distance);
			ent->velocity[1] = (dir[1] * distance);
			ent->velocity[2] = (dir[2] * distance);        
		}

		/* subtract endpos;player position, from chasecam position to get
		* a length to determine whether we should accelerate faster from
		* the player or not */
		VectorSubtract (ent->owner->s.origin, ent->s.origin, spot1);
		if (VectorLength(spot1) < 20)
		{
			ent->velocity[0] *= 2;
			ent->velocity[1] *= 2; 
			ent->velocity[2] *= 2;
		}        
	}

	/* if we DID hit something in the tr.fraction call ages back, then
	* make the spot2 we created, the position for the chasecamera. */
	else
		VectorCopy (spot2, ent->s.origin);

	/* add to the distance between the player and the camera */
	ent->chasedist1 += 2;

	/* if we're too far away, give us a maximum distance */
	if (ent->chasedist1 > 60.00)
		ent->chasedist1 = 60.000;

	/* if we haven't gone anywhere since the last think routine, and we
	* are greater than 20 points in the distance calculated, add one to
	* the second chasedistance variable
	* The "ent->movedir" is a vector which is not used in this entity, so
	* we can use this a tempory vector belonging to the chasecam, which
	* can be carried through think routines. */
	if (ent->movedir == ent->s.origin)
	{
		if (distance > 20)
		ent->chasedist2++;
	}

	/* if we've buggered up more than 3 times, there must be some mistake,
	* so restart the camera so we re-create a chasecam, destroy the old one,
	* slowly go outwards from the player, and keep thinking this routing in
	* the new camera entity */
	if (ent->chasedist2 > 3)
	{
		ChasecamStart (ent->owner);
		G_FreeEdict(ent);
		return;
	}

	/* Copy the position of the chasecam now, and stick it to the movedir
	* variable, for position checking when we rethink this function */
	VectorCopy (ent->s.origin, ent->movedir);
}

void Cmd_Chasecam_Toggle (edict_t *ent)
{
	if (ent->client->chasetoggle)
		ChasecamRemove (ent, "off");
	else
		ChasecamStart (ent);
}

void CheckChasecam_Viewent (edict_t *ent)
{
	if ((ent->client->chasetoggle == 1) && (ent->client->oldplayer))
	{
		ent->client->oldplayer->s.frame = ent->s.frame;

		/* Copy the origin, the speed, and the model angle, NOT
		* literal angle to the display entity */
		VectorCopy (ent->s.origin, ent->client->oldplayer->s.origin);
		VectorCopy (ent->velocity, ent->client->oldplayer->velocity);
		VectorCopy (ent->s.angles, ent->client->oldplayer->s.angles);

		/* Make sure we are using the same model + skin as selected,
		* as well as the weapon model the player model is holding.
		* For customized deathmatch weapon displaying, you can
		* use the modelindex2 for different weapon changing, as you
		* can read in forthcoming tutorials */
		ent->client->oldplayer->s.modelindex = ent->s.modelindex;
		ent->client->oldplayer->s.modelindex2 = ent->s.modelindex2;
		ent->s.effects = RF_TRANSLUCENT;

		gi.linkentity (ent->client->oldplayer);
	}
}
