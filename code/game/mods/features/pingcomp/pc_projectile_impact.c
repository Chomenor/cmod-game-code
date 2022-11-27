/*
* Ping Compensation - Projectile Impact
* 
* When smoothing is enabled, it is usually better for projectiles to interact with the
* displayed position of players rather than the actual position, so the amount of delay
* from smoothing does not affect aiming.
* 
* This module accomplishes this by shifting player collision positions to the time of
* the most recent frame (level.time) during projectile movement.
*/

#define MOD_PREFIX( x ) ModPCProjectileImpact_##x

#include "mods/features/pingcomp/pc_local.h"

#define STUCK_MISSILE( ent ) ( (ent)->s.pos.trType == TR_STATIONARY && ((ent)->s.eFlags&EF_MISSILE_STICK) )

static struct {
	// For mod function stacking
	ModFNType_RadiusDamage Prev_RadiusDamage;
	ModFNType_AdjustGeneralConstant Prev_AdjustGeneralConstant;
	ModFNType_PostRunFrame Prev_PostRunFrame;
} *MOD_STATE;

/*
==============
ModPCProjectileImpact_RunMissiles

Shift all clients to the smoothed position (note this is not the same as ping compensated position)
before advancing missiles and calculating impacts.
==============
*/
static void ModPCProjectileImpact_RunMissiles( void ) {
	int i;
	gentity_t *ent;
	positionShiftState_t oldState = ModPCPositionShift_Shared_GetShiftState();

	// Run tripmines before shifting so they explode on contact without delay.
	ent = &g_entities[0];
	for (i=0 ; i<level.num_entities ; i++, ent++) {
		if ( !ent->inuse ) {
			continue;
		}

		if ( ( (ent->s.eType == ET_MISSILE) || (ent->s.eType == ET_ALT_MISSILE) ) && STUCK_MISSILE( ent ) ) {
			G_RunMissile( ent );
		}
	}

	// Run other missiles with shift to compensate for smoothing.
	ModPCPositionShift_Shared_TimeShiftClients( -1, level.time );
	ent = &g_entities[0];
	for (i=0 ; i<level.num_entities ; i++, ent++) {
		if ( !ent->inuse ) {
			continue;
		}

		if ( ( (ent->s.eType == ET_MISSILE) || (ent->s.eType == ET_ALT_MISSILE) ) && !STUCK_MISSILE( ent ) ) {
			G_RunMissile( ent );
		}
	}
	ModPCPositionShift_Shared_SetShiftState( &oldState );
}

/*
============
(ModFN) RadiusDamage

Shift clients during explosions for accurate radius damage.
============
*/
LOGFUNCTION_SRET( qboolean, MOD_PREFIX(RadiusDamage),
		( vec3_t origin, gentity_t *attacker, float damage, float radius, gentity_t *ignore, int dflags, int mod ),
		( origin, attacker, damage, radius, ignore, dflags, mod ), "G_MODFN_RADIUSDAMAGE" ) {
	if ( ModPingcomp_Static_SmoothingEnabled() ) {
		if ( ( mod == MOD_GRENADE_SPLASH || mod == MOD_GRENADE_ALT_SPLASH ) && ( !ignore || ignore->s.eType != ET_PLAYER ) ) {
			// Don't shift player positions for delayed explosion mines (tripmines and timed detonations).

		} else {
			positionShiftState_t oldState = ModPCPositionShift_Shared_GetShiftState();
			qboolean result;
			int attackerNum = attacker ? attacker - g_entities : -1;

			// Shift all clients except the one who caused the blast, so photon jumping works normally.
			ModPCPositionShift_Shared_TimeShiftClients( attackerNum, level.time );
			if ( attackerNum >= 0 ) {
				ModPCPositionShift_Shared_TimeShiftClient( attackerNum, 0 );
			}

			result = MOD_STATE->Prev_RadiusDamage( origin, attacker, damage, radius, ignore, dflags, mod );
			ModPCPositionShift_Shared_SetShiftState( &oldState );
			return result;
		}
	}

	return MOD_STATE->Prev_RadiusDamage( origin, attacker, damage, radius, ignore, dflags, mod );
}

/*
==================
(ModFN) AdjustGeneralConstant

Disable the normal run missile routine.
==================
*/
int MOD_PREFIX(AdjustGeneralConstant)( generalConstant_t gcType, int defaultValue ) {
	if ( gcType == GC_SKIP_RUN_MISSILE && ModPingcomp_Static_SmoothingEnabled() ) {
		return 1;
	}

	return MOD_STATE->Prev_AdjustGeneralConstant( gcType, defaultValue );
}

/*
================
(ModFN) PostRunFrame

Perform custom run missile routine.
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(PostRunFrame), ( void ), (), "G_MODFN_POSTRUNFRAME" ) {
	MOD_STATE->Prev_PostRunFrame();

	// At this point, PostRunFrame should have been called in the ModPCPositionShift module, so
	// smoothing positions for this frame have been calculated and saved in the collision record.

	if ( ModPingcomp_Static_SmoothingEnabled() ) {
		ModPCProjectileImpact_RunMissiles();
	}
}

/*
================
ModPCProjectileImpact_Init
================
*/
LOGFUNCTION_VOID( ModPCProjectileImpact_Init, ( void ), (), "G_MOD_INIT" ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		ModPCPositionShift_Init();

		INIT_FN_STACKABLE( RadiusDamage );
		INIT_FN_STACKABLE( AdjustGeneralConstant );
		INIT_FN_STACKABLE( PostRunFrame );
	}
}
