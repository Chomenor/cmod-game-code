/*
* Ping Compensation - Instant Hit Weapons
* 
* This module handles ping compensation for instant hit weapons. During weapon fire trace
* calls, hitboxes for other players are shifted backwards based on the commandTime of
* the player firing the weapon.
*/

#include "mods/features/pingcomp/pc_local.h"

#define PREFIX( x ) ModPCInstantWeapons_##x
#define MOD_STATE PREFIX( state )

// Maximum ping allowed for full compensation (for instant hit weapons)
#define MAX_SHIFT_TIME 300

static struct {
	qboolean postPmoveActive;
	int postPmoveClient;

	// For mod function stacking
	ModFNType_PostPmoveActions Prev_PostPmoveActions;
	ModFNType_TrapTrace Prev_TrapTrace;
} *MOD_STATE;

/*
==============
ModPCInstantWeapons_ShiftedTrace
==============
*/
static void ModPCInstantWeapons_ShiftedTrace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
		const vec3_t end, int passEntityNum, int contentmask, int modFlags, int shiftTime, int ignoreClient ) {
	positionShiftState_t oldState = ModPCPositionShift_Shared_GetShiftState();

	ModPCPositionShift_Shared_TimeShiftClients( ignoreClient, shiftTime );

	MOD_STATE->Prev_TrapTrace( results, start, mins, maxs, end, passEntityNum, contentmask, modFlags );

	ModPCPositionShift_Shared_SetShiftState( &oldState );
}

/*
==============
(ModFN) PostPmoveActions

Store the currently active client while move is in progress.
==============
*/
LOGFUNCTION_SVOID( PREFIX(PostPmoveActions), ( pmove_t *pmove, int clientNum, int oldEventSequence ),
		( pmove, clientNum, oldEventSequence ), "G_MODFN_POSTPMOVEACTIONS" ) {
	MOD_STATE->postPmoveActive = qtrue;
	MOD_STATE->postPmoveClient = clientNum;

	MOD_STATE->Prev_PostPmoveActions( pmove, clientNum, oldEventSequence );

	MOD_STATE->postPmoveActive = qfalse;
}

/*
==============
(ModFN) TrapTrace
==============
*/
LOGFUNCTION_SVOID( PREFIX(TrapTrace), ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
		const vec3_t end, int passEntityNum, int contentmask, int modFlags ),
		( results, start, mins, maxs, end, passEntityNum, contentmask, modFlags ), "G_MODFN_TRAPTRACE" ) {
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
		ModPCInstantWeapons_ShiftedTrace( results, start, mins, maxs, end, passEntityNum, contentmask, modFlags,
				shiftTime, MOD_STATE->postPmoveClient );
	} else {
		MOD_STATE->Prev_TrapTrace( results, start, mins, maxs, end, passEntityNum, contentmask, modFlags );
	}
}

/*
================
ModPCInstantWeapons_Init
================
*/

#define INIT_FN_STACKABLE( name ) \
	MOD_STATE->Prev_##name = modfn.name; \
	modfn.name = PREFIX(name);

#define INIT_FN_OVERRIDE( name ) \
	modfn.name = PREFIX(name);

LOGFUNCTION_VOID( ModPCInstantWeapons_Init, ( void ), (), "G_MOD_INIT" ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		ModPCPositionShift_Init();

		INIT_FN_STACKABLE( PostPmoveActions );
		INIT_FN_STACKABLE( TrapTrace );
	}
}