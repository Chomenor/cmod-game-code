/*
* Click To Join Support
* 
* This module enables players to join the game from spectator by clicking the attack
* button, similar to Gladiator mod.
*/

#define MOD_NAME ModClickToJoin

#include "mods/g_mod_local.h"

static struct {
	int _unused;
} *MOD_STATE;

/*
================
ModClickToJoin_Static_ActiveForClient

Returns whether client is currently in click to join mode.
================
*/
qboolean ModClickToJoin_Static_ActiveForClient( int clientNum ) {
	gclient_t *client = &level.clients[clientNum];
	return MOD_STATE && level.matchState <= MS_WARMUP && client->sess.sessionTeam == TEAM_SPECTATOR;
}

/*
==============
(ModFN) RunPlayerMove
==============
*/
LOGFUNCTION_SVOID( MOD_PREFIX(RunPlayerMove), ( MODFN_CTV, int clientNum ), ( MODFN_CTN, clientNum ), "G_MODFN_RUNPLAYERMOVE" ) {
	gclient_t *client = &level.clients[clientNum];
	static qboolean recursive = qfalse;

	MODFN_NEXT( RunPlayerMove, ( MODFN_NC, clientNum ) );

	if ( ModClickToJoin_Static_ActiveForClient( clientNum ) && EF_WARN_ASSERT( !recursive ) &&
			!( client->pers.oldbuttons & BUTTON_ATTACK ) && ( client->pers.cmd.buttons & BUTTON_ATTACK ) ) {
		recursive = qtrue;
		SetTeam( &g_entities[clientNum], "auto" );
		recursive = qfalse;
	}
}

/*
================
(ModFN) SpawnCenterPrintMessage

Show info message when click to join is available.
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(SpawnCenterPrintMessage), ( MODFN_CTV, int clientNum, clientSpawnType_t spawnType ),
		( MODFN_CTN, clientNum, spawnType ), "G_MODFN_SPAWNCENTERPRINTMESSAGE" ) {
	gclient_t *client = &level.clients[clientNum];

	if ( ModClickToJoin_Static_ActiveForClient( clientNum ) ) {
		trap_SendServerCommand( clientNum, "cp \"\n\n\n\n\n\n^1Hit ^7fire button ^1to join!\"" );
	}

	else {
		MODFN_NEXT( SpawnCenterPrintMessage, ( MODFN_NC, clientNum, spawnType ) );
	}
}

/*
================
ModClickToJoin_Init
================
*/
LOGFUNCTION_VOID( ModClickToJoin_Init, ( void ), (), "G_MOD_INIT G_MOD_CLICKJOIN" ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( RunPlayerMove, MODPRIORITY_GENERAL );

		// high priority to override any other messages
		MODFN_REGISTER( SpawnCenterPrintMessage, MODPRIORITY_HIGH );
	}
}
