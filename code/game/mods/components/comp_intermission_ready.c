/*
* Intermission Ready Handling
* 
* This module provides some extra configuration options for mods to customize the
* exit conditions and display parameters of the end-of-match ready system.
*/

#define MOD_NAME ModIntermissionReady

#include "mods/g_mod_local.h"

static struct {
	modIntermissionReady_config_t config;

	qboolean playSoundWhenReady[MAX_CLIENTS];
	qboolean suspended;
} *MOD_STATE;

typedef enum {
	PRT_UNCOUNTED,
	PRT_READY,
	PRT_NOTREADY,
} playerReadyType_t;

typedef struct {
	playerReadyType_t readyType;
	qboolean indicateReady;
	qboolean enableSounds;
} playerReadyState_t;

/*
================
ModIntermissionReady_Shared_UpdateConfig

Calls shared config function to generate configuration.
================
*/
void ModIntermissionReady_Shared_UpdateConfig( void ) {
	EF_ERR_ASSERT( MOD_STATE );
	memset( &MOD_STATE->config, 0, sizeof( MOD_STATE->config ) );
	modfn_lcl.IntermissionReadyConfig( &MOD_STATE->config );
}

/*
=================
ModIntermissionReady_Shared_Suspend

Suspends intermission and blocks any exits or indicator updates. Used for elimination final scores.
=================
*/
void ModIntermissionReady_Shared_Suspend( void ) {
	EF_ERR_ASSERT( MOD_STATE );
	MOD_STATE->suspended = qtrue;
}

/*
=================
ModIntermissionReady_Shared_Resume

Resumes suspended intermission and resets start time.
=================
*/
void ModIntermissionReady_Shared_Resume( void ) {
	int i;
	EF_ERR_ASSERT( MOD_STATE );
	MOD_STATE->suspended = qfalse;
	level.intermissiontime = level.time;

	// Reset ready states
	for ( i = 0; i < level.maxclients; ++i ) {
		level.clients[i].readyToExit = qfalse;
	}
}

/*
================
(ModFN) IntermissionReadyConfig
================
*/
void ModFNDefault_IntermissionReadyConfig( modIntermissionReady_config_t *config ) {
	config->minExitTime = 5000;
	config->sustainedAnyReadyTime = g_intermissionTime.integer * 1000;
}

/*
================
ModIntermissionReady_CvarChanged
================
*/
static void ModIntermissionReady_CvarChanged( trackedCvar_t *cv ) {
	ModIntermissionReady_Shared_UpdateConfig();
}

/*
=================
GetPlayerReadyState

Returns ready state for each client, specifically:
1) Whether player is actually ready to go, not ready, or should be uncounted.
2) Whether ready indicator on scoreboard should be displayed for this client.
3) Whether beep sounds are enabled for when player becomes ready.
	 (should be enabled when ready state is set by player, not automatically)
=================
*/

#define RETURN_STATE( type, indicateReady, enableSounds ) \
	{ playerReadyState_t state = { type, indicateReady, enableSounds }; return state; }

static playerReadyState_t GetPlayerReadyState( int clientNum ) {
	gentity_t *ent = &g_entities[clientNum];
	gclient_t *client = &level.clients[clientNum];

	if ( MOD_STATE->suspended ||
			level.time < level.intermissiontime + MOD_STATE->config.minExitTime ||
			client->pers.connected < CON_CONNECTED )
		RETURN_STATE( PRT_UNCOUNTED, qfalse, qfalse );

	if ( ent->r.svFlags & SVF_BOT )
		RETURN_STATE( PRT_UNCOUNTED, qtrue, qfalse );

	if ( MOD_STATE->config.ignoreSpectators && client->sess.sessionTeam == TEAM_SPECTATOR )
		RETURN_STATE( PRT_UNCOUNTED, qtrue, qfalse );

	if ( client->readyToExit )
		RETURN_STATE( PRT_READY, qtrue, qtrue );

	RETURN_STATE( PRT_NOTREADY, qfalse, qtrue );
}

/*
=================
(ModFN) IntermissionReadyIndicator

Determines whether to display green 'ready' indicator next to player's name during intermission.
=================
*/
static qboolean MOD_PREFIX(IntermissionReadyIndicator)( MODFN_CTV, int clientNum ) {
	return GetPlayerReadyState( clientNum ).indicateReady;
}

