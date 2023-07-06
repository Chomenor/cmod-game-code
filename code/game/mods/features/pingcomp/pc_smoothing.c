/*
* Player Smoothing
* 
* Normally, player movement is not in sync with server frames, and the player commandTime
* can increase by different amounts between each server frame. This can lead to jerky
* player movement, especially for laggy players.
* 
* This module compensates for this by shifting visible player positions slightly backwards
* in time such that the commandTime increases by roughly the same amount each server frame,
* resulting in the appeareance of smooth movement.
*/

#define MOD_NAME ModPCSmoothing

#include "mods/features/pingcomp/pc_local.h"

#define WORKING_CLIENT_COUNT ( level.maxclients < MAX_SMOOTHING_CLIENTS ? level.maxclients : MAX_SMOOTHING_CLIENTS )
#define CLIENT_IN_RANGE( clientNum ) ( clientNum >= 0 && clientNum < WORKING_CLIENT_COUNT )

#define MAX_CLIENT_SNAPS 64
#define CLIENT_SNAP( clientNum, index ) MOD_STATE->clients[clientNum].snaps[ ( index ) % MAX_CLIENT_SNAPS ]

// Skip storing snaps less than this many milliseconds from the previous one
#define MINIMUM_SNAP_LENGTH 5

// Workaround for float/char conversion issues with QVM VectorCopy
// See VectorCopy definition in q_shared.h
#define VECTORCOPY_INT(a,b) ((b)[0]=(int)(a)[0],(b)[1]=(int)(a)[1],(b)[2]=(int)(a)[2])

typedef struct {
	int commandTime;
	vec3_t origin;
	vec3_t viewangles;
	vec3_t velocity;
	unsigned char liftCount;
	char mins[3];
	char maxs[3];
	char movementDir;
	char legsAnim;
} smoothing_client_snap_t;

typedef struct {
	smoothing_client_snap_t snaps[MAX_CLIENT_SNAPS];
	int snapCounter;
	qboolean teleportFlag;
	qboolean deadOnMover;
	unsigned char liftCount;
	float lastMoveZ;
	float lastLiftZ;
} smoothing_client_t;

static struct {
	smoothing_client_t clients[MAX_SMOOTHING_CLIENTS];

	// Set while G_Damage is in progress
	qboolean currentDamage;
	vec3_t currentDamageOldVelocity;
} *MOD_STATE;

/*
==============
ModPCSmoothing_ReadClientSnap

Generates a smoothing snap from the client's current position.
==============
*/
static void ModPCSmoothing_ReadClientSnap( int clientNum, smoothing_client_snap_t *snap ) {
	const gclient_t *client = &level.clients[clientNum];
	const gentity_t *ent = &g_entities[clientNum];
	smoothing_client_t *modclient = &MOD_STATE->clients[clientNum];

	snap->commandTime = client->ps.commandTime;
	VectorCopy( client->ps.origin, snap->origin );
	VectorCopy( client->ps.velocity, snap->velocity );
	VectorCopy( client->ps.viewangles, snap->viewangles );
	VECTORCOPY_INT( ent->r.mins, snap->mins );
	VECTORCOPY_INT( ent->r.maxs, snap->maxs );
	snap->movementDir = client->ps.movementDir;
	snap->legsAnim = (char)client->ps.legsAnim;
	snap->liftCount = modclient->liftCount;
}

/*
==============
ModPCSmoothing_ApplyClientSnap

Apply stored client position to entity position that will be sent out to other clients.
==============
*/
static void ModPCSmoothing_ApplyClientSnap( int clientNum, const smoothing_client_snap_t *snap ) {
	gentity_t *ent = &g_entities[clientNum];
	VectorCopy( snap->origin, ent->s.pos.trBase );
	SnapVector( ent->s.pos.trBase );	// maybe not ideal, but network savings do seem significant
	VectorCopy( snap->viewangles, ent->s.apos.trBase );
	SnapVector( ent->s.apos.trBase );
	ent->s.angles2[1] = (int)snap->movementDir;
	ent->s.legsAnim = snap->legsAnim;
}

/*
==============
ModPCSmoothing_CheckLiftMove

Check for lift movement during server frame. If movement was detected, store the Z origin
and increment the lift counter.
==============
*/
static void ModPCSmoothing_CheckLiftMove( int clientNum ) {
	gclient_t *client = &level.clients[clientNum];
	smoothing_client_t *modclient = &MOD_STATE->clients[clientNum];
	if ( modclient->lastMoveZ != client->ps.origin[2] ) {
		modclient->lastMoveZ = client->ps.origin[2];
		modclient->lastLiftZ = client->ps.origin[2];
		modclient->liftCount++;
	}
}

