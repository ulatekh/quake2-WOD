/*==========================================================================
//  x_fire.h -- by Patrick Martin               Last updated:  2-19-1998
//--------------------------------------------------------------------------
//  This is the header file for x_fire.c.  Any files that use the
//  functions in x_fire.c must include this file.
//========================================================================*/

/***************/
/*  CONSTANTS  */
/***************/

/* Time in seconds the entity on fire will burn. */
#define BURN_TIME   30

/* Important frames in my flame model. */
#define FRAME_FIRST_SMALLFIRE   0
#define FRAME_LAST_SMALLFIRE    5
#define FRAME_FIRST_LARGEFIRE   6
#define FRAME_LAST_LARGEFIRE   15
#define FRAME_FLAMEOUT         16

/* Path to my flame model.  (Used to be "models/fire/tris.md2".) */
#define MD2_FIRE  gi.modelindex("fire.md2")


/****************/
/*  PROTOTYPES  */
/****************/

/*
// g_misc.c
*/
void BecomeExplosion2 (edict_t *self);

/*
// x_fire.c
*/
void check_firedodge
 (edict_t *self, vec3_t start, vec3_t dir, int speed);

void     PBM_BecomeSmoke (edict_t *self);
void     PBM_BecomeSteam (edict_t *self);
void     PBM_SmallExplodeThink (edict_t *self);
void     PBM_BecomeNewExplosion (edict_t *self);

qboolean PBM_InWater (edict_t *ent);
qboolean PBM_Flammable (edict_t *ent);
qboolean PBM_FireResistant (edict_t *ent);
void     PBM_BurnDamage (edict_t *victim, edict_t *fire, vec3_t damage);
void     PBM_BurnRadius
 (edict_t *fire, float radius, vec3_t damage, edict_t *ignore);
void     PBM_FireSpot (vec3_t spot, edict_t *ent);
qboolean PBM_FlameOut (edict_t *self);
void     PBM_Burn (edict_t *self);
void     PBM_Ignite (edict_t *victim, edict_t *attacker);

void     PBM_CheckFire (edict_t *self);
void     PBM_FireDropTouch
 (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);
void     PBM_FireDrop (edict_t *self);

void     PBM_CloudBurst (edict_t *self);
void     PBM_FlameCloud
 (edict_t *attacker, vec3_t start, vec3_t cloud, vec3_t timer,
  qboolean deadly, float radius, vec3_t damage, vec3_t radius_damage,
  int rain_chance, int blast_chance);

void     PBM_FireballTouch
 (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);
void     PBM_FireFlamer
 (edict_t *self, vec3_t start, vec3_t dir, int speed, float radius,
  vec3_t damage, vec3_t radius_damage, int rain_chance, int blast_chance);


/*=============================  END OF FILE =============================*/
