#include "g_local.h"
#include "m_player.h"

char *ClientTeam (edict_t *ent)
{
	char		*p;
	static char	value[512];

	value[0] = 0;

	if (!ent->client)
		return value;

	strcpy(value, Info_ValueForKey (ent->client->pers.userinfo, "skin"));
	p = strchr(value, '/');
	if (!p)
		return value;

	if ((int)(dmflags->value) & DF_MODELTEAMS)
	{
		*p = 0;
		return value;
	}

	// if ((int)(dmflags->value) & DF_SKINTEAMS)
	return ++p;
}

qboolean OnSameTeam (edict_t *ent1, edict_t *ent2)
{
	char	ent1Team [512];
	char	ent2Team [512];

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
	{
		if (teamplay->value
		&& ent1->client && ent2->client
		&& ent1->client->resp.ctf_team == ent2->client->resp.ctf_team)
			return true;
		else
			return false;
	}

	strcpy (ent1Team, ClientTeam (ent1));
	strcpy (ent2Team, ClientTeam (ent2));

	if (strcmp(ent1Team, ent2Team) == 0)
		return true;
	return false;
}


void SelectNextItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	unsigned int i, index;
	gitem_t		*it;

	cl = ent->client;

//ZOID
	if (cl->menu)
	{
		PMenu_Next(ent);
		return;
	}
	else if (cl->chase_target)
	{
		ChaseNext(ent);
		return;
	}
//ZOID

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		// index = (cl->pers.selected_item + i)%MAX_ITEMS;
		index = (((unsigned)cl->pers.selected_item) + i) & (MAX_ITEMS-1);
		if (!cl->pers.inventory[index])
			continue;
		it = itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void SelectPrevItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	unsigned int i, index;
	gitem_t		*it;

	cl = ent->client;

//ZOID
	if (cl->menu)
	{
		PMenu_Prev(ent);
		return;
	}
	else if (cl->chase_target)
	{
		ChasePrev(ent);
		return;
	}
//ZOID

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		// index = (cl->pers.selected_item + MAX_ITEMS - i)%MAX_ITEMS;
		index = (((unsigned)cl->pers.selected_item) + MAX_ITEMS - i)
			& (MAX_ITEMS-1);
		if (!cl->pers.inventory[index])
			continue;
		it = itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void ValidateSelectedItem (edict_t *ent)
{
	gclient_t	*cl;

	cl = ent->client;

	if (cl->pers.inventory[cl->pers.selected_item])
		return;		// valid

	SelectNextItem (ent, -1);
}


//=================================================================================

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (edict_t *ent)
{
	char		*name;
	gitem_t		*it;
	int			index;
	int			i;
	qboolean	give_all;
	edict_t		*it_ent;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	name = gi.args();

	if (Q_stricmp(name, "all") == 0)
		give_all = true;
	else
		give_all = false;

	if (give_all || Q_stricmp(gi.argv(1), "health") == 0)
	{
		if (gi.argc() == 3)
			ent->health = atoi(gi.argv(2));
		else
			ent->health = ent->max_health;
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist[i];
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_WEAPON))
				continue;
			ent->client->pers.inventory[i] += 1;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist[i];
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_AMMO))
				continue;
			Add_Ammo (ent, it, 1000);
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		gitem_armor_t	*info;

		it = &gI_item_armor_jacket;
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = &gI_item_armor_combat;
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = &gI_item_armor_body;
		info = (gitem_armor_t *)it->info;
		ent->client->pers.inventory[ITEM_INDEX(it)] = info->max_count;

		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "Power Shield") == 0)
	{
		it = &gI_item_power_shield;
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);

		if (!give_all)
			return;
	}

	if (give_all)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist[i];
			if (!it->pickup)
				continue;
			if (it->flags & (IT_ARMOR|IT_WEAPON|IT_AMMO))
				continue;
			ent->client->pers.inventory[i] = 1;
		}
		return;
	}

	it = FindItem (name);
	if (!it)
	{
		name = gi.argv(1);
		it = FindItem (name);
		if (!it)
		{
			gi.cprintf (ent, PRINT_HIGH, "unknown item\n");
			return;
		}
	}

	if (!it->pickup)
	{
		gi.cprintf (ent, PRINT_HIGH, "non-pickup item\n");
		return;
	}

	index = ITEM_INDEX(it);

	if (it->flags & IT_AMMO)
	{
		if (gi.argc() == 3)
			ent->client->pers.inventory[index] = atoi(gi.argv(2));
		else
			ent->client->pers.inventory[index] += it->quantity;
	}
	else
	{
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		ent->movetype = MOVETYPE_WALK;
		msg = "noclip OFF\n";
	}
	else
	{
		ent->movetype = MOVETYPE_NOCLIP;
		if (ent->thirdperson) ThirdEnd(ent);
		msg = "noclip ON\n";
	}

	gi.cprintf (ent, PRINT_HIGH, msg);
}

