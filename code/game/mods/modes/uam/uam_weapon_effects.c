/*
* UAM Weapon Effects
* 
* Add some extra weapon sound effects consistent with Gladiator mod.
*/

#define MOD_NAME ModUAMWeaponEffects

#include "mods/modes/uam/uam_local.h"

static struct {
	int _unused;
} *MOD_STATE;

/*
======================
(ModFN) AddWeaponEffect
======================
*/
static void MOD_PREFIX(AddWeaponEffect)( MODFN_CTV, weaponEffect_t weType, gentity_t *ent, trace_t *trace ) {
	if ( weType == WE_IMOD_END ) {
		if ( !( trace->surfaceFlags & SURF_NOIMPACT ) ) {
			gentity_t *tent = G_TempEntity( trace->endpos, EV_GENERAL_SOUND );
			tent->s.eventParm = G_SoundIndex( "sound/weapons/scavenger/hit_wall.wav" );
		}
	}

	else if ( weType == WE_IMOD_RIFLE ) {
		gentity_t *tent = G_TempEntity( trace->endpos, EV_GENERAL_SOUND );
		gentity_t *traceEnt = &g_entities[trace->entityNum];
		if ( traceEnt->client && traceEnt->health > 0 ) {
			tent->s.eventParm = G_SoundIndex( "sound/ambience/stasis/breakfield.mp3" );
		} else {
			tent->s.eventParm = G_SoundIndex( "sound/weapons/scavenger/hit_wall.wav" );
		}
	}

	else {
		MODFN_NEXT( AddWeaponEffect, ( MODFN_NC, weType, ent, trace ) );
	}
}

/*
======================
(ModFN) PostFireProjectile

Add looping sound to certain weapon projectiles.
======================
*/
static void MOD_PREFIX(PostFireProjectile)( MODFN_CTV, gentity_t *projectile ) {
	MODFN_NEXT( PostFireProjectile, ( MODFN_NC, projectile ) );

	if ( projectile->methodOfDeath == MOD_GRENADE && !projectile->s.loopSound ) {
		projectile->s.loopSound = G_SoundIndex( "sound/ambience/steamloop1.wav" );
	}

	if ( projectile->methodOfDeath == MOD_QUANTUM && !projectile->s.loopSound ) {
		projectile->s.loopSound = G_SoundIndex( "sound/ambience/borg/bluebeam.wav" );
	}

	if ( projectile->methodOfDeath == MOD_QUANTUM_ALT && !projectile->s.loopSound ) {
		projectile->s.loopSound = G_SoundIndex( "sound/ambience/voyager/corermamb1.wav" );
	}
}

/*
================
ModUAMWeaponEffects_Init
================
*/
void ModUAMWeaponEffects_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( AddWeaponEffect, ++modePriorityLevel );
		MODFN_REGISTER( PostFireProjectile, ++modePriorityLevel );
	}
}
