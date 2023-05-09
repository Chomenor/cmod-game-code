/*
* Ping Compensation - Dead Move
* 
* If a player dies while in motion while smoothing is enabled, it can lead to
* inconsistent visual effects as the adjusted view catches up to the playerstate.
* 
* This module provides a workaround for this issue. When a player dies, control
* of the adjusted view is transferred to this module, and movement of the body
* is simulated starting from the point of death.
*/

#define MOD_NAME ModPCDeadMove

#include "mods/features/pingcomp/pc_local.h"

#define WORKING_CLIENT_COUNT ( level.maxclients < MAX_SMOOTHING_CLIENTS ? level.maxclients : MAX_SMOOTHING_CLIENTS )
#define CLIENT_IN_RANGE( clientNum ) ( clientNum >= 0 && clientNum < WORKING_CLIENT_COUNT )

typedef struct {
	qboolean deadMoveActive;
	int lastMoveTime;
	vec3_t origin;
	vec3_t velocity;
} PingcompDeadMove_deadState_t;

static struct {
	PingcompDeadMove_deadState_t deadStates[MAX_SMOOTHING_CLIENTS];
} *MOD_STATE;

/*
==============
ModPCDeadMove_RunDeadMove
==============
*/
static void ModPCDeadMove_RunDeadMove( playerState_t *ps, int startTime, int endTime ) {
	pmove_t pm;
	memset( &pm, 0, sizeof( pm ) );
	ps->commandTime = startTime;
	pm.ps = ps;
	pm.cmd.serverTime = endTime;
	pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	pm.trace = trap_Trace;
	pm.pointcontents = trap_PointContents;
	Pmove( &pm, 0, NULL, NULL );
}

/*
==============
ModPCDeadMove_Static_DeadMoveActive

Returns whether dead move is currently active for given client.
==============
*/
qboolean ModPCDeadMove_Static_DeadMoveActive( int clientNum ) {
	return MOD_STATE && CLIENT_IN_RANGE( clientNum ) &&
			MOD_STATE->deadStates[clientNum].deadMoveActive &&
			level.clients[clientNum].ps.pm_type == PM_DEAD ? qtrue : qfalse;
}

/*
==============
ModPCDeadMove_Static_ShiftClient

If dead move is active for client, advances position and updates client entity position.
Returns qtrue on success, qfalse on error or dead move inactive.
On success, additional data will be written to info_out for collision handling purposes.
==============
*/
qboolean ModPCDeadMove_Static_ShiftClient( int clientNum, Smoothing_ShiftInfo_t *info_out ) {
	if ( ModPCDeadMove_Static_DeadMoveActive( clientNum ) ) {
		gentity_t *ent = &g_entities[clientNum];
		PingcompDeadMove_deadState_t *deadState = &MOD_STATE->deadStates[clientNum];
		playerState_t movePS = level.clients[clientNum].ps;

		// advance deadState position
		VectorCopy( deadState->origin, movePS.origin );
		VectorCopy( deadState->velocity, movePS.velocity );
		ModPCDeadMove_RunDeadMove( &movePS, deadState->lastMoveTime, level.time );
		VectorCopy( movePS.origin, deadState->origin );
		VectorCopy( movePS.velocity, deadState->velocity );
		deadState->lastMoveTime = level.time;

		// update client entity position
		VectorCopy( movePS.origin, ent->s.pos.trBase );
		VectorCopy( movePS.viewangles, ent->s.apos.trBase );
		ent->s.angles2[1] = movePS.movementDir;
		ent->s.legsAnim = movePS.legsAnim;
		if ( info_out ) {
			VectorSet( info_out->mins, -15, -15, -24 );
			VectorSet( info_out->maxs, 15, 15, -8 );
		}
		return qtrue;
	}

	return qfalse;
}

/*
==============
ModPCDeadMove_Static_InitDeadMove

Activates dead move mode for specified client, enabling use of ModPCDeadMove_Static_ShiftClient.
Dead mode will be automatically reset when player respawns/disconnects.
==============
*/
void ModPCDeadMove_Static_InitDeadMove( int clientNum, vec3_t smoothingOrigin ) {
	if ( MOD_STATE && clientNum >= 0 && clientNum < WORKING_CLIENT_COUNT &&
			EF_WARN_ASSERT( level.clients[clientNum].ps.pm_type == PM_DEAD ) &&
			!MOD_STATE->deadStates[clientNum].deadMoveActive ) {
		PingcompDeadMove_deadState_t *deadState = &MOD_STATE->deadStates[clientNum];
		deadState->deadMoveActive = qtrue;
		deadState->lastMoveTime = level.time;
		VectorCopy( smoothingOrigin, deadState->origin );
		VectorCopy( level.clients[clientNum].ps.velocity, deadState->velocity );
	}
}

/*
================
(ModFN) PostRunFrame
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	int i;
	int clientCount = MAX_SMOOTHING_CLIENTS;

	for ( i = 0; i < clientCount; ++i ) {
		if ( MOD_STATE->deadStates[i].deadMoveActive &&
				( level.clients[i].pers.connected != CON_CONNECTED || level.clients[i].ps.pm_type != PM_DEAD ) ) {
			MOD_STATE->deadStates[i].deadMoveActive = qfalse;
		}
	}

	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );
}

/*
================
ModPCDeadMove_Init
================
*/
void ModPCDeadMove_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( PostRunFrame, MODPRIORITY_GENERAL );
	}
}
