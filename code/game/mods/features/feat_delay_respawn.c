/*
* Delayed Respawn
* 
* Adds support for g_delayRespawn cvar, which sets a fixed delay of a certain number of
* seconds before players respawn. Countdown is displayed on client with cgame support.
*/

#define MOD_NAME ModDelayRespawn

#include "mods/g_mod_local.h"

static struct {
	trackedCvar_t g_delayRespawn;	// Number of seconds until respawn (0 = disabled)
	trackedCvar_t g_delayRespawnSpawnOnFlagCapture;		// Respawn immediately if enemy team captures

	// Value set in PostPlayerDie and accessed in CheckRespawnTime.
	// Note that stale values may remain in case of disconnect or team switch, but
	// it shouldn't matter because CheckRespawnTime is not called in these cases.
	int pendingRespawn[MAX_CLIENTS];
} *MOD_STATE;

#define MOD_ENABLED ( MOD_STATE->g_delayRespawn.value > 0.0f )

/*
==============
(ModFN) AddModConfigInfo
==============
*/
static void MOD_PREFIX(AddModConfigInfo)( MODFN_CTV, char *info ) {
	MODFN_NEXT( AddModConfigInfo, ( MODFN_NC, info ) );
	if ( MOD_ENABLED ) {
		Info_SetValueForKey( info, "respawnTimerStatIndex", "10" );
	}
}

/*
=============
ModDelayRespawn_OnCvarUpdate
=============
*/
static void ModDelayRespawn_OnCvarUpdate( trackedCvar_t *cvar ) {
	ModModcfgCS_Static_Update();
}

/*
=============
ModDelayRespawn_UnlinkBody
=============
*/
static void ModDelayRespawn_UnlinkBody( gentity_t *ent ) {
	trap_UnlinkEntity( ent );
	ent->physicsObject = qfalse;
}

/*
=============
(ModFN) CopyToBodyQue

Have the body fade immediately after respawn, since we've already been dead awhile.
=============
*/
static gentity_t *MOD_PREFIX(CopyToBodyQue)( MODFN_CTV, int clientNum ) {
	gentity_t *body = MODFN_NEXT( CopyToBodyQue, ( MODFN_NC, clientNum ) );
	if ( MOD_ENABLED ) {
		body->think = ModDelayRespawn_UnlinkBody;
		body->nextthink = level.time + 2500;
		body->s.time = level.time + 2500;
		G_AddEvent( body, EV_GENERAL_SOUND, G_SoundIndex( "sound/enemies/borg/walkthroughfield.wav" ) );
	}
	return body;
}

/*
================
(ModFN) PostPlayerDie

Send command to clients to display respawn countdown.
================
*/
static void MOD_PREFIX(PostPlayerDie)( MODFN_CTV, gentity_t *self, gentity_t *inflictor, gentity_t *attacker,
		int meansOfDeath, int *awardPoints ) {
	int clientNum = self - g_entities;
	playerState_t *ps = &level.clients[clientNum].ps;
	int *pendingRespawn = &MOD_STATE->pendingRespawn[clientNum];

	MODFN_NEXT( PostPlayerDie, ( MODFN_NC, self, inflictor, attacker, meansOfDeath, awardPoints ) );

	if ( MOD_ENABLED ) {
		*pendingRespawn = level.time + MOD_STATE->g_delayRespawn.value * 1000;
	} else {
		*pendingRespawn = 0;
	}

	// Encode respawn time in playerstate for cgame to access. Needs to be split across
	// 2 fields because net protocol (msg.c) only sends 16 bits for playerstate stats.
	ps->stats[10] = ( *pendingRespawn >> 16 ) & 0xffff;
	ps->stats[11] = *pendingRespawn & 0xffff;
}

/*
==============
(ModFN) CheckRespawnTime
==============
*/
static qboolean MOD_PREFIX(CheckRespawnTime)( MODFN_CTV, int clientNum, qboolean voluntary ) {
	int *pendingRespawn = &MOD_STATE->pendingRespawn[clientNum];

	if ( MOD_ENABLED && *pendingRespawn ) {
		if ( !voluntary && level.time >= *pendingRespawn ) {
			*pendingRespawn = 0;
			return qtrue;
		}

		return qfalse;
	} else {
		return MODFN_NEXT( CheckRespawnTime, ( MODFN_NC, clientNum, voluntary ) );
	}
}

/*
================
(ModFN) PostFlagCapture

If g_delayRespawnSpawnOnFlagCapture is enabled, when a certain team captures the flag,
respawn any dead players on the other team.
================
*/
static void MOD_PREFIX(PostFlagCapture)( MODFN_CTV, team_t capturingTeam ) {
	MODFN_NEXT( PostFlagCapture, ( MODFN_NC, capturingTeam ) );

	if ( MOD_STATE->g_delayRespawnSpawnOnFlagCapture.integer ) {
		int i;

		for ( i = 0; i < level.maxclients; ++i ) {
			if ( level.clients[i].pers.connected == CON_CONNECTED && level.clients[i].sess.sessionTeam != capturingTeam &&
					!modfn.SpectatorClient( i ) && level.clients[i].ps.stats[STAT_HEALTH] <= 0 ) {
				modfn.ClientRespawn( i );
			}
		}
	}
}

/*
================
ModDelayRespawn_Init
================
*/
void ModDelayRespawn_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		G_RegisterTrackedCvar( &MOD_STATE->g_delayRespawn, "g_delayRespawn", "0", 0, qfalse );
		G_RegisterCvarCallback( &MOD_STATE->g_delayRespawn, ModDelayRespawn_OnCvarUpdate, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_delayRespawnSpawnOnFlagCapture, "g_delayRespawnSpawnOnFlagCapture", "1", 0, qfalse );

		ModModcfgCS_Init();

		MODFN_REGISTER( AddModConfigInfo, MODPRIORITY_GENERAL );
		MODFN_REGISTER( CopyToBodyQue, MODPRIORITY_GENERAL );
		MODFN_REGISTER( PostPlayerDie, MODPRIORITY_GENERAL );
		MODFN_REGISTER( CheckRespawnTime, MODPRIORITY_GENERAL );
		MODFN_REGISTER( PostFlagCapture, MODPRIORITY_GENERAL );
	}
}
