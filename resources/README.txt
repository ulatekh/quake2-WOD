=================================================
Weapons of Destruction for Quake2 - *Version 6.03*
=================================================

Author: Steven Boswell a.k.a. Hash Assassin
Original author: Matt Wright a.k.a. PunK

Email: weapons@telefragged.com

Date: 12/7/98

Page: http://www.telefragged.com/weapons/ - The official Weapons of
Destruction page.  Includes the old source of 3.6 and 5.4a, and all
recent news about the mod.  Also check out this page and ensure you have
the latest version.

========================
Installation - Important
========================

Make a directory in your Quake2 Directory, 

Say c:\quake2\weapons

("weapons" is what all the servers should be using.  It's best to keep
things consistent, so that QuakeII knows how to match up servers with
what you have installed on your machine.)

Unzip my mod using winzip -or- pkunzip -d wod603.zip
Unzip the files straight into the weapons directory.  (If you unzip
to another directory and then move the files over, be sure you move
any hidden files too.)

If you want to be able to play Extinction, the teamplay variant of WoD,
you'll need to have Zoid CTF installed.  If you don't have it installed,
http://www.planetquake.com/files/ contains several places where you can
download it.  Then, after it's installed to a "ctf" directory under your
Quake2 directory, copy ctf/pak0.pak to weapons/pak1.pak -- then
you're ready for Extinction.  (I apologize for this step...Id Software
should have realized CTF wouldn't be the only teamplay mod people would
want to play.)

Now, to start, launch Quake and type "set game weapons" in the console.
You can now start a single-player game, or a deathmatch server, or an
Extinction server, or whatever you want to do.

If you have trouble installing WoD, feel free to e-mail me.

You must have Quake2 version 3.19 to run this patch.

To locate WoD and WoD:Extinction servers on the Net, the distribution
comes with tabs for GameSpy and PingTool that will automatically
locate such servers for you.  Follow the directions that come with
GameSpy and PingTool to install them.

==========================================
Description of Weapons of Destruction 6.03
==========================================

There are three ways to enjoy Weapons of Destruction:

o Single-player
  The Strogg won't know what hit them.  To make things a little more
  fair, you might want to select Nightmare skill level -- type "skill 3"
  at your Q2 console.
o Deathmatch
  The classic way.  Get a bunch of your friends together and blow each
  other to smithereens.  Or join a public server and wreak chaos on
  total strangers.  Either way is fun.
o Extinction
  Teamplay WoD!  Two teams fight over control of the spawnpoints.  Once a
  team has no spawnpoints, they can't spawn into the game, and are in
  danger of going extinct.  Who needs a wimpy "capture the flag" game when
  you can fight to the death?

Weapons of Destruction adds several new weapons and features to the
game.  All the weapons are available as "alternates" to the normal
weapons.  Once you grab a normal weapon and select it, select it again
to get the alternate weapon.  (Select it again to return to the normal
weapon.  You get the idea.)  There are seven different types of grenades;
keep selecting grenades to run through the list.  All these weapons can
also be selected from the inventory, or with "use" commands.

-------
Weapons
-------

Super Blaster: The alternate blaster.  Fires a red bolt with a sparkling
red trail and does quite a lot of damage.  Uses 5 cells per shot.

Sniper Gun: The alternate shotgun.  Fires a very powerful bolt that
usually gibs your opponent, but it's very slow to reload and takes
5 shells per shot.

Freeze Gun: The alternate supershotgun.  Freezes your opponent in place
so that you can dispatch them with conventional weaponry.  Or just
freeze them to death, if you like.  The more you freeze them, the longer
they stay frozen.  Uses 10 cells per shot.

Machine Rocket Gun: The alternate machinegun, and everyone's favorite it
seems.  Instead of bullets, this fires a steady stream of rockets.
They're not as powerful as normal rockets, but then you can't bombard
a whole area with normal rockets.  Uses 2 bullets per shot.

Burst-fire machinegun: If you have the regular machinegun selected,
"cmd firemode" will switch between fully-automatic mode (the usual) and
a new burst-fire mode.  It'll shoot 6 bullets in a quick burst.
Surprisingly effective!

Streetsweeper: The alternate chaingun.  Fires a steady stream of shotgun
shells.  Tends to clear the way -- just charge through a group of
enemies and watch them disappear.

darKMajick grenades: The alternate grenades.

