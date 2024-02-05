/*
* Elimination Spectator Chat
* 
* This module implements the g_mod_ProtectedChat setting from Gladiator mod, which
* prevents active players and spectators from receiving each others chats.
*/

#define MOD_NAME ModElimSpectatorChat

#include "mods/modes/elimination/elim_local.h"

static struct {
	trackedCvar_t g_mod_ProtectedChat;
	int allowChatTime[MAX_CLIENTS];
} *MOD_STATE;

typedef enum {
	CHAT_SAY,
	CHAT_TEAM,
	CHAT_TELL
} chatType_t;

// Should probably match g_cmds.c
#define MAX_SAY_TEXT 150

#define IS_SPECTATOR( clientNum ) ( ModElimination_Static_IsPlayerEliminated( ( clientNum ) ) || \
		level.clients[clientNum].sess.sessionTeam == TEAM_SPECTATOR ? qtrue : qfalse )

/*
================
ModElimSpectatorChat_Static_IsEnabled

Returns whether protected chat is enabled, suitable for info message purposes.
================
*/
qboolean ModElimSpectatorChat_Static_IsEnabled( void ) {
	return MOD_STATE && G_ModUtils_ReadGladiatorBoolean( MOD_STATE->g_mod_ProtectedChat.string ) ? qtrue : qfalse;
}

/*
================
ModElimSpectatorChat_HandleChat

Returns qtrue to suppress normal handling of chat command.
================
*/
static qboolean ModElimSpectatorChat_HandleChat( int clientNum, chatType_t type ) {
	gclient_t *client = &level.clients[clientNum];
	qboolean isSpectator = IS_SPECTATOR( clientNum );
	if ( !ModElimSpectatorChat_Static_IsEnabled() || level.matchState != MS_ACTIVE ||
			client->pers.connected != CON_CONNECTED ) {
		return qfalse;
	}

	if ( type == CHAT_TEAM && g_gametype.integer < GT_TEAM ) {
		type = CHAT_SAY;
	}

	if ( type == CHAT_SAY ) {
		// Allow 1 unfiltered message following kills.
		int *allowChatTime = &MOD_STATE->allowChatTime[clientNum];
		if ( *allowChatTime >= level.time ) {
			*allowChatTime = 0;
			return qfalse;
		}
	}

	if ( type == CHAT_SAY || type == CHAT_TEAM ) {
		int i;
		char chat[MAX_SAY_TEXT];
		char msg[1024];

		Q_strncpyz( chat, ConcatArgs( 1 ), sizeof( chat ) );
		if ( type == CHAT_TEAM ) {
			char location[64];
			if ( Team_GetLocationMsg( &g_entities[clientNum], location, sizeof( location ) ) ) {
				Com_sprintf( msg, sizeof( msg ), "tchat \"(%s^7) (%s): ^5%s\"", client->pers.netname, location, chat );
			} else {
				Com_sprintf( msg, sizeof( msg ), "tchat \"(%s^7): ^5%s\"", client->pers.netname, chat );
			}
		} else if ( isSpectator ) {
			Com_sprintf( msg, sizeof( msg ), "chat \"(Spect) %s^7: %s\"", client->pers.netname, chat );
		} else {
			Com_sprintf( msg, sizeof( msg ), "chat \"%s^7: ^2%s\"", client->pers.netname, chat );
		}

		for ( i = 0; i < level.maxclients; ++i ) {
			gclient_t *tgtClient = &level.clients[i];
			if ( tgtClient->pers.connected == CON_CONNECTED && IS_SPECTATOR( i ) == isSpectator &&
					!( type == CHAT_TEAM && client->sess.sessionTeam != tgtClient->sess.sessionTeam ) &&
					!( g_gametype.integer == GT_TOURNAMENT && tgtClient->sess.sessionTeam == TEAM_FREE &&
						client->sess.sessionTeam != TEAM_FREE ) ) {
				trap_SendServerCommand( i, msg );
			}
		}
		return qtrue;
	}

	if ( type == CHAT_TELL ) {
		int targetNum;
		char arg[MAX_TOKEN_CHARS];
		trap_Argv( 1, arg, sizeof( arg ) );
		targetNum = atoi( arg );
		if ( targetNum >= 0 && targetNum < level.maxclients &&
				level.clients[targetNum].pers.connected != CON_DISCONNECTED &&
				IS_SPECTATOR( targetNum ) != isSpectator ) {
			return qtrue;
		}
	}

	return qfalse;
}

/*
================
(ModFN) ModClientCommand
================
*/
static qboolean MOD_PREFIX( ModClientCommand )( MODFN_CTV, int clientNum, const char *cmd ) {
	if ( !Q_stricmp( cmd, "say" ) && ModElimSpectatorChat_HandleChat( clientNum, CHAT_SAY ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( cmd, "say_team" ) && ModElimSpectatorChat_HandleChat( clientNum, CHAT_TEAM ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( cmd, "tell" ) && ModElimSpectatorChat_HandleChat( clientNum, CHAT_TELL ) ) {
		return qtrue;
	}

	return MODFN_NEXT( ModClientCommand, ( MODFN_NC, clientNum, cmd ) );
}

/*
================
ModElimSpectatorChat_RegisterAllowWindow
================
*/
static void ModElimSpectatorChat_RegisterAllowWindow( gentity_t *ent ) {
	if ( ent ) {
		int clientNum = ent - g_entities;
		if ( clientNum >= 0 && clientNum < level.maxclients ) {
			MOD_STATE->allowChatTime[clientNum] = level.time + 15000;
		}
	}
}

/*
================
(ModFN) PostPlayerDie

When player dies, grant both them and their attacker a 15 second window in which they
can send 1 unfiltered chat message visible to everybody.
================
*/
static void MOD_PREFIX( PostPlayerDie )( MODFN_CTV, gentity_t *self, gentity_t *inflictor, gentity_t *attacker,
		int meansOfDeath, int *awardPoints ) {
	MODFN_NEXT( PostPlayerDie, ( MODFN_NC, self, inflictor, attacker, meansOfDeath, awardPoints ) );
	ModElimSpectatorChat_RegisterAllowWindow( self );
	ModElimSpectatorChat_RegisterAllowWindow( attacker );
}

/*
================
(ModFN) InitClientSession
================
*/
static void MOD_PREFIX(InitClientSession)( MODFN_CTV, int clientNum, qboolean initialConnect, const info_string_t *info ) {
	MODFN_NEXT( InitClientSession, ( MODFN_NC, clientNum, initialConnect, info ) );
	MOD_STATE->allowChatTime[clientNum] = 0;
}

/*
================
ModElimSpectatorChat_Init
================
*/
void ModElimSpectatorChat_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		G_RegisterTrackedCvar( &MOD_STATE->g_mod_ProtectedChat, "g_mod_ProtectedChat", "0", 0, qfalse );

		MODFN_REGISTER( ModClientCommand, ++modePriorityLevel );
		MODFN_REGISTER( PostPlayerDie, ++modePriorityLevel );
		MODFN_REGISTER( InitClientSession, ++modePriorityLevel );
	}
}