/*
=====
cmd_kamikaze_f
=====
*/

void Cmd_Kamikaze_f (edict_t *ent)
{
	// No kamikazes when you're dead.
	if (ent->deadflag)
		return;

	// Or when you're a ghost.
	if (teamplay->value
	&& ent->movetype == MOVETYPE_NOCLIP
	&& ent->solid == SOLID_NOT)
		return;
	
	// Or if you're frozen.
	if (ent->frozen)
		return;

	// Or if kamikazes have been banned.
	if ((int)featureban->value & FB_KAMIKAZE)
		return;
	
	Start_Kamikaze_Mode (ent);
}

/*
=================
Cmd_Third_f
=================
*/
void Cmd_Third_f (edict_t *ent)
{
	if (ent->deadflag)
		return;

	// The chasecam either needs to be reconciled with VWep, or all the
	// chasecam code needs to be replaced with something better.
	gi.cprintf (ent, PRINT_HIGH, "ChaseCam removed for debugging. Sorry.\n");
	return;

   if (ent->movetype == MOVETYPE_NOCLIP)
	{
		gi.cprintf (ent, PRINT_HIGH, "Can't be in Chasecam Mode and noclip!\n");
		return;
	}
	else if (gi.pointcontents (ent->s.origin) & CONTENTS_SOLID)
	{
		gi.cprintf (ent, PRINT_HIGH, "Can't activate Chasecam Mode when in a solid!\n");
		return;
	}
	else
	{
		ent->thirdperson = 1 - ent->thirdperson;
		if (ent->thirdperson)
			ThirdBegin(ent);
		else
			ThirdEnd(ent);
	}
}
/*
=================
Cmd_ThirdX_f
=================
*/
void Cmd_ThirdX_f (edict_t *ent)
{
	if (gi.argc() == 1)
	{
		gi.cprintf(ent, PRINT_HIGH, va("%f\n", ent->thirdoffx));
		return;
	}
	ent->thirdoffx = atof (gi.argv(1));
}
/*
=================
Cmd_ThirdZ_f
=================
*/
void Cmd_ThirdZ_f (edict_t *ent)
{
	if (gi.argc() == 1)
	{
		gi.cprintf(ent, PRINT_HIGH, va("%f\n", ent->thirdoffz));
		return;
	}
	ent->thirdoffz = atof (gi.argv(1));
}