o Cluster grenade: Explodes into several other grenades.
o Rail bomb: Explodes into several railgun shots.
o Plasma grenade: Explodes into a BFG shot.
o Napalm grenade: Explodes into a bunch of fire and burning explosions.
o Shrapnel grenade: Explodes into a bunch of flaming shrapnel.
o Cataclysm device: Has to be seen to be believed.  Explodes like an
  atomic bomb, complete with a shockwave that tends to kill everything.

The grenade launcher will lob whatever grenade type you selected last.

Bazooka: The alternate grenade launcher.  Instead of lobbing grenades,
it fires them in a straight line.  Like the grenade launcher, it fires
whatever type of grenade you selected last.

Homing Rocket Launcher: The alternate rocketlauncher.  Aim in someone's
general direction and fire; it'll do the rest.

Plasma Rifle: The alternate hyperblaster.  Fires a beam that you can
wave around; it tends to fry anything it touches.

Flame Thrower: The alternate railgun.  Here are the specifics (partly
from the Napalm mod text file by Patrick Martin):

Monsters and players that catch on fire will burn for at least 5 seconds.
The following will extinguish the fire:
  * The player immerses in liquid at waist deep.
    Crouching can help if the liquid is too shallow.
  * The player picks up adrenaline or megahealth.
  * The player activates an invulnerability powerup.
  * Health box has a percent chance equal to its power of
    extinguishing the fire.  If the fire is not extinguished,
    then it will speed up the time when the fire burns out.
  * A bio-suit greatly reduces the chances of catching on fire
    and offers partial immunity to indirect fire damage.
  * Active power armor reduces the chance of igniting.
  * If a fire "ignites" a target already on fire, then the
    time before the fire burns out is reset.
  * All frags go to the entity who ignited the target most recently.
  * Some monsters, such as tanks or bosses (except the makron), cannot burn.

The flame thrower uses 1 slug per fire and fires quite rapidly.
The standard railgun now uses 4 slugs per shot.  All slug quantities
and capacities have been boosted by 4 times, to account for this.

There are a few more weapon-like features not done with "alternate"
weapons.

Laser Tripwires: Much like the Duke Nukem 3D laser-tripped grenades.
Set up a darKMajick grenade to fire when they walk through a laserbeam.
The wall-mounted (floor-mounted, ceiling-mounted, door-mounted,
whatever) grenade can also be shot at until it blows up.  Takes 5 cells,
plus the grenades.

Tractor Beam: When all else fails, use The Force.  Pull player or
monster toward you or push them away.

Kamikaze: When all else *really* fails.  This isn't entirely a weapon,
since it kills you, so it is intended for deathmatch only.  It sends a
big explosion from where you die...kinda like strapping a huge timebomb
on your chest and setting it off.

Finally, some of the standard weapons have been modified:

The blaster fires red or blue lasers.

The super shotgun now has a big kickback.  Shoot someone and they go
flying; if the shot doesn't kill them, maybe they'll die when they slam
into the wall.

The rockets from the standard rocket launcher, and hand-thrown grenades,
will spray fire when they explode.

When you grab a machinegun, you get the machine-rocket gun first.
Similarly, when you grab a railgun, the flamethrower comes up first.

--------
Features
--------

Swinging Grappling hook: This is not a glorified flying machine, like
the grapples in other mods.  Not only do you swing around as you use
it, but you can seriously hurt yourself if you use it wrong.  You can
can also make the line grow or shrink at will.

Decoy: Sort of like the Hologram in Duke3D, except it shoots at players.

Scanner: Track players down in DM.

Jetpack: This replaces the quad.  (Who needs quad damage with all these
heavy weapons?  Right.)  Will let you fly, but it's got all the problems
of a real jetpack: you have to work at it to avoid slamming into walls
and so on.

Lasersight: A little red ball will dance in front of you, showing you
where your gun is pointed.  It'll glow brighter once it finds a target.
But be careful, everyone else can see it too.

Sniper Zooming: Press F11 to cycle through views and f10 to switch back
to your normal view as a sort of a "panic button".

Night Vision: this only works right in GL (3dfx) mode.

========
Commands
========

