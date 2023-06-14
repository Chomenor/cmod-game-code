/*
* Ping Compensation - Position Shift
* 
* This module provides core functionality for backwards shifting and restoring player
* collision positions. It has no effect by itself, but is used by other modules
* to implement ping compensation features.
* 
* Functions ModPCPositionShift_Shared_TimeShiftClient and ModPCPositionShift_Shared_TimeShiftClients
* are used to shift clients to a position corresponding to a certain server time.
* 
* Functions ModPCPositionShift_Shared_GetShiftState and ModPCPositionShift_Shared_SetShiftState
* allow temporarily saving and restoring the state of which clients are shifted, which can be
* useful for handling nested cases such as projectile movement and explosions.
*/

#define MOD_NAME ModPCPositionShift

#include "mods/features/pingcomp/pc_local.h"

#define WORKING_CLIENT_COUNT ( level.maxclients < POSITION_SHIFT_MAX_CLIENTS ? level.maxclients : POSITION_SHIFT_MAX_CLIENTS )
#define CLIENT_IN_RANGE( clientNum ) ( clientNum >= 0 && clientNum < WORKING_CLIENT_COUNT )

#define MAX_CLIENT_SNAPS 32
#define CLIENT_SNAP( clientNum, index ) MOD_STATE->clients[clientNum].snaps[ ( index ) % MAX_CLIENT_SNAPS ]

// Workaround for float/char conversion issues with QVM VectorCopy
// See VectorCopy definition in q_shared.h
#define VECTORCOPY_INT(a,b) ((b)[0]=(int)(a)[0],(b)[1]=(int)(a)[1],(b)[2]=(int)(a)[2])

typedef struct {
	vec3_t origin;
	char mins[3];
	char maxs[3];
} position_client_position_t;

typedef struct {
	int snapshotTime;
	position_client_position_t pos;
} position_client_snap_t;

typedef struct {
	position_client_snap_t snaps[MAX_CLIENT_SNAPS];
	position_client_position_t oldPosition;		// temporary while shift in progress
	int snapCounter;
} position_client_t;

static struct {
	position_client_t clients[POSITION_SHIFT_MAX_CLIENTS];
	positionShiftState_t currentState;
} *MOD_STATE;

/*
================
ModPCPositionShift_RestoreClientPosition

Restores position data for single client.
================
*/
static void ModPCPositionShift_RestoreClientPosition( int clientNum, const position_client_position_t *position ) {
	gentity_t *ent = &g_entities[clientNum];
	VectorCopy( position->origin, ent->r.currentOrigin );
	VECTORCOPY_INT( position->mins, ent->r.mins );
	VECTORCOPY_INT( position->maxs, ent->r.maxs );
	trap_LinkEntity( ent );
}

/*
================
ModPCPositionShift_StoreClientPosition

Saves position data for single client.
================
*/
static void ModPCPositionShift_StoreClientPosition( int clientNum, position_client_position_t *position,
		Smoothing_ShiftInfo_t *shiftInfo ) {
	gentity_t *ent = &g_entities[clientNum];
	if ( shiftInfo ) {
		VectorCopy( ent->s.pos.trBase, position->origin );
		VECTORCOPY_INT( shiftInfo->mins, position->mins );
		VECTORCOPY_INT( shiftInfo->maxs, position->maxs );
	} else {
		VectorCopy( ent->r.currentOrigin, position->origin );
		VECTORCOPY_INT( ent->r.mins, position->mins );
		VECTORCOPY_INT( ent->r.maxs, position->maxs );
	}
}

/*
================
ModPCPositionShift_RunServerFrame

Performs smoothing adjustments if enabled, and stores position of all clients.
================
*/
static void ModPCPositionShift_RunServerFrame( void ) {
	int clientCount = WORKING_CLIENT_COUNT;
	Smoothing_ShiftInfo_t shiftInfo;
	int i;

	for ( i = 0; i < clientCount; ++i ) {
		if ( g_entities[i].r.linked ) {
			position_client_t *modclient = &MOD_STATE->clients[i];
			position_client_snap_t *snap = &CLIENT_SNAP( i, modclient->snapCounter );
			modclient->snapCounter++;

			snap->snapshotTime = level.time;
			if ( ModPCSmoothing_Static_ShiftClient( i, &shiftInfo ) ) {
				ModPCPositionShift_StoreClientPosition( i, &snap->pos, &shiftInfo );
			} else {
				ModPCPositionShift_StoreClientPosition( i, &snap->pos, NULL );
			}
		}
	}
}