/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
void Cmd_Use_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}

	// Handle weapon selection specially, to account for alternate weapons as
	// well as weapon banning.  Basically, if they're already holding the normal
	// weapon and they have the alternate in their inventory, or if they don't
	// have the normal weapon, select the alternate.
	if (it == &gI_weapon_blaster)
	{
		if ((it == ent->client->pers.weapon
		&& ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_superblaster)])
		|| (it != ent->client->pers.weapon
		&& !ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_blaster)]))
			it = &gI_weapon_superblaster;
	}
	else if (it == &gI_weapon_shotgun)
	{
		if ((it == ent->client->pers.weapon
		&& ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_sniper)])
		|| (it != ent->client->pers.weapon
		&& (!ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_shotgun)]
			|| (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_shotgun.ammo)]
				< gI_weapon_shotgun.quantity
					&& !g_select_empty->value))))
			it = &gI_weapon_sniper;
	}
	else if (it == &gI_weapon_supershotgun)
	{
		if ((it == ent->client->pers.weapon
		&& ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_freezer)])
		|| (it != ent->client->pers.weapon
		&& (!ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_supershotgun)]
			|| (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_supershotgun.ammo)]
				< gI_weapon_supershotgun.quantity
					&& !g_select_empty->value))))
			it = &gI_weapon_freezer;
	}
	else if (it == &gI_weapon_machinegun)
	{
		if ((it == ent->client->pers.weapon
		&& ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_machine)])
		|| (it != ent->client->pers.weapon
		&& (!ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_machinegun)]
			|| (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_machinegun.ammo)]
				< gI_weapon_machinegun.quantity
					&& !g_select_empty->value))))
			it = &gI_weapon_machine;
	}
	else if (it == &gI_weapon_chaingun)
	{
		if ((it == ent->client->pers.weapon
		&& ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_streetsweeper)])
		|| (it != ent->client->pers.weapon
		&& (!ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_chaingun)]
			|| (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_chaingun.ammo)]
				< gI_weapon_chaingun.quantity
					&& !g_select_empty->value))))
			it = &gI_weapon_streetsweeper;
	}
	else if (it == &gI_weapon_grenadelauncher)
	{
		if ((it == ent->client->pers.weapon
		&& ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_bazooka)])
		|| (it != ent->client->pers.weapon
		&& (!ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_grenadelauncher)]
			|| (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_grenadelauncher.ammo)]
				< gI_weapon_grenadelauncher.quantity
					&& !g_select_empty->value))))
			it = &gI_weapon_bazooka;
	}
	else if (it == &gI_weapon_rocketlauncher)
	{
		if ((it == ent->client->pers.weapon
		&& ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_guidedrocketlauncher)])
		|| (it != ent->client->pers.weapon
		&& (!ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_rocketlauncher)]
			|| (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_rocketlauncher.ammo)]
				< gI_weapon_rocketlauncher.quantity
					&& !g_select_empty->value))))
			it = &gI_weapon_guidedrocketlauncher;
	}
	else if (it == &gI_weapon_hyperblaster)
	{
		if ((it == ent->client->pers.weapon
		&& ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_plasma)])
		|| (it != ent->client->pers.weapon
		&& (!ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_hyperblaster)]
			|| (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_hyperblaster.ammo)]
				< gI_weapon_hyperblaster.quantity
					&& !g_select_empty->value))))
			it = &gI_weapon_plasma;
	}
	else if (it == &gI_weapon_railgun)
	{
		if ((it == ent->client->pers.weapon
		&& ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_railgun2)])
		|| (it != ent->client->pers.weapon
		&& (!ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_railgun)]
			|| (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_railgun.ammo)]
				< gI_weapon_railgun.quantity
					&& !g_select_empty->value))))
			it = &gI_weapon_railgun2;
	}
	else if (it == &gI_ammo_grenades)
	{
		// If grenades are already selected, move to the next type.
		if (it == ent->client->pers.weapon)
		{
			// Move to the next grenade type, skipping the ones that are not in
			// our inventory.
			do
			{
				ent->client->dM_grenade += 1;
				it = itemlist[ITEM_INDEX(&gI_ammo_grenades)
					+ ent->client->dM_grenade];
			} while (it != &gI_weapon_grenadelauncher
				&& !ent->client->pers.inventory[ITEM_INDEX(it)]);
			
			// If we've gone past the last type, wrap around to the beginning.
			if (it == &gI_weapon_grenadelauncher)
			{
				ent->client->dM_grenade = 0;
				it = &gI_ammo_grenades;
			}
		}

		// If grenades were not already selected, restore their old choice.
		else
			it = itemlist[ITEM_INDEX(&gI_ammo_grenades) + ent->client->dM_grenade];
	}

	// test if they have item
	index = ITEM_INDEX (it);
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	// Print what they selected.  Some of the normal weapons have been replaced
	// by the alternates, and all the alternates need to be printed.  Also, we
	// already printed the message for the grenades.
	if (it == &gI_weapon_machinegun)
	{
		gi.cprintf (ent, PRINT_MEDIUM, "Machine Rocket Gun\n");
	}
	else if (it == &gI_weapon_grenadelauncher)
	{
		if (ent->client->dM_grenade == 0)
			gi.cprintf (ent, PRINT_MEDIUM, "Fire Grenade Launcher\n");
		else
			gi.cprintf (ent, PRINT_MEDIUM, "%s Launcher\n",
				itemlist[ITEM_INDEX(&gI_ammo_grenades) + ent->client->dM_grenade]
					->pickup_name);
	}
	else if (it == &gI_weapon_bazooka)
	{
		if (ent->client->dM_grenade == 0)
			gi.cprintf (ent, PRINT_MEDIUM, "Bazooka\n");
		else
			gi.cprintf (ent, PRINT_MEDIUM, "%s Bazooka\n",
				itemlist[ITEM_INDEX(&gI_ammo_grenades) + ent->client->dM_grenade]
					->pickup_name);
	}
	else if (it == &gI_weapon_rocketlauncher)
	{
		gi.cprintf (ent, PRINT_MEDIUM, "Flame Rocket Launcher\n");
	}
	else if (it == &gI_weapon_railgun)
	{
		gi.cprintf (ent, PRINT_MEDIUM, "Flamethrower\n");
	}
	else if (it == &gI_weapon_railgun2)
	{
		gi.cprintf (ent, PRINT_MEDIUM, "Railgun\n");
	}
	else if (it == &gI_ammo_grenades)
	{
		if (ent->client->dM_grenade == 0)
			gi.cprintf (ent, PRINT_MEDIUM, "Standard Grenade\n");
		else
			gi.cprintf (ent, PRINT_MEDIUM, "%s\n",
				itemlist[ITEM_INDEX(&gI_ammo_grenades) + ent->client->dM_grenade]
					->pickup_name);
	}
	else if (it->pickup_name)
	{
		// Show them the pickup name.
		gi.cprintf (ent, PRINT_MEDIUM, "%s\n", it->pickup_name);
	}

	// Try to use it.
	it->use (ent, it);
}

/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
void Cmd_Drop_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->drop (ent, it);
}