--------------------------------------------------------
Function		| binding	| Command
--------------------------------------------------------
Night Vision		| N		| cmd night vision
--------------------------------------------------------
Jetpack			| J		| use jetpack
--------------------------------------------------------
Laser Sight		| S		| cmd lsight
--------------------------------------------------------
Laser Tripwire		| L		| cmd laser
--------------------------------------------------------
TractorBeam Pull	| o		| cmd pull
--------------------------------------------------------
TractorBeam Push	| p		| cmd push
--------------------------------------------------------
Cycles Zoom Views	| f11		| cmd zoom 1
--------------------------------------------------------
Original View		| f10		| cmd zoom 0
--------------------------------------------------------
Kamikaze		| end		| cmd kamikaze
--------------------------------------------------------
darKMajick grenades     | N\A           | see below
--------------------------------------------------------
Grappling Hook:		| G		| +hook
Slide up grapple        | R		| +shrink
Slide down grapple      | F		| +grow
--------------------------------------------------------
Scanner			| S		| cmd scanner
--------------------------------------------------------
Decoy			| D		| cmd decoy
--------------------------------------------------------
Toggle Burst fire for   | M		| cmd firemode
Standard machine gun    | M		| cmd firemode
--------------------------------------------------------
			
* Tip * Use console command "give all" to get all the items.

========
Mod info
========

New DLL(s): Yep

New Sound(s): Yep

New Model(s): Yep

Known Bugs: I've fixed all the bugs I've received. If you find any more,
mail me at weapons@telefragged.com

Source: You can download the full source of Weapons of Destruction 5.4a
and 3.6 at the Weapons of Destruction page.

=======
Credits
=======

-Matt Wright [punkz93@geocities.com] for the original mod.

Matt thanks:

