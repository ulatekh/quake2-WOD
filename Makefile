# ----------------------------------------------------- #
# Makefile for the Weapons Of Destruction game module   #
# for Quake II                                          #
#                                                       #
# Just type "make" to compile the                       #
#  - Weapons Of Destruction mod (game.so)               #
#                                                       #
# Dependencies:                                         #
# - None, but you need a Quake II to play.              #
#   While in theorie every client should work           #
#   Yamagi Quake II is recommended.                     #
#                                                       #
# Platforms:                                            #
# - FreeBSD                                             #
# - Linux                                               #
# - Mac OS X                                            #
# - OpenBSD                                             #
# - Windows                                             #
# ----------------------------------------------------- #

# Detect the OS
ifdef SystemRoot
OSTYPE := Windows
else
OSTYPE := $(shell uname -s)
endif

# Special case for MinGW
ifneq (,$(findstring MINGW,$(OSTYPE)))
OSTYPE := Windows
endif

# On Windows / MinGW $(CC) is undefined by default.
ifeq ($(OSTYPE),Windows)
CC := gcc
endif

# Detect the architecture
ifeq ($(OSTYPE), Windows)
ifdef PROCESSOR_ARCHITEW6432
# 64 bit Windows
ARCH := $(PROCESSOR_ARCHITEW6432)
else
# 32 bit Windows
ARCH := $(PROCESSOR_ARCHITECTURE)
endif
else
# Normalize some abiguous ARCH strings
ARCH := $(shell uname -m | sed -e 's/i.86/i386/' -e 's/amd64/x86_64/' -e 's/^arm.*/arm/')
endif

# Detect the compiler
ifeq ($(shell $(CC) -v 2>&1 | grep -c "clang version"), 1)
COMPILER := clang
COMPILERVER := $(shell $(CC)  -dumpversion | sed -e 's/\.\([0-9][0-9]\)/\1/g' -e 's/\.\([0-9]\)/0\1/g' -e 's/^[0-9]\{3,4\}$$/&00/')
else ifeq ($(shell $(CC) -v 2>&1 | grep -c -E "(gcc version|gcc-Version)"), 1)
COMPILER := gcc
COMPILERVER := $(shell $(CC)  -dumpversion | sed -e 's/\.\([0-9][0-9]\)/\1/g' -e 's/\.\([0-9]\)/0\1/g' -e 's/^[0-9]\{3,4\}$$/&00/')
else
COMPILER := unknown
endif

# ----------

# Base CFLAGS. 
#
# -O2 are enough optimizations.
# 
# -fno-strict-aliasing since the source doesn't comply
#  with strict aliasing rules and it's next to impossible
#  to get it there...
#
# -fomit-frame-pointer since the framepointer is mostly
#  useless for debugging Quake II and slows things down.
#
# -g to build allways with debug symbols. Please do not
#  change this, since it's our only chance to debug this
#  crap when random crashes happen!
#
# -fPIC for position independend code.
#
# -MMD to generate header dependencies.
ifeq ($(OSTYPE), Darwin)
CFLAGS := -O2 -fno-strict-aliasing -fomit-frame-pointer \
		  -Wall -pipe -g -fwrapv -arch i386 -arch x86_64
else
CFLAGS := -std=gnu99 -O2 -fno-strict-aliasing -fomit-frame-pointer \
		  -Wall -pipe -g -MMD -fwrapv
endif

# ----------

# Switch of some annoying warnings.
ifeq ($(COMPILER), clang)
	# -Wno-missing-braces because otherwise clang complains
	#  about totally valid 'vec3_t bla = {0}' constructs.
	CFLAGS += -Wno-missing-braces
else ifeq ($(COMPILER), gcc)
	# GCC 8.0 or higher.
	ifeq ($(shell test $(COMPILERVER) -ge 80000; echo $$?),0)
	    # -Wno-format-truncation and -Wno-format-overflow
		# because GCC spams about 50 false positives.
    	CFLAGS += -Wno-format-truncation -Wno-format-overflow
	endif
endif

# ----------

# Old CFLAGS from original Makefile.
ifeq ($(OSTYPE),Windows)
else
	CFLAGS += -D__cdecl=