/*
=================
Cmd_Inven_f
=================
*/
void Cmd_Inven_f (edict_t *ent)
{
	int			i;
	gclient_t	*cl;

	cl = ent->client;

	cl->showscores = false;
	cl->showhelp = false;

	// If the menu's up, close it: that's the end of the cycle.
	if (ent->client->menu)
	{
		PMenu_Close (ent);
		ent->client->update_chase = true;
		return;
	}

	// If this is a teamplay game and they just closed the inventory window,
	// show the menu.
	if (teamplay->value
	&& (cl->showinventory || cl->resp.ctf_team == CTF_NOTEAM))
	{
		cl->showinventory = false;
		CTFOpenJoinMenu(ent);
		return;
	}

	// If the inventory's up, close it.
	if (cl->showinventory)
	{
		cl->showinventory = false;
		return;
	}

	// Otherwise, we're gonna show the inventory window.
	cl->showinventory = true;

	gi.WriteByte (svc_inventory);
	for (i=0 ; i<MAX_ITEMS ; i++)
	{
		gi.WriteShort (cl->pers.inventory[i]);
	}
	gi.unicast (ent, true);

	if (cl->pers.scanner_active & 1)
		cl->pers.scanner_active = 2;
}

/*
=================
Cmd_InvUse_f
=================
*/
void Cmd_InvUse_f (edict_t *ent)
{
	gitem_t		*it;

//ZOID
	if (ent->client->menu) {
		PMenu_Select(ent);
		return;
	}
//ZOID

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to use.\n");
		return;
	}

	it = itemlist[ent->client->pers.selected_item];
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	it->use (ent, it);
}

//ZOID
/*
=================
Cmd_LastWeap_f
=================
*/
void Cmd_LastWeap_f (edict_t *ent)
{
	gclient_t	*cl;

	cl = ent->client;

	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	cl->pers.lastweapon->use (ent, cl->pers.lastweapon);
}
//ZOID

/*
=================
Cmd_WeapPrev_f
=================
*/
void Cmd_WeapPrev_f (edict_t *ent)
{
	gclient_t	*cl;
	unsigned int i, index;
	gitem_t		*it;
	unsigned int selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ((unsigned)(ITEM_INDEX(cl->pers.weapon)));

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		// index = (selected_weapon + i)%MAX_ITEMS;
		index = (selected_weapon + i) & (MAX_ITEMS-1);
		if (!cl->pers.inventory[index])
			continue;
		it = itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->newweapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapNext_f
=================
*/
void Cmd_WeapNext_f (edict_t *ent)
{
	gclient_t	*cl;
	unsigned int i, index;
	gitem_t		*it;
	unsigned int selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ((unsigned)(ITEM_INDEX (cl->pers.weapon)));

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		// index = (selected_weapon + MAX_ITEMS - i)%MAX_ITEMS;
		index = (selected_weapon + MAX_ITEMS - i) & (MAX_ITEMS-1);
		if (!cl->pers.inventory[index])
			continue;
		it = itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->newweapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
void Cmd_WeapLast_f (edict_t *ent)
{
	gclient_t	*cl;
	int			index;
	gitem_t		*it;

	cl = ent->client;

	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	index = ITEM_INDEX(cl->pers.lastweapon);
	if (!cl->pers.inventory[index])
		return;
	it = itemlist[index];
	if (!it->use)
		return;
	if (! (it->flags & IT_WEAPON) )
		return;
	it->use (ent, it);
}

/*
=================
Cmd_InvDrop_f
=================
*/
void Cmd_InvDrop_f (edict_t *ent)
{
	gitem_t		*it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to drop.\n");
		return;
	}

	it = itemlist[ent->client->pers.selected_item];
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	it->drop (ent, it);
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f (edict_t *ent)
{
//ZOID
	if (ent->solid == SOLID_NOT)
		return;
//ZOID

	// Don't allow frozen people to kill themselves -- people use this to cheat
	// inflictors out of a frag.
	if (ent->frozen)
		return;

	// Don't let them kill themselves more often than every 5 seconds.
	if ((level.time - ent->client->respawn_time) < 5)
		return;

	ent->flags &= ~FL_GODMODE;
	ent->health = 0;
	ent->s.effects = 0;
	ent->s.renderfx = 0;
	meansOfDeath = MOD_SUICIDE;
	player_die (ent, ent, ent, 100000, vec3_origin);
}

/*
=================
Cmd_PutAway_f
=================
*/
void Cmd_PutAway_f (edict_t *ent)
{
	ent->client->showscores = false;
	ent->client->showhelp = false;
	ent->client->showinventory = false;
//ZOID
	if (ent->client->menu)
		PMenu_Close(ent);
	ent->client->update_chase = true;
//ZOID
}


int PlayerSort (void const *a, void const *b)
{
	int		anum, bnum;

	anum = *(int *)a;
	bnum = *(int *)b;

	anum = game.clients[anum].ps.stats[STAT_FRAGS];
	bnum = game.clients[bnum].ps.stats[STAT_FRAGS];

	if (anum < bnum)
		return -1;
	if (anum > bnum)
		return 1;
	return 0;
}

/*
=================
Cmd_Players_f
=================
*/
void Cmd_Players_f (edict_t *ent)
{
	int		i;
	int		count;
	char	small[64];
	char	large[1280];
	int		index[256];

	count = 0;
	for (i = 0 ; i < maxclients->value ; i++)
		if (game.clients[i].pers.connected && (g_edicts + i + 1)->inuse)
		{
			index[count] = i;
			count++;
		}

	// sort by frags
	qsort (index, count, sizeof(index[0]), PlayerSort);

	// print information
	large[0] = 0;

	for (i = 0 ; i < count ; i++)
	{
		Com_sprintf (small, sizeof(small), "%3i %s\n",
			game.clients[index[i]].ps.stats[STAT_FRAGS],
			game.clients[index[i]].pers.netname);
		if (strlen (small) + strlen(large) > sizeof(large) - 100 )
		{	// can't print all of them in one packet
			strcat (large, "...\n");
			break;
		}
		strcat (large, small);
	}

	gi.cprintf (ent, PRINT_HIGH, "%s\n%i players\n", large, count);
}

// The distance at which push/pull works.
static const int kPushPullRange = 1000;

/*
=================
Cmd_Push_f
=================
*/
void Cmd_Push_f (edict_t *ent)
{
	vec3_t	start;
	vec3_t	forward;
	vec3_t	end;
	vec3_t	extent;
	vec_t		strength;
	trace_t	tr;

	// No pushing when you're dead.
	if (ent->deadflag)
		return;

	// Or when you're a ghost.
	if (teamplay->value
	&& ent->movetype == MOVETYPE_NOCLIP
	&& ent->solid == SOLID_NOT)
		return;

	// Or if you're frozen.
	if (ent->frozen)
		return;

	// Or if push/pull has been banned.
	if ((int)featureban->value & FB_PUSHPULL)
		return;

	VectorCopy (ent->s.origin, start);
	start[2] += ent->viewheight - 8;
	AngleVectors (ent->client->v_angle, forward, NULL, NULL);
	VectorMA (start, kPushPullRange, forward, end);
	tr = gi.trace (start, NULL, NULL, end, ent, MASK_SHOT);
	if (tr.ent
	&& ((tr.ent->svflags & SVF_MONSTER)
		/* || (tr.ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)) */
		|| (tr.ent->client)))
	{
		// Show a trail from the pusher to the pushee.
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BUBBLETRAIL);
		gi.WritePosition (start);
		gi.WritePosition (tr.endpos);
		gi.multicast (ent->s.origin, MULTICAST_PHS);
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BFG_LASER);
		gi.WritePosition (start);
		gi.WritePosition (tr.endpos);
		gi.multicast (ent->s.origin, MULTICAST_PHS);

		// Have the pusher emit a sound.
		gi.sound (ent, CHAN_WEAPON, gi.soundindex ("items/damage3.wav"), 1,
			ATTN_NORM, 0);

		// Calculate how much power to give to the push.
		VectorSubtract (tr.endpos, start, extent);
		strength = kPushPullRange - VectorLength (extent);

		// Now push them.
		VectorScale (forward, strength, forward);
		VectorAdd(forward, tr.ent->velocity, tr.ent->velocity);
	}
}