/*
==============
ModPCSmoothing_CheckTeleport

If teleport is detected, reset prior snaps to avoid bad interpolations.
==============
*/
static void ModPCSmoothing_CheckTeleport( int clientNum ) {
	qboolean teleportFlag = ( level.clients[clientNum].ps.eFlags & EF_TELEPORT_BIT ) ? qtrue : qfalse;
	if ( MOD_STATE->clients[clientNum].teleportFlag != teleportFlag ) {
		MOD_STATE->clients[clientNum].teleportFlag = teleportFlag;
		MOD_STATE->clients[clientNum].snapCounter = 0;
	}
}

/*
==============
ModPCSmoothing_Static_RecordClientMove

Record client positions after each move fragment.
==============
*/
void ModPCSmoothing_Static_RecordClientMove( int clientNum ) {
	if ( MOD_STATE && ModPingcomp_Static_SmoothingEnabledForClient( clientNum ) ) {
		smoothing_client_t *modclient = &MOD_STATE->clients[clientNum];

		ModPCSmoothing_CheckTeleport( clientNum );

		modclient->lastMoveZ = level.clients[clientNum].ps.origin[2];

		if ( !modclient->snapCounter || CLIENT_SNAP( clientNum, modclient->snapCounter - 1 ).commandTime + MINIMUM_SNAP_LENGTH
				<= level.clients[clientNum].ps.commandTime ) {
			smoothing_client_snap_t *snap = &CLIENT_SNAP( clientNum, modclient->snapCounter );
			modclient->snapCounter++;
			ModPCSmoothing_ReadClientSnap( clientNum, snap );
		}
	}
}

/*
==============
ModPCSmoothing_LerpVector

Creates interpolation between 'base' and 'next'.
Proportion is indicated by 'fraction' on a scale from 0 (only base) to 1 (only next).
==============
*/
static void ModPCSmoothing_LerpVector( vec3_t base, vec3_t next, float fraction, vec3_t target ) {
	target[0] = base[0] + ( next[0] - base[0] ) * fraction;
	target[1] = base[1] + ( next[1] - base[1] ) * fraction;
	target[2] = base[2] + ( next[2] - base[2] ) * fraction;
}

/*
==============
ModPCSmoothing_RetrieveClientSnap

Retrieves client position as close as possible to specified command time, interpolating if possible.
Returns qtrue on success, qfalse on error.
==============
*/
static qboolean ModPCSmoothing_RetrieveClientSnap( int clientNum, int time, smoothing_client_snap_t *snap ) {
	smoothing_client_t *modclient = &MOD_STATE->clients[clientNum];
	int counter = modclient->snapCounter;
	if ( counter > 0 ) {
		int maxSnapIndex = counter - 1;
		int minSnapIndex = counter - MAX_CLIENT_SNAPS > 0 ? counter - MAX_CLIENT_SNAPS : 0;
		int currentSnapIndex = maxSnapIndex;
		smoothing_client_snap_t *current_snap = &CLIENT_SNAP( clientNum, currentSnapIndex );

		// try to find the snap at or directly before target time
		while ( currentSnapIndex > minSnapIndex && current_snap->commandTime > time ) {
			--currentSnapIndex;
			current_snap = &CLIENT_SNAP( clientNum, currentSnapIndex );
		}

		// make sure time is reasonable (can fail if smoothing is switched off and on)
		if ( current_snap->commandTime + 200 < time ) {
			return qfalse;
		}

		*snap = *current_snap;

		// see if we have a valid next snap to interpolate to
		if ( current_snap->commandTime < time && currentSnapIndex + 1 <= maxSnapIndex ) {
			smoothing_client_snap_t *next_snap = &CLIENT_SNAP( clientNum, currentSnapIndex + 1 );
			float lerpFraction;

			EF_WARN_ASSERT( next_snap->commandTime > time );

			lerpFraction = (float)( time - current_snap->commandTime ) /
					(float)( next_snap->commandTime - current_snap->commandTime );

			ModPCSmoothing_LerpVector( current_snap->origin, next_snap->origin, lerpFraction, snap->origin );
			snap->commandTime = time;
		}

		// if lift is causing z movement, patch the z origin to avoid floating above/below the lift
		if ( snap->liftCount != modclient->liftCount ) {
			snap->origin[2] = modclient->lastLiftZ;
		}

		return qtrue;
	}

	return qfalse;
}

