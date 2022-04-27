/*
* Ping Compensation - Projectile Launch
* 
* This module handles projectile ping compensation. When a projectile weapon is fired,
* the client command time is used as the launch time, and the projectile is advanced
* forward to the server level time.
* 
* The times of previous server frames are used as increments as the projectile is advanced,
* for consistency with standard behavior and to allow more accurate client prediction.
* 
* For example, suppose a client has a 100 ping and the server framerate is 40fps. The
* client command time would be around 100ms behind the server time, so the projectile
* would be advanced by 100ms as soon as it is fired, in 25ms increments.
*/

#include "mods/features/pingcomp/pc_local.h"

#define PREFIX( x ) ModPCProjectileLaunch_##x
#define MOD_STATE PREFIX( state )

// Maximum ping allowed for full compensation (for projectile weapons)
#define MAX_SHIFT_TIME 300

#define MAX_FRAME_RECORD 32

static struct {
	int eventTimeOffset;

	// Keep track of server frame times
	int frameRecord[MAX_FRAME_RECORD];
	unsigned int frameCounter;

	// For mod function stacking
	ModFNType_AdjustWeaponConstant Prev_AdjustWeaponConstant;
	ModFNType_AdjustGeneralConstant Prev_AdjustGeneralConstant;
	ModFNType_PostFireProjectile Prev_PostFireProjectile;
	ModFNType_PostRunFrame Prev_PostRunFrame;
} *MOD_STATE;

/*
======================
ModPCProjectileLaunch_AdvanceProjectile

Advance projectile forward to compensate for the ping of the player who launched it.
======================
*/
static void ModPCProjectileLaunch_AdvanceProjectile( gentity_t *projectile ) {
	int clientNum = projectile->parent - g_entities;

	if ( G_AssertConnectedClient( clientNum ) ) {
		int startTime = level.clients[clientNum].ps.commandTime;
		int shift = level.time - startTime;

		if ( shift > MAX_SHIFT_TIME ) {
			shift = MAX_SHIFT_TIME;
			startTime = level.time - MAX_SHIFT_TIME;
		}

		if ( shift > 0 ) {
			unsigned int frameIndex;
			positionShiftState_t oldState = ModPCPositionShift_Shared_GetShiftState();
			int oldLevelTime = level.time;
			int oldPreviousTime = level.previousTime;

			projectile->nextthink -= shift;
			projectile->s.pos.trTime -= shift;
			level.previousTime = startTime;

			// perform initial welder burst think.
			if ( projectile->s.weapon == WP_DREADNOUGHT ) {
				level.time = startTime;
				ModPCPositionShift_Shared_TimeShiftClients( -1, level.time );
				MOD_STATE->eventTimeOffset = oldLevelTime - level.time;
				DreadnoughtBurstThink( projectile );
			}

			// advance missile using time indexes of previous frames.
			for ( frameIndex = MOD_STATE->frameCounter > MAX_FRAME_RECORD ? MOD_STATE->frameCounter - MAX_FRAME_RECORD : 0 ;
					frameIndex < MOD_STATE->frameCounter; ++frameIndex ) {
				level.time = MOD_STATE->frameRecord[frameIndex % MAX_FRAME_RECORD];
				if ( level.time <= level.previousTime ) {
					continue;
				}

				// note: currently only shifting players, not other moving entities...
				ModPCPositionShift_Shared_TimeShiftClients( -1, level.time );

				MOD_STATE->eventTimeOffset = oldLevelTime - level.time;
				G_RunMissile( projectile );
				if ( !projectile->inuse || !( projectile->s.eType == ET_MISSILE || projectile->s.eType == ET_ALT_MISSILE ) ) {
					// the missile has probably exploded
					goto endMove;
				}
				level.previousTime = level.time;
			}

			EF_WARN_ASSERT( level.time == oldLevelTime );

			endMove:
			level.time = oldLevelTime;
			level.previousTime = oldPreviousTime;
			MOD_STATE->eventTimeOffset = 0;
			ModPCPositionShift_Shared_SetShiftState( &oldState );
		}

		else {
			if ( projectile->s.weapon == WP_DREADNOUGHT ) {
				DreadnoughtBurstThink( projectile );
			}
		}
	}
}

/*
======================
(ModFN) AdjustWeaponConstant
======================
*/
static int PREFIX(AdjustWeaponConstant)( weaponConstant_t wcType, int defaultValue ) {
	// Disable initial welder projectile think, so it can be handled here with appropriate shifting
	if ( wcType == WC_WELDER_SKIP_INITIAL_THINK && ModPingcomp_Static_ProjectileCompensationEnabled() ) {
		return 1;
	}

	return MOD_STATE->Prev_AdjustWeaponConstant( wcType, defaultValue );
}

/*
==================
(ModFN) AdjustGeneralConstant
==================
*/
int PREFIX(AdjustGeneralConstant)( generalConstant_t gcType, int defaultValue ) {
	if ( gcType == GC_EVENT_TIME_OFFSET ) {
		return MOD_STATE->eventTimeOffset;
	}

	return MOD_STATE->Prev_AdjustGeneralConstant( gcType, defaultValue );
}

/*
======================
(ModFN) PostFireProjectile
======================
*/
LOGFUNCTION_SVOID( PREFIX(PostFireProjectile), ( gentity_t *projectile ), ( projectile ), "G_MODFN_POSTFIREPROJECTILE" ) {
	if ( projectile->inuse && ModPingcomp_Static_ProjectileCompensationEnabled() ) {
		ModPCProjectileLaunch_AdvanceProjectile( projectile );
	}

	if ( projectile->inuse ) {
		MOD_STATE->Prev_PostFireProjectile( projectile );
	}
}

/*
================
(ModFN) PostRunFrame

Log the frame time in order to replay frames accurately.
================
*/
LOGFUNCTION_SVOID( PREFIX(PostRunFrame), (void), (), "G_MODFN_POSTRUNFRAME" ) {
	MOD_STATE->frameRecord[( MOD_STATE->frameCounter++ ) % MAX_FRAME_RECORD] = level.time;
	MOD_STATE->Prev_PostRunFrame();
}

/*
================
ModPCProjectileLaunch_Init
================
*/

#define INIT_FN_STACKABLE( name ) \
	MOD_STATE->Prev_##name = modfn.name; \
	modfn.name = PREFIX(name);

#define INIT_FN_OVERRIDE( name ) \
	modfn.name = PREFIX(name);

LOGFUNCTION_VOID( ModPCProjectileLaunch_Init, ( void ), (), "G_MOD_INIT" ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		ModPCPositionShift_Init();

		INIT_FN_STACKABLE( AdjustWeaponConstant );
		INIT_FN_STACKABLE( AdjustGeneralConstant );
		INIT_FN_STACKABLE( PostFireProjectile );
		INIT_FN_STACKABLE( PostRunFrame );
	}
}