// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"


/*
=======================================================================

  SESSION DATA

Session data is the only data that stays persistant across level loads
and tournament restarts.
=======================================================================
*/

// Random identifier to prevent loading data from incompatible mods, in case
// server uses mod switching.
#define SESSION_VERSION "hszK3I2I"

extern int	borgQueenClientNum;
extern int	noJoinLimit;
extern int	numKilled;

/*
================
(ModFN) InitClientSession
================
*/
LOGFUNCTION_VOID( ModFNDefault_InitClientSession, ( int clientNum, qboolean initialConnect, const info_string_t *info ),
		( clientNum, initialConnect, info ), "G_MODFN_INITCLIENTSESSION" ) {
}

/*
================
(ModFN) GenerateClientSessionInfo
================
*/
LOGFUNCTION_VOID( ModFNDefault_GenerateClientSessionInfo, ( int clientNum, info_string_t *info ),
		( clientNum, info ), "G_MODFN_GENERATECLIENTSESSIONINFO" ) {
}

/*
================
G_VerifySessionVersion

Returns qtrue if session data was written by compatible mod.
================
*/
static qboolean G_VerifySessionVersion( void ) {
	char buffer[256];
	char *version;
	trap_Cvar_VariableStringBuffer( "session", buffer, sizeof( buffer ) );
	version = strchr( buffer, '*' );
	if ( version && !strcmp( &version[1], SESSION_VERSION ) ) {
		return qtrue;
	}

	return qfalse;
}

/*
================
G_WriteClientSessionData

Called on game shutdown
================
*/
void G_WriteClientSessionData( gclient_t *client ) {
	int clientNum = client - level.clients;
	const char	*s;
	const char	*var;
	info_string_t info;

	s = va("%i %i %i %i %i %i %i %i",
		client->sess.sessionTeam,
		client->sess.sessionClass,
		client->sess.spectatorTime,
		client->sess.spectatorState,
		client->sess.spectatorClient,
		client->sess.wins,
		client->sess.losses,
		client->sess.altSwapFlags
		);

	var = va( "session%i", clientNum );

	trap_Cvar_Set( var, s );

	// Write session info
	info.s[0] = '\0';
	modfn.GenerateClientSessionInfo( clientNum, &info );
	var = va( "session_info%i", clientNum );
	trap_Cvar_Set( var, info.s );
}

/*
==================
G_RetrieveSessionInfo

Retrieves client session info segment in session_info* cvars.

This should only be called for clients that are reconnecting with firstTime=false,
otherwise invalid info may be returned.
==================
*/
void G_RetrieveSessionInfo( int clientNum, info_string_t *output ) {
	output->s[0] = '\0';

	if ( G_VerifySessionVersion() ) {
		char buffer[256];
		Com_sprintf( buffer, sizeof( buffer ), "session_info%i", clientNum );
		trap_Cvar_VariableStringBuffer( buffer, output->s, sizeof( output->s ) );
	}
}

/*
================
G_ReadSessionData

Called on a reconnect
================
*/
void G_ReadSessionData( gclient_t *client ) {
	int clientNum = client - level.clients;
	char	s[MAX_STRING_CHARS];
	const char	*var;

	var = va( "session%i", clientNum );
	trap_Cvar_VariableStringBuffer( var, s, sizeof(s) );

	sscanf( s, "%i %i %i %i %i %i %i %i",
		&client->sess.sessionTeam,
		&client->sess.sessionClass,
		&client->sess.spectatorTime,
		&client->sess.spectatorState,
		&client->sess.spectatorClient,
		&client->sess.wins,
		&client->sess.losses,
		&client->sess.altSwapFlags
		);

	// Call mod initialization
	{
		info_string_t info;
		G_RetrieveSessionInfo( clientNum, &info );
		modfn.InitClientSession( clientNum, qfalse, &info );
	}
}


/*
================
G_InitSessionData

Called on a first-time connect
================
*/
void G_InitSessionData( gclient_t *client, char *userinfo ) {
	int clientNum = client - level.clients;
	clientSession_t	*sess;
	const char		*value;

	sess = &client->sess;

	// initial team determination
	if ( g_gametype.integer >= GT_TEAM )
	{//Team holomatch or CTF
		if ( g_teamAutoJoin.integer && !(g_team_group_red.string[0] || g_team_group_blue.string[0]) ) {
			if ( g_pModElimination.integer && noJoinLimit != 0 && ( level.time-level.startTime > noJoinLimit || numKilled > 0 ) )
			{//elim game already in progress
				sess->sessionTeam = TEAM_SPECTATOR;
			}
			else if ( g_pModAssimilation.integer && borgQueenClientNum != -1 && noJoinLimit != 0 && ( level.time-level.startTime > noJoinLimit || numKilled > 0 ) )
			{//assim game already in progress
				sess->sessionTeam = TEAM_SPECTATOR;
			}
			else
			{
				sess->sessionTeam = PickTeam( -1 );
				BroadcastTeamChange( client, -1 );
			}
		} else {
			// always spawn as spectator in team games
			sess->sessionTeam = TEAM_SPECTATOR;
		}
	}
	else
	{//FFA, tournament or sp
		value = Info_ValueForKey( userinfo, "team" );
		if ( value[0] == 's' ) {
			// a willing spectator, not a waiting-in-line
			sess->sessionTeam = TEAM_SPECTATOR;
		} else {
			switch ( g_gametype.integer ) {
			default:
			case GT_FFA:
			case GT_SINGLE_PLAYER:
				if ( g_maxGameClients.integer > 0 &&
					level.numNonSpectatorClients >= g_maxGameClients.integer ) {
					sess->sessionTeam = TEAM_SPECTATOR;
				} else {
					if ( g_pModElimination.integer && noJoinLimit != 0 && ( level.time-level.startTime > noJoinLimit || numKilled > 0 ) )
					{//game already in progress
						sess->sessionTeam = TEAM_SPECTATOR;
					}
					else
					{
						sess->sessionTeam = TEAM_FREE;
					}
				}
				break;
			case GT_TOURNAMENT:
				// if the game is full, go into a waiting mode
				if ( level.numNonSpectatorClients >= 2 ) {
					sess->sessionTeam = TEAM_SPECTATOR;
				} else {
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			}
		}
	}

	sess->sessionClass = PC_NOCLASS;
	sess->spectatorState = SPECTATOR_FREE;
	sess->spectatorTime = level.time;

	// Call mod initialization
	{
		info_string_t info;
		*info.s = '\0';
		modfn.InitClientSession( clientNum, qtrue, &info );
	}
}


/*
==================
G_InitWorldSession

==================
*/
void G_InitWorldSession( void ) {
	char	s[MAX_STRING_CHARS];
	int			gt;

	trap_Cvar_VariableStringBuffer( "session", s, sizeof(s) );
	gt = atoi( s );

	// if the gametype changed since the last session, don't use any
	// client sessions
	if ( g_gametype.integer != gt ) {
		level.newSession = qtrue;
		G_Printf( "Gametype changed, clearing session data.\n" );
	}
}

/*
==================
G_WriteSessionData

==================
*/
void G_WriteSessionData( void ) {
	int		i;

	trap_Cvar_Set( "session", va("%i*" SESSION_VERSION, g_gametype.integer) );

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected >= CON_CONNECTING ) {
			G_WriteClientSessionData( &level.clients[i] );
		}
	}
}