/*
=================
Cmd_Pull_f
=================
*/
void Cmd_Pull_f (edict_t *ent)
{
	vec3_t	start;
	vec3_t	forward;
	vec3_t	end;
	trace_t	tr;
	vec3_t	extent;
	vec_t		strength;

	// No pulling when you're dead.
	if (ent->deadflag)
		return;

	// Or when you're a ghost.
	if (teamplay->value
	&& ent->movetype == MOVETYPE_NOCLIP
	&& ent->solid == SOLID_NOT)
		return;

	// Or if you're frozen.
	if (ent->frozen)
		return;

	// Or if push/pull has been banned.
	if ((int)featureban->value & FB_PUSHPULL)
		return;

	VectorCopy(ent->s.origin, start);
	start[2] += ent->viewheight - 8;
	AngleVectors(ent->client->v_angle, forward, NULL, NULL);
	VectorMA(start, kPushPullRange, forward, end);
	tr = gi.trace(start, NULL, NULL, end, ent, MASK_SHOT);
	if (tr.ent
	&& ((tr.ent->svflags & SVF_MONSTER)
		/* || (tr.ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)) */
		|| (tr.ent->client)))
	{
		// Show a trail from the puller to the pullee.
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BUBBLETRAIL);
		gi.WritePosition (start);
		gi.WritePosition (tr.endpos);
		gi.multicast (ent->s.origin, MULTICAST_PHS);
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BFG_LASER);
		gi.WritePosition (start);
		gi.WritePosition (tr.endpos);
		gi.multicast (ent->s.origin, MULTICAST_PHS);

		// Have the puller emit a sound.
		gi.sound (ent, CHAN_WEAPON, gi.soundindex ("items/damage3.wav"), 1,
			ATTN_NORM, 0);

		// Calculate how much power to give to the push.
		VectorSubtract (tr.endpos, start, extent);
		strength = kPushPullRange - VectorLength (extent);

		// Now pull them.
		VectorScale (forward, -strength, forward);
		VectorAdd (forward, tr.ent->velocity, tr.ent->velocity);
	}
}


