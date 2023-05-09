/*
* Ping Compensation - Instant Hit Weapons
* 
* This module handles ping compensation for instant hit weapons. During weapon fire trace
* calls, hitboxes for other players are shifted backwards based on the commandTime of
* the player firing the weapon.
*/

#define MOD_NAME ModPCInstantWeapons

#include "mods/features/pingcomp/pc_local.h"

// Maximum ping allowed for full compensation (for instant hit weapons)
#define MAX_SHIFT_TIME 300

static struct {
	qboolean postPmoveActive;
	int postPmoveClient;
} *MOD_STATE;

/*
==============
ModPCInstantWeapons_ShiftedTrace
==============
*/
static void ModPCInstantWeapons_ShiftedTrace( MODFN_CTV, trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
		const vec3_t end, int passEntityNum, int contentmask, int modFlags, int shiftTime, int ignoreClient ) {
	positionShiftState_t oldState = ModPCPositionShift_Shared_GetShiftState();

	ModPCPositionShift_Shared_TimeShiftClients( ignoreClient, shiftTime );

	MODFN_NEXT( TrapTrace, ( MODFN_NC, results, start, mins, maxs, end, passEntityNum, contentmask, modFlags ) );

	ModPCPositionShift_Shared_SetShiftState( &oldState );
}

/*
==============
(ModFN) PostPmoveActions

Store the currently active client while move is in progress.
==============
*/
static void MOD_PREFIX(PostPmoveActions)( MODFN_CTV, pmove_t *pmove, int clientNum, int oldEventSequence ) {
	MOD_STATE->postPmoveActive = qtrue;
	MOD_STATE->postPmoveClient = clientNum;

	MODFN_NEXT( PostPmoveActions, ( MODFN_NC, pmove, clientNum, oldEventSequence ) );

	MOD_STATE->postPmoveActive = qfalse;
}

/*
==============
(ModFN) TrapTrace
==============
*/
static void MOD_PREFIX(TrapTrace)( MODFN_CTV, trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
		const vec3_t end, int passEntityNum, int contentmask, int modFlags ) {
	int shiftTime = 0;
	if ( MOD_STATE->postPmoveActive ) {
		gclient_t *client = &level.clients[MOD_STATE->postPmoveClient];
		if ( ModPingcomp_Static_PingCompensationEnabled() && client->ps.weapon != WP_VOYAGER_HYPO &&
				client->ps.weapon != WP_BORG_ASSIMILATOR ) {
			// Perform full ping compensated shift
			int shift = level.time - client->ps.commandTime;
			if ( shift > MAX_SHIFT_TIME ) {
				shift = MAX_SHIFT_TIME;
			}
			shiftTime = level.time - shift;
		} else if ( ModPingcomp_Static_SmoothingEnabled() ) {
			// Smoothing only, so just shift to the latest position
			shiftTime = level.time;
		}
	}

	if ( shiftTime ) {
		ModPCInstantWeapons_ShiftedTrace( MODFN_CTN, results, start, mins, maxs, end, passEntityNum, contentmask, modFlags,
				shiftTime, MOD_STATE->postPmoveClient );
	} else {
		MODFN_NEXT( TrapTrace, ( MODFN_NC, results, start, mins, maxs, end, passEntityNum, contentmask, modFlags ) );
	}
}

/*
================
ModPCInstantWeapons_Init
================
*/
void ModPCInstantWeapons_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		ModPCPositionShift_Init();

		MODFN_REGISTER( PostPmoveActions, MODPRIORITY_GENERAL );
		MODFN_REGISTER( TrapTrace, MODPRIORITY_GENERAL );
	}
}
