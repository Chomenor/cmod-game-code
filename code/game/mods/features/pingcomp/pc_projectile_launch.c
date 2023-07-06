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

#define MOD_NAME ModPCProjectileLaunch

#include "mods/features/pingcomp/pc_local.h"

// Maximum ping allowed for full compensation (for projectile weapons)
#define MAX_SHIFT_TIME 300

#define MAX_FRAME_RECORD 32

static struct {
	int eventTimeOffset;

	// Keep track of server frame times
	int frameRecord[MAX_FRAME_RECORD];
	unsigned int frameCounter;
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
static int MOD_PREFIX(AdjustWeaponConstant)( MODFN_CTV, weaponConstant_t wcType, int defaultValue ) {
	// Disable initial welder projectile think, so it can be handled here with appropriate shifting
	if ( wcType == WC_WELDER_SKIP_INITIAL_THINK && ModPingcomp_Static_ProjectileCompensationEnabled() ) {
		return 1;
	}

	return MODFN_NEXT( AdjustWeaponConstant, ( MODFN_NC, wcType, defaultValue ) );
}

/*
==================
(ModFN) AdjustGeneralConstant
==================
*/
int MOD_PREFIX(AdjustGeneralConstant)( MODFN_CTV, generalConstant_t gcType, int defaultValue ) {
	if ( gcType == GC_EVENT_TIME_OFFSET ) {
		return MOD_STATE->eventTimeOffset;
	}

	return MODFN_NEXT( AdjustGeneralConstant, ( MODFN_NC, gcType, defaultValue ) );
}

/*
======================
(ModFN) PostFireProjectile
======================
*/
static void MOD_PREFIX(PostFireProjectile)( MODFN_CTV, gentity_t *projectile ) {
	if ( projectile->inuse ) {
		MODFN_NEXT( PostFireProjectile, ( MODFN_NC, projectile ) );
	}

	if ( projectile->inuse && ModPingcomp_Static_ProjectileCompensationEnabled() ) {
		ModPCProjectileLaunch_AdvanceProjectile( projectile );
	}
}

/*
================
(ModFN) PostRunFrame

Log the frame time in order to replay frames accurately.
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	MOD_STATE->frameRecord[( MOD_STATE->frameCounter++ ) % MAX_FRAME_RECORD] = level.time;
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );
}

/*
================
ModPCProjectileLaunch_Init
================
*/
void ModPCProjectileLaunch_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		ModPCPositionShift_Init();

		MODFN_REGISTER( AdjustWeaponConstant, MODPRIORITY_GENERAL );
		MODFN_REGISTER( AdjustGeneralConstant, MODPRIORITY_GENERAL );
		MODFN_REGISTER( PostFireProjectile, MODPRIORITY_HIGH );
		MODFN_REGISTER( PostRunFrame, MODPRIORITY_GENERAL );
	}
}
