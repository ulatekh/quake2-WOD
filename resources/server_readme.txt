               *************************************
               Instructions for running a WoD server
               *************************************

This documentation assumes you're already familiar with running Q2 and
know your way around it pretty well.  If you don't, I suggest going to
http://www.planetquake.com/console/ to learn about all the different
things you can do at the Q2 console.

-----

I suggest naming your Weapons of Destruction directory "weapons".  That
seems to be the standard over the Internet.  And it makes it much
easier, when it comes to automatic downloads and all that, to know all
Weapons of Destruction files are going into the same directory.

Three default config files have been provided -- server.cfg for
deathmatch, server_t.cfg for team deathmatch, server_e.cfg for
Extinction.  You will probably want to edit them before running your
server for the first time (though their defaults work well).

To run a dedicated server, the command is:

quake2 +set dedicated 1 +set game weapons +exec server.cfg

or, for team deathmatch,

quake2 +set dedicated 1 +set game weapons +exec server_t.cfg

or, for Extinction,

quake2 +set dedicated 1 +set game weapons +exec server_e.cfg

-----

Try "set maplist 1" on your deathmatch or team-deathmatch server...the
next map will be chosen from maplist.txt, instead of running in the
normal loop.  (maplist will be subsequently set to 2, 3, etc. by the
server, indicating where it is in the maplist.  It will loop back to 1
at the end of the file.)

I recommend starting with either "biggun" or "fact3" -- in my
experience, those are the two biggest fragfest maps in the standard
collection, and they're nice to use whenever you want to wake everyone
up and get their blood pumping.

The maplist.txt file, as provided, contains my opinion of which game
maps make good DM maps.  You are free to have your own opinion.
maplist.txt contains the name of each of the good maps, 10 times,
shuffled together thoroughly.  Should be fine for most uses!

Don't use maplist with Extinction.  Extinction needs modified maps in
order to work, and they're set up to run in a loop.  (The only maps that
presently work with Extinction are jail1, jail5, mine1, strike, ware1,
and ware2.)

-----

Your server can have a "message of the day".  It'll be shown briefly
to players as they log in (and will appear in their scrollback).  The
standard name for the MOTD file is "motd.txt", but that can be changed
by changing the value of the "motdfile" variable.  (The MOTD file will
always be looked for in your "weapons" directory.)

NOTE: due to limitations in the Q2 network protocol, your MOTD can't be
very long.  There is a per-packet size limit of 1500 bytes, and the
MOTD is not the only thing being transmitted in the first packet.  If
your MOTD is too long, your players will overflow immediately upon
connection, and won't be able to connect.  This is even more true for
Teamplay DM or Extinction, which display a menu to new players -- the
menu takes up even more space in the first packet.  I'll fix this
eventually, but the solution is sorta complicated and involved, and kinda
more work than it's really worth, so until I waste the time to do it, just
be aware the limitation exists.

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
Ban guided rocket launcher	1048576
Ban hyperblaster		2097152
Ban plasma rifle		4194304
Ban railgun			8388608
Ban flamethrower		16777216
Ban BFG10K			33554432

For example, to ban the cataclysm device and the BFG10K (the 2 most
hated weapons it seems), weaponban is 65536 + 33554432 == 33619968.

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

For example, to ban frags for cataclysm devices and telefrags, fragban
is 65536 + 134217728 == 134283264.

Also note that banning frags for the flamethrower (value 16777216) will
ban frags for all fire.

-----

WoD now features IP banning.  You can add or remove addresses from the
filter list with "sv addip <ip>" and "sv removeip <ip>".  The ip address
is specified in dot format, and any unspecified digits will match any
value, so you can specify an entire class C network with "sv addip 192.246.40".

removeip will only remove an address specified exactly the same way.
You cannot addip a subnet, then removeip a single host.

There is one more way to ban a player.  "sv kickban <player#>" will ban
the player's IP address and then kick them from the server.

"sv listip" prints the current list of filters.  "sv writeip" dumps
"sv addip <ip>" commands to listip.cfg so it can be execed at a later date.
The filter lists are not saved and restored by default.

In addition, there's a "filterban" variable.  If set to 1 (the default),
then ip addresses matching the current list will be prohibited from
entering the game.  If set to 0, then only addresses matching the list
will be allowed.  This lets you easily set up a private game, or a game
that only allows players from your local network.

-----

Other variables:

idledetect - The number of minutes a player can be idle before they're
	kicked.  (Set it to 0 to turn it off.)
maplistfile - The name of the maplist file.  Default is "maplist.txt"
motdfile - The name of the message-of-the-day file.  Default is "motd.txt"

Changed dmflags:

DF_TEAMREBALANCE - Available under Teamplay DM or Extinction.  Replaces
	DF_CTF_NO_TECH (since there aren't any techs in WoD).  If set, then
	after each game, the teams are rearranged according to players'
	frags-per-minute.  (This is probably what you'd want to run on a
	public Teamplay WoD server.)

-----

Does your server sometimes crash with "ERROR: *Index: overflow"?

Usually, it's the server running out of modelindexes.

The problem happens because anything in the map that can move or be
destroyed (exploding walls, bridges, and so on) are also considered a
model.  They might not be used in the deathmatch game, but the
modelindex doesn't get freed.

It just means you can't use that map with WoD.  Stop using it.

-----

As always, if you have questions, feel free to mail me at
weapons@telefragged.com