-Mark Wheeler [markwheeler@erols.com] for Freeze code help
-Ryan Nunn [triforce@merlin.net.au] for the great new chasecam
-Perry Manole [Perrym@diamondmm.com] for the swinging grappling hook
-Hentai [www.telefragged.com/tsunami/] for some of the code from VWep
-Patrick Martin ( http://www.planetquake.com/TheCoven - cmarti39@icon.net)
for the the source to the flame weapons, which have been modified.
-The darKMajick team for the darKMajick grenades
-Thom W. for running a server helping me test my mod in DM.
-Mike Fox for bug fixing help, and compiling with MSVC++ (I'm too poor for MSVC)
-Telefragged.com for hosting my site
-QdeveLS at http://www.planetquake.com/qdevels/ for being a great 
page for QuakeII DLL resources. If you don't know where to start in making
Quake II mods, visit this place.
-PeZKiNG
-Everyone who sent in comments/suggestions

Steve thanks:

-Hocus and the (LOF)Clan server, for hosting my initial modifications to
 WoD (and for putting up with the resulting bugs :).
 http://members.home.com/hocus/
-L-Fire, for giving me the time to do this.
 http://www.planetquake.com/lfire/
-George for the PingTool tabs.
 http://pingtool.com/
-All the great people that run WoD servers.  YOU RULE!

Don't see your name here and you helped? Mail me.

Editors used by Matt: LCC, Wordpad, Notepad, and edit.
Editors used by Steve: Microsoft Developer Studio and vi.

===========
Mod History
===========

6.02
-Grenade launcher now fires all darKMajick grenades
-New alternate grenade-launcher weapon, the bazooka.  Fires all
 darKMajick grenades, but in a straight line.
-Lasers replaced with laser tripwires that fire all the
 darKMajick grenades.
-Homing rockets can now be shot down, and only take 1 rocket per shot
 (was 2 per shot).
-Doubled the amount of ammo backpacks & bandoliers give.
-New plasma rifle.
-Fire now makes a sizzling sound.
-Added ZbotCheck 1.01 by the Lithium crew.  (Windows version only: the
 Linux version won't link & they seem to be uninterested in providing
 support.)
6.01
-Added the allow_cataclysm server variable
-Sniper gun now takes 5 shells, not 2.  Weapon balancing thing.
-Cataclysm device kills no longer credited to inflictor.
-Homing rocket radius explosion kills now credited to inflictor.
-Added "You've been frozen" message when player gets frozen.  It's
 a lame hack until I rewrite the frozen code to make being frozen
 more intuitive.
-Fixed even more bugs -- spawnpoints no longer get thrown in
 explosions, decoys work again, skin no longer gets hosed when
 you're frozen, the frozen obituary message is accurate now,
 the lasersight gets cleaned up when the player dies, and
 NoAmmoWeaponChange() is a little more intelligent.
-Officially released a Linux version, finally.
6.0
 Done by Hash Assassin, whatis@yyz.com, on or around 9/28/1998.  My
 first official version.
-Fixed an ungodly number of bugs, compiler warnings, and inefficiencies, some
 in WoD code, some in the original Id code.
-Added streetsweeper
-Rewrote plasma rifle to be more like Doom II version
-Rewrote NoAmmoWeaponChange() to select some of the new weapons.
-Increased ammo capacities.  Now more fragfest than ever!
-"maplist" feature.  Just set the Q2 variable "maplist" to something non-zero,
 and the next map will be chosen out of the maplist.txt file in your WOD dir.
 No more running maps in a boring old loop.
-Deathmatch players now invulnerable for 3 seconds when they spawn.  Gives them
 a fighting chance & protects against people putting laser tripwires on spawn
 spots.
-Balanced the weapons better.  Now ammo usage is more in line with power.
-Added/modified client obituary messages.
-Laser sight brightens when on a player or monster.  Hee hee.
-Removed triple hyperblaster.  It's a nice weapon, just very laggy.  And the
 machine rockets are already pushing the limits.

5.4a-

-Fixed Crashing when dying with freeze gun.

5.4-

-Added Freeze Gun
-Night Vision
-Re-added standard railgun
-Added new Grenade and Super Blaster effects
-No bugs needed to be fixed! Woohoo!

5.3:

-Perfect Chasecam, I'm pretty sure it's bugless.
-Re-added standard Machine-Gun
-Added Burst Fire mode for Standard machine-gun
-Fixed Cluster Bomb bug
-Fixed kamikaze when dead bug
-Removed Viewable models, do to so many bugs [people wanted to use their
custom models, but got a white diamond on its ass, etc.]


5.2:

-replaced detpipes with shrapnel grenade
-added sniper gun
-added decoy
-added plasma gun
-better rocket explosions
-replaced grappling hook with swinging grappling hook
-fixed tons of bugs
-removed ricocheting lasers and range finder- it lags net play horribly

5.0:

-Added chasecam
-Added Visible Weapons
-Added sound for ricocheting lasers
-Added scanner
-MOTD support
-Fixed yet more bugs - Thanks Mike Fox

4.9b:

-Fixed messages
-Fixed crashing when using super blaster
-Lowered number grenades picked up

4.9:

-Fixed numerous deathmatch bugs.

4.8:

-Re coded the WHOLE mod with the 3.14 source as a base. This should
fix all the deathmatch and coop bugs.

-Re added shotgun

-Enhanced flamethrower burn. It now uses a model for the burn animation.

4.7:

-Now works with 3.12/3.13/3.14

-added Grappling Hook

-fixed 8 twice

-put fire model for fire on the ground

4.6 - minor update:

-Updated flame thrower. It has a great new model.

4.5:

-Removed super blaster model, so now there's no null skin error, and it
can run on servers without clients having the patch.

-Replaced Burst Railgun with Flamethrower

-Added Flame Grenade Launcher

-Replaced chain shotgun with Fire Rocket Launcher

-Changed Chain Shotgun to Flame Rocket Launcher

4.1 - small update:

-(Re)Added random blue/red lasers to blaster

-(Re)Added long kickback for Super Shotgun.

4.0:

-Removed Homing Rockets due to code conflicts

-Added modified model for Superblaster

-Removed Grenade Launcher proximity (see below)

-Added "darKMajick" grenades- Cluster, Floating Proximity 
(improved from previous grenade launcher proximity), 
Railbomb, Plasma Bomb, Napalm Grenade, and Cataclysm device

-Added kamikaze device

-Added ricocheting lasers sound

-Added Machine Gun Rockets

-Fixed entities covering up blaster laser effects

Other: This was a challenging task. I had to rewrite the whole code from
the base up to add in the darKMajick grenades.

Version 3.6:

-Made the jetpack much better.

-Fixed crashing

Version 3:

-Fixed the laser sight skin error problem

-Added Rangefinder

-Added "Star Wars" Lasers

-Added Zoom Commands

-Added Laser Tripwire

-Added Burst Fire shotgun

Version 2 :

-Changed the super blaster trail effects so it looks like a blaster and not 
a grenade.

-Changed the Cluster Bombs to a Triple Grenade launcher

-Balanced Railgun more.

-Detonatable Throwing Grenades!

Version 1: Initial release.

======
misc.
======

Read that freakin' id license agreement.

If you need further assistance with installing/playing this mod,
contact me.