/*
=================
(ModFN) IntermissionReadyToExit

Determine whether it is time to exit intermission.
=================
*/
static qboolean MOD_PREFIX(IntermissionReadyToExit)( MODFN_CTV ) {
	int i;
	int ready = 0;
	int notReady = 0;

	// never exit in less than min time
	if ( MOD_STATE->suspended ||
			level.time < level.intermissiontime + MOD_STATE->config.minExitTime ) {
		return qfalse;
	}

	for ( i = 0; i < level.maxclients; ++i ) {
		playerReadyState_t state = GetPlayerReadyState( i );
		if ( state.readyType == PRT_READY ) {
			++ready;
		}
		if ( state.readyType == PRT_NOTREADY ) {
			++notReady;
		}
	}

	// exit due to no valid players
	if ( MOD_STATE->config.noPlayersExit && !ready && !notReady ) {
		G_DedPrintf( "intermission: Exit due to no valid players.\n" );
		return qtrue;
	}

	// exit due to max time
	if ( MOD_STATE->config.maxExitTime && level.time >= level.intermissiontime + MOD_STATE->config.maxExitTime ) {
		G_DedPrintf( "intermission: Exit due to max time (%i).\n", MOD_STATE->config.maxExitTime );
		return qtrue;
	}

	// exit due to somebody ready
	if ( MOD_STATE->config.anyReadyTime && ready && level.time >= level.intermissiontime + MOD_STATE->config.anyReadyTime ) {
		G_DedPrintf( "intermission: Exit due to any player ready (%i).\n", MOD_STATE->config.anyReadyTime );
		return qtrue;
	}


	// if nobody wants to go, clear timer
	if ( !ready ) {
		level.readyToExitTime = 0;
		return qfalse;
	}

	// if everyone wants to go, go now
	if ( !notReady ) {
		G_DedPrintf( "intermission: Exit due to all valid players ready.\n" );
		return qtrue;
	}

	if ( MOD_STATE->config.sustainedAnyReadyTime ) {
		// the first person ready starts timer
		if ( !level.readyToExitTime ) {
			G_DedPrintf( "intermission: Starting countdown due to 1+ players ready (%i).\n",
					MOD_STATE->config.sustainedAnyReadyTime );
			level.readyToExitTime = level.time;
		}

		// if we reach exit time without it being reset, go ahead
		if ( level.time >= level.readyToExitTime + MOD_STATE->config.sustainedAnyReadyTime ) {
			G_DedPrintf( "intermission: Exit due to continuous ready.\n" );
			return qtrue;
		}
	}

	return qfalse;
}

/*
================
(ModFN) PostRunFrame
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );

	if ( level.intermissiontime ) {
		int i;
		qboolean playSound = qfalse;

		for ( i = 0; i < level.maxclients; ++i ) {
			playerReadyState_t playerReadyState = GetPlayerReadyState( i );

			// determine whether to play sound now
			if ( MOD_STATE->config.readySound && playerReadyState.enableSounds && playerReadyState.indicateReady &&
					MOD_STATE->playSoundWhenReady[i] ) {
				playSound = qtrue;
			}

			// determine whether to play sound next time player becomes ready
			if ( playerReadyState.enableSounds && !playerReadyState.indicateReady ) {
				MOD_STATE->playSoundWhenReady[i] = qtrue;
			} else {
				MOD_STATE->playSoundWhenReady[i] = qfalse;
			}
		}

		if ( playSound ) {
			G_GlobalSound( G_SoundIndex( "sound/movers/switches/voyviewscreen.wav" ) );
		}
	}
}

/*
================
(ModFN) GeneralInit
================
*/
static void MOD_PREFIX(GeneralInit)( MODFN_CTV ) {
	MODFN_NEXT( GeneralInit, ( MODFN_NC ) );

	// Update config after cvar initialization is complete, for access to g_intermissionTime.
	ModIntermissionReady_Shared_UpdateConfig();
	G_RegisterCvarCallback( &g_intermissionTime, ModIntermissionReady_CvarChanged, qfalse );
}

/*
================
ModIntermissionReady_Init
================
*/
void ModIntermissionReady_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( IntermissionReadyIndicator, MODPRIORITY_GENERAL );
		MODFN_REGISTER( IntermissionReadyToExit, MODPRIORITY_GENERAL );
		MODFN_REGISTER( PostRunFrame, MODPRIORITY_GENERAL );
		MODFN_REGISTER( GeneralInit, MODPRIORITY_GENERAL );
	}
}
