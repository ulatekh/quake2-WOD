               *************************************
               Instructions for running a WoD server
               *************************************

I suggest naming your Weapons of Destruction directory "weapons".  That
seems to be the standard over the Internet.  And it makes it much
easier, when it comes to automatic downloads and all that, to know all
Weapons of Destruction files are going into the same directory.

Two default config files have been provided -- server.cfg for deathmatch,
server_e.cfg for Extinction.  You will probably want to edit them before
running your server for the first time (though their defaults work
well).

To run a dedicated server, the command is:

quake2 +set dedicated 1 +set game weapons +exec server.cfg

or, for Extinction,

quake2 +set dedicated 1 +set game weapons +exec server_e.cfg

-----

Try "set maplist 1" on your deathmatch server...the next map will be
chosen from maplist.txt, instead of running in the normal loop.
(maplist will be subsequently set to 2, 3, etc. by the server,
indicating where it is in the maplist.  It will loop back to 1 at the
end of the file.)

I recommend starting with either "biggun" or "fact3" -- in my
experience, those are the two biggest fragfest maps in the standard
collection, and they're nice to use whenever you want to wake everyone
up and get their blood pumping.

The maplist.txt file, as provided, contains my opinion of which game
maps make good DM maps.  You are free to have your own opinion.
maplist.txt contains the name of each of the good maps, 10 times,
shuffled together thoroughly.  Should be fine for most uses!

Don't use maplist with Extinction.  Extinction needs modified maps in
order to work, and they're set up to run in a loop.

-----

You can ban any of the weapons and features with the "weaponban" and
"featureban" variables.  Both variables are flags, in the same way
that the "dmflags" variable is: just add together the numbers of the
weapons/features you want to ban, and that's your value for
weaponban/featureban, respectively.

Values for weaponban:

Ban super blaster		1
Ban shotgun			2
Ban sniper gun			4
Ban super shotgun		8
Ban freeze gun			16
Ban standard machinegun		32
Ban machine rocket gun		64
Ban burstfire machinegun	128
Ban chaingun			256
Ban streetsweeper		512
Ban grenades			1024
Ban cluster grenade		2048
Ban rail bomb			4096
Ban plasma grenade		8192
Ban napalm grenade		16384
Ban shrapnel grenade		32768
Ban cataclysm device		65536
Ban grenade launcher		131072
Ban bazooka			262144
Ban rocket launcher		524288
Ban homing rocket launcher	1048576
Ban hyperblaster		2097152
Ban plasma rifle		4194304
Ban railgun			8388608
Ban flamethrower		16777216
Ban BFG10K			33554432

For example, to ban the cataclysm device, the homing rocketlauncher,
and the BFG10K (the 3 most hated weapons it seems), weaponban is
65536 + 1048576 + 33554432 == 34668544.

Note that if you ban grenades (value 1024), then *all* of the grenade
types, as well as the grenade launcher and bazooka, are banned too.

Values for featureban:

Ban bandolier		1
Ban ammo pack		2
Ban silencer		4
Ban rebreather		8
Ban environment suit	16
Ban adrenaline		32
Ban megahealth		64
Ban power armor		128
Ban invulnerability	256
Ban decoy		512
Ban grappling hook	1024
Ban player ID		2048
Ban kamikaze		4096
Ban lasersight		8192
Ban triplasers		16384
Ban nightvision		32768
Ban push/pull		65536
Ban scanner		131072
Ban zoom view		262144
Ban jetpack		524288

For example, to ban the decoy, nightvision, and scanner, featureban is
512 + 32768 + 131072 == 164352.

You can also ban frags for certain weapons, i.e. players can use them
to kill other players, but won't score any points for doing so.  The
"fragban" variable uses the same flags and values as the "weaponban"
variable, with the following additions:

Ban frags for blaster				67108864
Ban frags for telefrags				134217728
Ban frags for kamikazes				268435456
Ban frags for grappling hook			536870912

For example, to ban frags for cataclysm devices, homing rockets, and
telefrags, fragban is 65536 + 1048576 + 134217728 == 135331840.

Also note that banning frags for the flamethrower (value 16777216) will
ban frags for all fire.

-----

WoD now features IP banning.  You can add or remove addresses from the
filter list with "sv addip <ip>" and "sv removeip <ip>".  The ip address
is specified in dot format, and any unspecified digits will match any
value, so you can specify an entire class C network with "addip 192.246.40".

removeip will only remove an address specified exactly the same way.
You cannot addip a subnet, then removeip a single host.

There is one more way to ban a player.  "sv kickban <player#>" will ban
the player's IP address and then kick them from the server.

"sv listip" prints the current list of filters.  "sv writeip" dumps
"addip <ip>" commands to listip.cfg so it can be execed at a later date.
The filter lists are not saved and restored by default.

In addition, there's a "filterban" variable.  If set to 1 (the default),
then ip addresses matching the current list will be prohibited from
entering the game.  If set to 0, then only addresses matching the list
will be allowed.  This lets you easily set up a private game, or a game
that only allows players from your local network.

-----

Does your server sometimes crash with "ERROR: *Index: overflow"?

I've finally figured out what the problem is.  Turns out the game is
running out of modelindexes.  Every model in the game (male player,
female player, each weapon, health-pack, and so on) gets loaded in and
is assigned a modelindex.  Quake II has an internal limit of 256 models.

Most people don't get this problem unless they have several extra player
models installed on their server machine, *and* they use the maplist
feature.

The problem happens because anything in the map that can move or be
destroyed (exploding walls, bridges, and so on) are also considered a
model.  They might not be used in the deathmatch game, but the
modelindex doesn't get freed.  That's just some sloppiness on Id's part.
I've written them a letter about it.

Until they fix their bug, here's what you do.  First, if you have extra
installed player models, you don't necessarily have to remove them.
First, see if you can get just the standard 8 DM maps to run without
incident.  If not, you have too many player models & you're gonna have
to remove some.  But if the DM maps work fine, then you can run your
server with maplist turned on.  Note what maps cause your server to
crash, and remove them from the maplist.txt file.  Eventually :) you
will have a list of maps that won't crash your server.

-----

As always, if you have questions, feel free to mail me at
weapons@telefragged.com