/*
==============
ModPCSmoothing_Static_ShiftClient

Moves client's shared entity to "smoothed" position ahead of server snapshot.
Returns qtrue on success, qfalse on error or smoothing inactive.
On success, additional data will be written to info_out for collision handling purposes.
==============
*/
qboolean ModPCSmoothing_Static_ShiftClient( int clientNum, Smoothing_ShiftInfo_t *info_out ) {
	if ( MOD_STATE && ModPingcomp_Static_SmoothingEnabledForClient( clientNum ) ) {
		gclient_t *client = &level.clients[clientNum];
		smoothing_client_t *modclient = &MOD_STATE->clients[clientNum];
		smoothing_client_snap_t snap;
		int targetTime;

		// Perform some once-per-server-frame checks here.
		ModPCSmoothing_CheckTeleport( clientNum );
		ModPCSmoothing_CheckLiftMove( clientNum );

		// Process dead clients.
		if ( client->ps.pm_type == PM_DEAD ) {
			if ( modclient->deadOnMover ) {
				return qfalse;
			}

			// Try to retrieve dead move position
			if ( ModPCDeadMove_Static_ShiftClient( clientNum, info_out ) ) {
				return qtrue;
			}
		} else {
			modclient->deadOnMover = qfalse;
		}

		// Apply smoothed position to client entity.
		targetTime = level.time - ModPCSmoothingOffset_Shared_GetOffset( clientNum );
		if ( ModPCSmoothing_RetrieveClientSnap( clientNum, targetTime, &snap ) ) {
			ModPCSmoothing_ApplyClientSnap( clientNum, &snap );
			if ( info_out ) {
				VECTORCOPY_INT( snap.mins, info_out->mins );
				VECTORCOPY_INT( snap.maxs, info_out->maxs );
			}

			ModPCSmoothingDebug_Static_LogFrame( clientNum, targetTime, snap.commandTime );
			return qtrue;
		}
	}

	return qfalse;
}

/*
=============
(ModFN) CopyToBodyQue

Use smoothed position when creating body entity so it stays in the same visible position
as the player entity body.
=============
*/
static gentity_t *MOD_PREFIX(CopyToBodyQue)( MODFN_CTV, int clientNum ) {
	ModPCSmoothing_Static_ShiftClient( clientNum, NULL );
	return MODFN_NEXT( CopyToBodyQue, ( MODFN_NC, clientNum ) );
}

/*
================
(ModFN) PostPlayerDie
================
*/
static void MOD_PREFIX(PostPlayerDie)( MODFN_CTV, gentity_t *self, gentity_t *inflictor, gentity_t *attacker,
		int meansOfDeath, int *awardPoints ) {
	int clientNum = self - g_entities;
	gclient_t *client = &level.clients[clientNum];
	smoothing_client_t *modclient = &MOD_STATE->clients[clientNum];
	smoothing_client_snap_t snap;

	MODFN_NEXT( PostPlayerDie, ( MODFN_NC, self, inflictor, attacker, meansOfDeath, awardPoints ) );

	if ( ModPCSmoothing_RetrieveClientSnap( clientNum, level.time - ModPCSmoothingOffset_Shared_GetOffset( clientNum ), &snap ) ) {
		if ( snap.liftCount != modclient->liftCount ) {
			// If player died on a lift, disable time shifting until they respawn. This can cause the
			// player to visually snap to the playerstate position a bit, but is usually pretty minor
			// and works adequately for this special case.
			modclient->deadOnMover = qtrue;
		} else if ( meansOfDeath == MOD_TRIGGER_HURT && modfn_lcl.AdjustModConstant( MC_PINGCOMP_NO_TH_DEAD_MOVE, 0 ) ) {
			// If mod disables dead move for trigger hurt, keep using time-shifted position with no
			// move splitting.
		} else if ( VectorCompare( client->ps.velocity, vec3_origin ) ) {
			// If velocity was cleared by death effects also start the dead move with no velocity.
			ModPCDeadMove_Static_InitDeadMove( clientNum, snap.origin, vec3_origin );
		} else if ( MOD_STATE->currentDamage ) {
			// If killed through G_Damage, start the dead move with time-shifted velocity but add in
			// any knockback that was added during the hit.
			vec3_t knockback;
			vec3_t newVelocity;
			VectorSubtract( client->ps.velocity, MOD_STATE->currentDamageOldVelocity, knockback );
			VectorAdd( snap.velocity, knockback, newVelocity );
			ModPCDeadMove_Static_InitDeadMove( clientNum, snap.origin, newVelocity );
		} else {
			ModPCDeadMove_Static_InitDeadMove( clientNum, snap.origin, snap.velocity );
		}
	}
}

/*
================
(ModFN) Damage

Store current velocity ahead of damage so knockback can be calculated.
================
*/
static void MOD_PREFIX(Damage)( MODFN_CTV, gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
			vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	if ( targ->client ) {
		MOD_STATE->currentDamage = qtrue;
		VectorCopy( targ->client->ps.velocity, MOD_STATE->currentDamageOldVelocity );
	}

	MODFN_NEXT( Damage, ( MODFN_NC, targ, inflictor, attacker, dir, point, damage, dflags, mod ) );

	MOD_STATE->currentDamage = qfalse;
}

/*
================
ModPCSmoothing_Init
================
*/
void ModPCSmoothing_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( CopyToBodyQue, MODPRIORITY_GENERAL );
		MODFN_REGISTER( PostPlayerDie, MODPRIORITY_GENERAL );
		MODFN_REGISTER( Damage, MODPRIORITY_LOW );

		ModPlayerMove_Init();	// for ModPCSmoothing_Static_RecordClientMove callback
		ModPCSmoothingDebug_Init();
		ModPCSmoothingOffset_Init();
		ModPCProjectileImpact_Init();
		ModPCDeadMove_Init();
	}
}