/*
==============
ModPCPositionShift_LerpVector

Creates interpolation between 'base' and 'next'.
Proportion is indicated by 'fraction' on a scale from 0 (only base) to 1 (only next).
==============
*/
static void ModPCPositionShift_LerpVector( vec3_t base, vec3_t next, float fraction, vec3_t target ) {
	target[0] = base[0] + ( next[0] - base[0] ) * fraction;
	target[1] = base[1] + ( next[1] - base[1] ) * fraction;
	target[2] = base[2] + ( next[2] - base[2] ) * fraction;
}

/*
================
ModPCPositionShift_Shared_TimeShiftClient

Set the position of specified client to the archived position corresponding to the given server time.
Time 0 indicates to restore client to unshifted position.
================
*/
void ModPCPositionShift_Shared_TimeShiftClient( int clientNum, int time ) {
	EF_ERR_ASSERT( MOD_STATE );

	if ( CLIENT_IN_RANGE( clientNum ) && g_entities[clientNum].r.linked &&
			MOD_STATE->currentState.shiftTime[clientNum] != time ) {
		position_client_t *modclient = &MOD_STATE->clients[clientNum];

		if ( time == 0 ) {
			ModPCPositionShift_RestoreClientPosition( clientNum, &modclient->oldPosition );

		} else {
			int currentFrame = modclient->snapCounter - 1;
			int oldestFrame = modclient->snapCounter - MAX_CLIENT_SNAPS > 0 ? modclient->snapCounter - MAX_CLIENT_SNAPS : 0;

			if ( MOD_STATE->currentState.shiftTime[clientNum] == 0 ) {
				ModPCPositionShift_StoreClientPosition( clientNum, &modclient->oldPosition, NULL );
			}

			while ( currentFrame > oldestFrame && time < CLIENT_SNAP( clientNum, currentFrame ).snapshotTime ) {
				--currentFrame;
			}

			// see if we have a valid next snap to interpolate to
			if ( time > CLIENT_SNAP( clientNum, currentFrame ).snapshotTime && currentFrame < modclient->snapCounter - 1 &&
					EF_WARN_ASSERT( CLIENT_SNAP( clientNum, currentFrame + 1 ).snapshotTime > time ) ) {
				position_client_snap_t *current_frame = &CLIENT_SNAP( clientNum, currentFrame );
				position_client_snap_t *next_frame = &CLIENT_SNAP( clientNum, currentFrame + 1 );
				position_client_position_t lerpPos = current_frame->pos;
				int delta = next_frame->snapshotTime - current_frame->snapshotTime;
				float lerpFraction = delta ? (float)( time - current_frame->snapshotTime ) / (float)( delta ) : 0.0f;

				ModPCPositionShift_LerpVector( current_frame->pos.origin, next_frame->pos.origin, lerpFraction, lerpPos.origin );
				ModPCPositionShift_RestoreClientPosition( clientNum, &lerpPos );

			} else {
				ModPCPositionShift_RestoreClientPosition( clientNum, &CLIENT_SNAP( clientNum, currentFrame ).pos );
			}
		}

		MOD_STATE->currentState.shiftTime[clientNum] = time;
	}
}

/*
================
ModPCPositionShift_Shared_TimeShiftClients

Set the position of all clients to the archived position corresponding to the given server time.
Specify time 0 to reset clients to unshifted position.
Specify ignoreClient -1 to shift all clients.
================
*/
void ModPCPositionShift_Shared_TimeShiftClients( int ignoreClient, int time ) {
	int clientCount = WORKING_CLIENT_COUNT;
	int i;

	for ( i = 0; i < clientCount; ++i ) {
		if ( i != ignoreClient ) {
			ModPCPositionShift_Shared_TimeShiftClient( i, time );
		}
	}
}

/*
================
ModPCPositionShift_Shared_GetShiftState

Retrieves the current state of shifted clients.
================
*/
positionShiftState_t ModPCPositionShift_Shared_GetShiftState( void ) {
	EF_ERR_ASSERT( MOD_STATE );
	return MOD_STATE->currentState;
}

/*
================
ModPCPositionShift_Shared_SetShiftState

Applies the stored state of shifted clients.
================
*/
void ModPCPositionShift_Shared_SetShiftState( positionShiftState_t *shiftState ) {
	int clientCount = WORKING_CLIENT_COUNT;
	int i;

	for ( i = 0; i < clientCount; ++i ) {
		ModPCPositionShift_Shared_TimeShiftClient( i, shiftState->shiftTime[i] );
	}
}

/*
================
(ModFN) PostRunFrame
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	if ( ModPingcomp_Static_PositionShiftEnabled() ) {
		ModPCPositionShift_RunServerFrame();
	}

	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );
}

/*
================
ModPCPositionShift_Init
================
*/
void ModPCPositionShift_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( PostRunFrame, MODPRIORITY_GENERAL );
	}
}