/*
=================
Cmd_Wave_f
=================
*/
void Cmd_Wave_f (edict_t *ent)
{
	int		i;

	i = atoi (gi.argv(1));

	// can't wave when ducked
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		return;

	// Or when you're a ghost.
	if (teamplay->value
	&& ent->movetype == MOVETYPE_NOCLIP
	&& ent->solid == SOLID_NOT)
		return;

	if (ent->client->anim_priority > ANIM_WAVE)
		return;

	ent->client->anim_priority = ANIM_WAVE;

	switch (i)
	{
	case 0:
		gi.cprintf (ent, PRINT_HIGH, "flipoff\n");
		ent->s.frame = FRAME_flip01-1;
		ent->client->anim_end = FRAME_flip12;
		break;
	case 1:
		gi.cprintf (ent, PRINT_HIGH, "salute\n");
		ent->s.frame = FRAME_salute01-1;
		ent->client->anim_end = FRAME_salute11;
		break;
	case 2:
		gi.cprintf (ent, PRINT_HIGH, "taunt\n");
		ent->s.frame = FRAME_taunt01-1;
		ent->client->anim_end = FRAME_taunt17;
		break;
	case 3:
		gi.cprintf (ent, PRINT_HIGH, "wave\n");
		ent->s.frame = FRAME_wave01-1;
		ent->client->anim_end = FRAME_wave11;
		break;
	case 4:
	default:
		gi.cprintf (ent, PRINT_HIGH, "point\n");
		ent->s.frame = FRAME_point01-1;
		ent->client->anim_end = FRAME_point12;
		break;
	}

}

