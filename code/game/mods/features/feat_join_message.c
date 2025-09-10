/*
* Join Message handling
* 
* Supports some tweaks and options to customize "entered the game" and "joined the..."
* messages when players enter the game. The regular prints are disabled and reimplemented
* in this module to make the logic clearer and avoid cluttering the main code.
*/

#define MOD_NAME ModJoinMessage

#include "mods/g_mod_local.h"

static struct {
	char oldTeam[MAX_CLIENTS];

	// print team join messages to console instead of center print (-1 = default)
	trackedCvar_t g_joinMessageConsolePrint;

	// skip join messages during map restart (-1 = default)
	trackedCvar_t g_joinMessageSkipRestart;
} *MOD_STATE;

/*
================
ModJoinMessage_BroadcastTeamChange
================
*/
static void ModJoinMessage_BroadcastTeamChange( gclient_t *client, qboolean consolePrint )
{
	const char *cmd = consolePrint ? "print" : "cp";
	char name[64];

	Q_strncpyz( name, client->pers.netname, sizeof( name ) );
	if ( !consolePrint ) {
		// Original version limited to 15 chars
		// Avoids screen overflow in case of long name containing @ characters...
		name[15] = '\0';
	}

	if ( client->sess.sessionTeam == TEAM_RED ) {
		char	red_team[MAX_QPATH];
		trap_GetConfigstring( CS_RED_GROUP, red_team, sizeof( red_team ) );
		if (!red_team[0])	{
			Q_strncpyz( red_team, "red team", sizeof( red_team ) );
		}
		trap_SendServerCommand( -1, va("%s \"%s" S_COLOR_WHITE " joined the %s.\n\"", cmd, name, red_team ) );
	} else if ( client->sess.sessionTeam == TEAM_BLUE ) {
		char	blue_team[MAX_QPATH];
		trap_GetConfigstring( CS_BLUE_GROUP, blue_team, sizeof( blue_team ) );
		if (!blue_team[0]) {
			Q_strncpyz( blue_team, "blue team", sizeof( blue_team ) );
		}
		trap_SendServerCommand( -1, va("%s \"%s" S_COLOR_WHITE " joined the %s.\n\"", cmd, name, blue_team ) );
	} else if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( -1, va("%s \"%s" S_COLOR_WHITE " joined the spectators.\n\"", cmd, name));
	} else if ( client->sess.sessionTeam == TEAM_FREE ) {
		trap_SendServerCommand( -1, va("%s \"%s" S_COLOR_WHITE " joined the battle.\n\"", cmd, name));
	}
}

/*
================
(ModFN) SpawnEnterGameAnnounce
================
*/
static void MOD_PREFIX(SpawnEnterGameAnnounce)( MODFN_CTV, int clientNum, clientSpawnType_t spawnType ) {
	gclient_t *client = &level.clients[clientNum];

	qboolean initialTeamMsg = ( spawnType == CST_FIRSTTIME || spawnType == CST_MAPCHANGE || spawnType == CST_MAPRESTART )
			&& g_gametype.integer >= GT_TEAM && client->sess.sessionTeam != TEAM_SPECTATOR;
	qboolean changeTeamMsg = spawnType == CST_TEAMCHANGE && !( client->sess.sessionTeam == TEAM_SPECTATOR &&
			MOD_STATE->oldTeam[clientNum] == TEAM_SPECTATOR );
	qboolean printEnterGame = spawnType != CST_RESPAWN && client->sess.sessionTeam != TEAM_SPECTATOR &&
			!g_holoIntro.integer && !modfn_lcl.AdjustModConstant( MC_SKIP_ENTER_GAME_PRINT, 0 );

	qboolean useConsolePrint = MOD_STATE->g_joinMessageConsolePrint.integer >= 0 ?
			MOD_STATE->g_joinMessageConsolePrint.integer : modfn_lcl.AdjustModConstant( MC_JOIN_MESSAGE_CONSOLE_PRINT, 0 );
	qboolean skipOnRestart = MOD_STATE->g_joinMessageSkipRestart.integer >= 0 ?
			MOD_STATE->g_joinMessageSkipRestart.integer : modfn_lcl.AdjustModConstant( MC_JOIN_MESSAGE_SKIP_RESTART, 0 );

	if ( skipOnRestart && spawnType == CST_MAPRESTART ) {
		return;
	}

	if ( initialTeamMsg || changeTeamMsg ) {
		ModJoinMessage_BroadcastTeamChange( client, useConsolePrint );

		if ( useConsolePrint ) {
			// no need for both console join message and entered game message at the same time
			printEnterGame = qfalse;
		}
	}

	if ( printEnterGame ) {
		trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " entered the game\n\"", client->pers.netname) );
	}
}

/*
================
(ModFN) PostClientSpawn

Set oldTeam.
================
*/
static void MOD_PREFIX(PostClientSpawn)( MODFN_CTV, int clientNum, clientSpawnType_t spawnType ) {
	gclient_t *client = &level.clients[clientNum];
	MOD_STATE->oldTeam[clientNum] = client->sess.sessionTeam;
	MODFN_NEXT( PostClientSpawn, ( MODFN_NC, clientNum, spawnType ) );
}

/*
================
(ModFN) PostClientConnect

Reset oldTeam.
================
*/
static void MOD_PREFIX(PostClientConnect)( MODFN_CTV, int clientNum, qboolean firstTime, qboolean isBot ) {
	MOD_STATE->oldTeam[clientNum] = -1;
	MODFN_NEXT( PostClientConnect, ( MODFN_NC, clientNum, firstTime, isBot ) );
}

/*
================
(ModFN) AdjustGeneralConstant
================
*/
static int MOD_PREFIX(AdjustGeneralConstant)( MODFN_CTV, generalConstant_t gcType, int defaultValue ) {
	if ( gcType == GC_SKIP_JOIN_MESSAGES ) {
		// Disable regular messages so they can be handled here separately.
		return 1;
	}

	return MODFN_NEXT( AdjustGeneralConstant, ( MODFN_NC, gcType, defaultValue ) );
}

/*
================
ModJoinMessage_Init
================
*/
void ModJoinMessage_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( SpawnEnterGameAnnounce, MODPRIORITY_GENERAL );
		MODFN_REGISTER( PostClientSpawn, MODPRIORITY_GENERAL );
		MODFN_REGISTER( PostClientConnect, MODPRIORITY_GENERAL );
		MODFN_REGISTER( AdjustGeneralConstant, MODPRIORITY_GENERAL );

		G_RegisterTrackedCvar( &MOD_STATE->g_joinMessageConsolePrint, "g_joinMessageConsolePrint", "-1", 0, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_joinMessageSkipRestart, "g_joinMessageSkipRestart", "-1", 0, qfalse );
	}
}