endif

# ----------

# Defines the operating system and architecture
CFLAGS += -DYQ2OSTYPE=\"$(OSTYPE)\" -DYQ2ARCH=\"$(ARCH)\"

# ----------

# Base LDFLAGS.
ifeq ($(OSTYPE), Darwin)
LDFLAGS := -shared -arch i386 -arch x86_64 
else ifeq ($(OSTYPE), Windows)
LDFLAGS := -shared -static-libgcc
else
LDFLAGS := -shared -lm
endif

# ----------

# Builds everything
all: rogue

# ----------

# When make is invoked by "make VERBOSE=1" print
# the compiler and linker commands.

ifdef VERBOSE
Q :=
else
Q := @
endif

# ----------
 
# Phony targets
.PHONY : all clean rogue

# ----------
 
# Cleanup
clean:
	@echo "===> CLEAN"
	${Q}rm -Rf build release

# ----------

# The rogue game
ifeq ($(OSTYPE), Windows)
rogue:
	@echo "===> Building game.dll"
	${Q}mkdir -p release
	$(MAKE) release/game.dll
else ifeq ($(OSTYPE), Darwin)
rogue:
	@echo "===> Building game.dylib"
	${Q}mkdir -p release
	$(MAKE) release/game.dylib
else
rogue:
	@echo "===> Building game.so"
	${Q}mkdir -p release
	$(MAKE) release/game.so

release/game.so : CFLAGS += -fPIC
endif

build/%.o: %.c
	@echo "===> CC $<"
	${Q}mkdir -p $(@D)
	${Q}$(CC) -c $(CFLAGS) -o $@ $<

# ----------

WOD_OBJS_ = \
	dwm.o \
	g_ai.o \
	g_chase.o \
	g_cmds.o \
	g_combat.o \
	g_func.o \
	g_items.o \
	g_light.o \
	g_main.o \
	g_misc.o \
	g_monster.o \
	g_phys.o \
	g_save.o \
	g_spawn.o \
	g_svcmds.o \
	g_target.o \
	g_team.o \
	g_trigger.o \
	g_turret.o \
	g_utils.o \
	g_weapon.o \
	jetpack.o \
	kamikaze.o \
	lasertrip.o \
	lsight.o \
	m_actor.o \
	maplist.o \
	m_berserk.o \
	m_boss2.o \
	m_boss31.o \
	m_boss32.o \
	m_boss3.o \
	m_brain.o \
	m_chick.o \
	m_flash.o \
	m_flipper.o \
	m_float.o \
	m_flyer.o \
	m_gladiator.o \
	m_gunner.o \
	m_hover.o \
	m_infantry.o \
	m_insane.o \
	m_medic.o \
	m_move.o \
	m_mutant.o \
	m_parasite.o \
	m_soldier.o \
	m_supertank.o \
	m_tank.o \
	p_botcheck.o \
	p_client.o \
	p_hook.o \
	p_hud.o \
	p_menu.o \
	p_trail.o \
	p_view.o \
	p_weapon.o \
	q_shared.o \
	s_cam.o \
	scanner.o \
	wf_decoy.o \
	x_fbomb.o \
	x_fire.o

# ----------

# Rewrite pathes to our object directory
WOD_OBJS = $(patsubst %,build/%,$(WOD_OBJS_))

# ----------

# Generate header dependencies
WOD_DEPS= $(WOD_OBJS:.o=.d)

# ----------

# Suck header dependencies in
-include $(WOD_DEPS)

# ----------

ifeq ($(OSTYPE), Windows)
release/game.dll : $(WOD_OBJS)
	@echo "===> LD $@"
	${Q}$(CC) $(LDFLAGS) -o $@ $(WOD_OBJS)
else ifeq ($(OSTYPE), Darwin)
release/game.dylib : $(WOD_OBJS)
	@echo "===> LD $@"
	${Q}$(CC) $(LDFLAGS) -o $@ $(WOD_OBJS)
else
release/game.so : $(WOD_OBJS)
	@echo "===> LD $@"
	${Q}$(CC) $(LDFLAGS) -o $@ $(WOD_OBJS)
endif

# ----------
