CFLAGS = -O -fPIC -Dstricmp=Q_stricmp -D__cdecl=
LDFLAGS = -S

ORIGDIR=Source

OBJS = dwm.o g_ai.o g_chase.o g_cmds.o g_combat.o g_func.o g_items.o \
	g_light.o g_main.o g_misc.o g_monster.o g_phys.o g_team.o g_save.o \
	g_spawn.o g_svcmds.o g_target.o g_trigger.o g_turret.o g_utils.o \
	g_weapon.o jetpack.o kamikaze.o lasertrip.o lsight.o m_actor.o \
	m_berserk.o m_boss2.o m_boss3.o m_boss31.o m_boss32.o m_brain.o \
	m_chick.o m_flash.o m_flipper.o m_float.o m_flyer.o m_gladiator.o \
	m_gunner.o m_hover.o m_infantry.o m_insane.o m_medic.o m_move.o \
	m_mutant.o m_parasite.o m_soldier.o m_supertank.o m_tank.o \
	maplist.o p_botcheck.o p_client.o p_hook.o p_hud.o p_menu.o \
	p_trail.o p_view.o p_weapon.o q_shared.o s_cam.o scanner.o \
	wf_decoy.o x_fbomb.o x_fire.o

game.so: $(OBJS)
	ld -o $@ $(OBJS) -shared $(LDFLAGS); chmod 0755 $@

clean:
	/bin/rm -f $(OBJS) game.so

$*.o: $*.c
	$(CC) $(CFLAGS) -c $*.c

$*.c: $(ORIGDIR)/$*.c
	tr -d '\015' < $(ORIGDIR)/$*.c > $*.c

$*.h: $(ORIGDIR)/$*.h
	tr -d '\015' < $(ORIGDIR)/$*.h > $*.h

# DO NOT DELETE

dwm.o: g_local.h q_shared.h game.h
g_ai.o: g_local.h q_shared.h game.h
g_cmds.o: g_local.h q_shared.h game.h m_player.h
g_combat.o: g_local.h q_shared.h game.h
g_func.o: g_local.h q_shared.h game.h
g_items.o: g_local.h q_shared.h game.h
g_main.o: g_local.h q_shared.h game.h
g_misc.o: g_local.h q_shared.h game.h
g_monster.o: g_local.h q_shared.h game.h
g_phys.o: g_local.h q_shared.h game.h
g_save.o: g_local.h q_shared.h game.h tables/gamefunc_decs.h \
	tables/gamefunc_list.h tables/gamemmove_decs.h \
	tables/gamemmove_list.h tables/fields.h tables/levelfields.h \
	tables/clientfields.h
g_spawn.o: g_local.h q_shared.h game.h
g_svcmds.o: g_local.h q_shared.h game.h
g_target.o: g_local.h q_shared.h game.h
g_trigger.o: g_local.h q_shared.h game.h
g_turret.o: g_local.h q_shared.h game.h
g_utils.o: g_local.h q_shared.h game.h
g_weapon.o: g_local.h q_shared.h game.h
jetpack.o: g_local.h q_shared.h game.h
kamikaze.o: g_local.h q_shared.h game.h
laser.o: g_local.h q_shared.h game.h
lsight.o: g_local.h q_shared.h game.h
m_actor.o: g_local.h q_shared.h game.h m_actor.h
m_berserk.o: g_local.h q_shared.h game.h m_berserk.h
m_boss2.o: g_local.h q_shared.h
m_boss2.o: game.h m_boss2.h
m_boss3.o: g_local.h q_shared.h
m_boss3.o: game.h m_boss32.h
m_boss31.o: g_local.h q_shared.h
m_boss31.o: game.h m_boss31.h
m_boss32.o: g_local.h q_shared.h
m_boss32.o: game.h m_boss32.h
m_brain.o: g_local.h q_shared.h
m_brain.o: game.h m_brain.h
m_chick.o: g_local.h q_shared.h
m_chick.o: game.h m_chick.h
m_flash.o: q_shared.h
m_flipper.o: g_local.h q_shared.h
m_flipper.o: game.h
m_flipper.o: m_flipper.h
m_float.o: g_local.h q_shared.h
m_float.o: game.h m_float.h
m_flyer.o: g_local.h q_shared.h
m_flyer.o: game.h m_flyer.h
m_gladiator.o: g_local.h q_shared.h
m_gladiator.o: game.h m_gladiator.h
m_gunner.o: g_local.h q_shared.h
m_gunner.o: game.h m_gunner.h
m_hover.o: g_local.h q_shared.h
m_hover.o: game.h m_hover.h
m_infantry.o: g_local.h q_shared.h
m_infantry.o: game.h
m_infantry.o: m_infantry.h
m_insane.o: g_local.h q_shared.h
m_insane.o: game.h m_insane.h
m_medic.o: g_local.h q_shared.h
m_medic.o: game.h m_medic.h
m_move.o: g_local.h q_shared.h
m_move.o: game.h
m_mutant.o: g_local.h q_shared.h
m_mutant.o: game.h m_mutant.h
m_parasite.o: g_local.h q_shared.h
m_parasite.o: game.h
m_parasite.o: m_parasite.h
m_soldier.o: g_local.h q_shared.h
m_soldier.o: game.h
m_soldier.o: m_soldier.h
m_supertank.o: g_local.h q_shared.h
m_supertank.o: game.h m_supertank.h
m_tank.o: g_local.h q_shared.h
m_tank.o: game.h m_tank.h
maplist.o: g_local.h q_shared.h
maplist.o: game.h
p_client.o: g_local.h q_shared.h
p_client.o: game.h m_player.h
p_hook.o: g_local.h q_shared.h
p_hook.o: game.h
p_hud.o: g_local.h q_shared.h
p_hud.o: game.h
p_trail.o: g_local.h q_shared.h
p_trail.o: game.h
p_view.o: g_local.h q_shared.h
p_view.o: game.h m_player.h
p_weapon.o: g_local.h q_shared.h
p_weapon.o: game.h m_player.h x_fbomb.h x_fire.h
q_shared.o: q_shared.h
s_cam.o: g_local.h q_shared.h
s_cam.o: game.h
scanner.o: g_local.h q_shared.h
scanner.o: game.h
wf_decoy.o: g_local.h q_shared.h
wf_decoy.o: game.h m_player.h
x_fbomb.o: g_local.h q_shared.h
x_fbomb.o: game.h x_fbomb.h x_fire.h
x_fire.o: g_local.h q_shared.h
x_fire.o: game.h x_fire.h
