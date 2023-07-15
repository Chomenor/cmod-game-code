/*
* Game Info
* 
* This module shares some non-essential information about the state of the game
* with the engine for engine-based features to access.
* 
* In particular, in Elimination mode, the engine needs to get the score values for
* status queries from this module, because the normal playerstate score is typically
* overwritten as part of a workaround to display the round numbers in the client HUD.
*/

#define MOD_NAME ModGameInfo

#include "mods/g_mod_local.h"

#define CLIENT_INFO_LENGTH 96

static struct {
	char currentInfo[MAX_CLIENTS][CLIENT_INFO_LENGTH];
} *MOD_STATE;

/*
================
ModGameInfo_TeamString
================
*/
static const char *ModGameInfo_TeamString( team_t team ) {
	switch ( team ) {
		case TEAM_RED:
			return "red";
		case TEAM_BLUE:
			return "blue";
		case TEAM_SPECTATOR:
			return "spectator";
		default:
			return "free";
	}
}

/*
================
ModGameInfo_UpdateClients

Generate info string for each client, and update engine if different from last updated version.
================
*/
static void ModGameInfo_UpdateClients( void ) {
	info_string_t info;
	int clientNum;

	for ( clientNum = 0; clientNum < level.maxclients; ++clientNum ) {
		const gclient_t *client = &level.clients[clientNum];
		Com_sprintf( info.s, sizeof( info.s ), "\\connected\\%i\\spawned\\%i",
				client->pers.connected >= CON_CONNECTING ? 1 : 0,
				client->pers.connected >= CON_CONNECTED ? 1 : 0 );

		if ( client->pers.connected >= CON_CONNECTING ) {
			Info_SetValueForKey_Big( info.s, "score", va( "%i", modfn.EffectiveScore( clientNum, EST_STATUS_QUERY ) ) );
			Info_SetValueForKey_Big( info.s, "team", ModGameInfo_TeamString( modfn.RealSessionTeam( clientNum ) ) );
			modfn_lcl.AddGameInfoClient( clientNum, &info );
		}

		if ( Q_stricmp( MOD_STATE->currentInfo[clientNum], info.s ) ) {
			// Send the info string to the engine via the getvalue system.
			VMExt_GVCommandInt( va( "gameinfo \\type\\clientstate\\client\\%i%s", clientNum, info.s ), 0 );
			Q_strncpyz( MOD_STATE->currentInfo[clientNum], info.s, sizeof( MOD_STATE->currentInfo[clientNum] ) );
		}
	}
}

/*
================
(ModFN) PostCalculateRanks
================
*/
static void MOD_PREFIX(PostCalculateRanks)( MODFN_CTV ) {
	MODFN_NEXT( PostCalculateRanks, ( MODFN_NC ) );

	ModGameInfo_UpdateClients();
}

/*
================
ModGameInfo_Init
================
*/
void ModGameInfo_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( PostCalculateRanks, MODPRIORITY_GENERAL );
	}
}