/*
=================
Cmd_FireMode_f
MUCE: new function for adjusting firing mode
=================
*/
void Cmd_FireMode_f (edict_t *ent)
{
	// Don't allow if the burst machinegun has been banned.
	if ((int)weaponban->value & WB_BURSTMACHINEGUN)
		return;

	switch (ent->client->pers.fire_mode)
	{
		case 0:
			ent->client->pers.fire_mode = 1;
			gi.cprintf (ent, PRINT_HIGH,
				"Standard Machinegun Burst Fire Mode\n");
			break;

		case 1:
			default:
			ent->client->burstfire_count = 0;
			ent->client->pers.fire_mode = 0;
			gi.cprintf (ent, PRINT_HIGH,
				"Standard Machinegun Fully Automatic Mode\n");
			break;
	}
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f (edict_t *ent, qboolean team, qboolean arg0)
{
	int		i,j;
	edict_t	*other;
	char	*p;
	char	text[2048];
	gclient_t *cl;

	if (gi.argc () < 2 && !arg0)
		return;

	// If there's no teamplay, don't do team chat.
	if (!teamplay->value
	&& !((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		team = false;

	// If this player is dead or a ghost, don't allow team chat.
	if (teamplay->value
	&& (ent->deadflag
		|| (ent->solid == SOLID_NOT && ent->movetype == MOVETYPE_NOCLIP))
	&& !TeamplayCheckCountdown())
		team = false;

	if (team)
		Com_sprintf (text, sizeof(text), "(%s): ", ent->client->pers.netname);
	else
		Com_sprintf (text, sizeof(text), "%s: ", ent->client->pers.netname);

	if (arg0)
	{
		strcat (text, gi.argv(0));
		strcat (text, " ");
		strcat (text, gi.args());
	}
	else
	{
		p = gi.args();

		if (*p == '"')
		{
			p++;
			p[strlen(p)-1] = 0;
		}
		strcat(text, p);
	}

	// don't let text be too long for malicious reasons
	if (strlen(text) > 150)
		text[150] = 0;

	strcat(text, "\n");

	if (flood_msgs->value) {
		cl = ent->client;

        if (level.time < cl->flood_locktill) {
			gi.cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n",
				(int)(cl->flood_locktill - level.time));
            return;
        }
        i = cl->flood_whenhead - flood_msgs->value + 1;
        if (i < 0)
            i = (sizeof(cl->flood_when)/sizeof(cl->flood_when[0])) + i;
		if (cl->flood_when[i] && 
			level.time - cl->flood_when[i] < flood_persecond->value) {
			cl->flood_locktill = level.time + flood_waitdelay->value;
			gi.cprintf(ent, PRINT_CHAT, "Flood protection:  You can't talk for %d seconds.\n",
				(int)flood_waitdelay->value);
            return;
        }
		cl->flood_whenhead = (cl->flood_whenhead + 1) %
			(sizeof(cl->flood_when)/sizeof(cl->flood_when[0]));
		cl->flood_when[cl->flood_whenhead] = level.time;
	}

	if (dedicated->value)
		gi.cprintf(NULL, PRINT_CHAT, "%s", text);

	for (j = 1; j <= game.maxclients; j++)
	{
		other = &g_edicts[j];
		if (!other->inuse)
			continue;
		if (!other->client)
			continue;
		if (team)
		{
			if (!OnSameTeam (ent, other))
				continue;
		}
		gi.cprintf (other, PRINT_CHAT, "%s", text);
	}
}

/* 
================= 
Cmd_Lowlight_f 
JDB: new command for lowlight vision (GL mode ONLY) 4/4/98 
================= 
*/ 
void Cmd_Lowlight_f (edict_t *ent) 
{ 
	// No nightvision if it's been banned.
	if ((int)featureban->value & FB_NIGHTVISION)
		return;

	if(ent->client->lowlight ^= 1) 
	{ 
		gi.cvar_forceset("gl_saturatelighting","1"); 
		gi.cvar_forceset("r_fullbright","1"); 
		ent->client->ps.fov = 75; 
	} 
	else 
	{ 
		gi.cvar_forceset("gl_saturatelighting","0"); 
		gi.cvar_forceset("r_fullbright","0"); 
		ent->client->ps.fov = 90; 
	} 
} 

/* 
================= 
Cmd_Zoom_f 
================= 
*/ 
void Cmd_Zoom_f (edict_t *ent) 
{
	int zoomtype;

	// No zooming if it's been banned.
	if ((int)featureban->value & FB_ZOOM)
		return;

	zoomtype = atoi (gi.argv (1));
	if (zoomtype == 0)	
	{	
		ent->client->ps.fov = 90;		
	}	
	else if (zoomtype == 1)
	{	
		if (ent->client->ps.fov == 90)
			ent->client->ps.fov = 40;
		else if (ent->client->ps.fov == 40) 
			ent->client->ps.fov = 20;
		else if (ent->client->ps.fov == 20)
			ent->client->ps.fov = 10;
		else
			ent->client->ps.fov = 90;	
	}	
}

void Cmd_PlayerList_f(edict_t *ent)
{
	int i;
	char st[80];
	char text[1400];
	edict_t *e2;

	// connect time, ping, score, name
	*text = 0;
	for (i = 0, e2 = g_edicts + 1; i < maxclients->value; i++, e2++) {
		if (!e2->inuse)
			continue;

		Com_sprintf(st, sizeof(st), "%02d:%02d %4d %3d %s%s\n",
			(level.framenum - e2->client->resp.enterframe) / 600,
			((level.framenum - e2->client->resp.enterframe) % 600)/10,
			e2->client->ping,
			e2->client->resp.score,
			e2->client->pers.netname,
			e2->client->resp.spectator ? " (spectator)" : "");
		if (strlen(text) + strlen(st) > sizeof(text) - 50) {
			sprintf(text+strlen(text), "And more...\n");
			gi.cprintf(ent, PRINT_HIGH, "%s", text);
			return;
		}
		strcat(text, st);
	}
	gi.cprintf(ent, PRINT_HIGH, "%s", text);
}


/*
=================
ClientCommand
=================
*/
void ClientCommand (edict_t *ent)
{
	char *cmd;

	if (!ent->client)
		return;		// not fully in game yet

	// Issuing any command is considered not idle.
	ent->client->resp.idleTime = level.time;

	cmd = gi.argv(0);

	// Only a few commands are available during intermissions.
	if (level.intermissiontime)
	{
		if (Q_stricmp (cmd, "players") == 0)
			Cmd_Players_f (ent);
		else if (Q_stricmp (cmd, "say") == 0)
			Cmd_Say_f (ent, false, false);
		else if (Q_stricmp (cmd, "say_team") == 0
		|| Q_stricmp (cmd, "steam") == 0)
		{
			Cmd_Say_f (ent, true, false);
		}
		else if (Q_stricmp (cmd, "score") == 0)
			Cmd_Score_f (ent);
		else if (Q_stricmp (cmd, "help") == 0)
			Cmd_Help_f (ent);

		// Those are the only commands we allow now.
		return;
	}

	// Parse the command.  To speed things up cheaply, do a switch on the first
	// few characters in the command.
	switch (cmd[0])
	{
		case 'd':
			if (Q_stricmp (cmd, "drop") == 0)
				Cmd_Drop_f (ent);
			else if (Q_stricmp (cmd, "decoy") == 0)
				SP_Decoy (ent); 
			else
				goto notrecognized;
			break;

		case 'g':
			if (Q_stricmp (cmd, "give") == 0)
				Cmd_Give_f (ent);
			else if (Q_stricmp (cmd, "god") == 0)
				Cmd_God_f (ent);
			else
				goto notrecognized;
			break;

		case 'h':
			if (Q_stricmp (cmd, "help") == 0)
				Cmd_Help_f (ent);
			else if (Q_stricmp (cmd, "hook") == 0)
				Cmd_Hook_f (ent);
			else
				goto notrecognized;
			break;

		case 'i':
			if (Q_stricmp (cmd, "id") == 0)
				CTFID_f (ent);
			else if (Q_stricmp (cmd, "inven") == 0)
				Cmd_Inven_f (ent);
			else if (Q_stricmp (cmd, "invnext") == 0)
				SelectNextItem (ent, -1);
			else if (Q_stricmp (cmd, "invprev") == 0)
				SelectPrevItem (ent, -1);
			else if (Q_stricmp (cmd, "invnextw") == 0)
				SelectNextItem (ent, IT_WEAPON);
			else if (Q_stricmp (cmd, "invprevw") == 0)
				SelectPrevItem (ent, IT_WEAPON);
			else if (Q_stricmp (cmd, "invnextp") == 0)
				SelectNextItem (ent, IT_POWERUP);
			else if (Q_stricmp (cmd, "invprevp") == 0)
				SelectPrevItem (ent, IT_POWERUP);
			else if (Q_stricmp (cmd, "invuse") == 0)
				Cmd_InvUse_f (ent);
			else if (Q_stricmp (cmd, "invdrop") == 0)
				Cmd_InvDrop_f (ent);
			else
				goto notrecognized;
			break;

		case 'k':
			if (Q_stricmp (cmd, "kill") == 0)
				Cmd_Kill_f (ent);
			else if (Q_stricmp (cmd, "kamikaze") == 0) 
				Cmd_Kamikaze_f (ent);
			else
				goto notrecognized;
			break;

		case 'l':
			if (Q_stricmp (cmd, "lsight") == 0) 
				SP_LaserSight (ent);
			else if (Q_stricmp (cmd, "laser") == 0) 
				PlaceLaserTripwire (ent);
			else
				goto notrecognized;
			break;

		case 'n':
			if (Q_stricmp (cmd, "notarget") == 0)
				Cmd_Notarget_f (ent);
			else if (Q_stricmp (cmd, "noclip") == 0)
				Cmd_Noclip_f (ent);
			else if (Q_stricmp (cmd, "nightvision") == 0)
				Cmd_Lowlight_f (ent);
			else
				goto notrecognized;
			break;

		case 'p':
			if (Q_stricmp (cmd, "push") == 0)
				Cmd_Push_f (ent);
			else if (Q_stricmp (cmd, "pull") == 0)
				Cmd_Pull_f (ent);
			else if (Q_stricmp (cmd, "putaway") == 0)
				Cmd_PutAway_f (ent);
			else if (Q_stricmp (cmd, "players") == 0)
				Cmd_Players_f (ent);
			else if (Q_stricmp(cmd, "playerlist") == 0)
				Cmd_PlayerList_f (ent);
			else
				goto notrecognized;
			break;

		case 's':
			if (Q_stricmp (cmd, "say") == 0)
				Cmd_Say_f (ent, false, false);
			else if (Q_stricmp (cmd, "say_team") == 0
			|| Q_stricmp (cmd, "steam") == 0)
				Cmd_Say_f (ent, true, false);
			else if (Q_stricmp (cmd, "scanner") == 0)
				Toggle_Scanner (ent);
			else if (Q_stricmp (cmd, "score") == 0)
				Cmd_Score_f (ent);
			else
				goto notrecognized;
			break;

		case 't':
			if (Q_stricmp (cmd, "thirdx") == 0)
				Cmd_ThirdX_f (ent);
			else if (Q_stricmp (cmd, "thirdz") == 0)
				Cmd_ThirdZ_f (ent);
			else if (Q_stricmp (cmd, "team") == 0)
				CTFTeam_f (ent);
			else
				goto notrecognized;
			break;

		case 'w':
			if (Q_stricmp (cmd, "weapprev") == 0)
				Cmd_WeapPrev_f (ent);
			else if (Q_stricmp (cmd, "weapnext") == 0)
				Cmd_WeapNext_f (ent);
			else if (Q_stricmp (cmd, "weaplast") == 0)
				Cmd_WeapLast_f (ent);
			else if (Q_stricmp (cmd, "wave") == 0)
				Cmd_Wave_f (ent);
			else
				goto notrecognized;
			break;

		// Some commands are the only one to start with that letter.
		default:
			if (Q_stricmp (cmd, "use") == 0)
				Cmd_Use_f (ent);
			else if (Q_stricmp (cmd, "chasecam") == 0)
				Cmd_Third_f (ent);
			else if (Q_stricmp (cmd, "firemode") == 0)
				Cmd_FireMode_f (ent);
			else if (Q_stricmp (cmd, "zoom") == 0)
				Cmd_Zoom_f (ent);

		// Anything not recognized will print an error message.
			else
			{
		notrecognized:
				gi.cprintf (ent, PRINT_HIGH, "Unrecognized command: %s %s\n",
					gi.argv (0), gi.args());
			}
			break;
	}
}
