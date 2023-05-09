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

/*
================
(ModFN) InitClientSession
================
*/
void ModFNDefault_InitClientSession( int clientNum, qboolean initialConnect, const info_string_t *info ) {
}

/*
================
(ModFN) GenerateClientSessionInfo
================
*/
void ModFNDefault_GenerateClientSessionInfo( int clientNum, info_string_t *info ) {
}

/*
================
(ModFN) GenerateClientSessionStructure
================
*/
void ModFNDefault_GenerateClientSessionStructure( int clientNum, clientSession_t *sess ) {
	gclient_t *client = &level.clients[clientNum];

	sess->sessionTeam = modfn.RealSessionTeam( clientNum );
	sess->spectatorState = client->sess.spectatorState;
	sess->spectatorClient = client->sess.spectatorClient;
}

/*
================
(ModFN) GenerateGlobalSessionInfo
================
*/
void ModFNDefault_GenerateGlobalSessionInfo( info_string_t *info ) {
	Info_SetValueForKey_Big( info->s, "numConnected", va( "%i", level.numConnectedClients ) );
	Info_SetValueForKey_Big( info->s, "warmupRestarting", level.warmupRestarting ? "1" : "0" );
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
==================
G_RetrieveClientSessionInfo

Retrieves client info segment in session_info* cvars.

This should only be called for clients that are reconnecting with firstTime=false,
otherwise invalid info may be returned.
==================
*/
static void G_RetrieveClientSessionInfo( int clientNum, info_string_t *output ) {
	output->s[0] = '\0';
	if ( G_VerifySessionVersion() ) {
		char buffer[256];
		Com_sprintf( buffer, sizeof( buffer ), "session_info%i", clientNum );
		trap_Cvar_VariableStringBuffer( buffer, output->s, sizeof( output->s ) );
	}
}

/*
==================
G_RetrieveGlobalSessionInfo

Retrieves match info segment in session_info cvar.
==================
*/
void G_RetrieveGlobalSessionInfo( info_string_t *output ) {
	output->s[0] = '\0';
	if ( G_VerifySessionVersion() ) {
		trap_Cvar_VariableStringBuffer( "session_info", output->s, sizeof( output->s ) );
	}
}

/*
==================
G_RetrieveGlobalSessionValue

Retrieves integer value from session_info cvar.
==================
*/
int G_RetrieveGlobalSessionValue( const char *key ) {
	info_string_t info;
	G_RetrieveGlobalSessionInfo( &info );
	return atoi( Info_ValueForKey( info.s, key ) );
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
	clientSession_t sess;
	info_string_t info;

	// Write session structure
	memset( &sess, 0, sizeof( sess ) );
	modfn.GenerateClientSessionStructure( clientNum, &sess );

	s = va("%i %i %i %i %i %i %i",
		sess.sessionTeam,
		sess.sessionClass,
		sess.spectatorTime,
		sess.spectatorState,
		sess.spectatorClient,
		sess.wins,
		sess.losses
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

	sscanf( s, "%i %i %i %i %i %i %i",
		&client->sess.sessionTeam,
		&client->sess.sessionClass,
		&client->sess.spectatorTime,
		&client->sess.spectatorState,
		&client->sess.spectatorClient,
		&client->sess.wins,
		&client->sess.losses
		);

	// Reset when switching to/from tournament mode
	if ( ( g_gametype.integer == GT_TOURNAMENT ) != ( trap_Cvar_VariableIntegerValue( "session" ) == GT_TOURNAMENT ) ) {
		client->sess.sessionTeam = TEAM_SPECTATOR;
		client->sess.wins = 0;
		client->sess.losses = 0;
	}

	// Make sure appropriate team is selected for gametype
	if ( g_gametype.integer >= GT_TEAM ) {
		if ( client->sess.sessionTeam != TEAM_RED && client->sess.sessionTeam != TEAM_BLUE ) {
			client->sess.sessionTeam = TEAM_SPECTATOR;
		}
	} else {
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			client->sess.sessionTeam = TEAM_FREE;
		}
	}

	// Make sure appropriate class is selected for mod
	modfn.UpdateSessionClass( clientNum );

	// Call mod initialization
	{
		info_string_t info;
		G_RetrieveClientSessionInfo( clientNum, &info );
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
	if ( g_gametype.integer >= GT_TEAM ) {
		// Team holomatch or CTF
		if ( g_teamAutoJoin.integer && !( g_entities[clientNum].r.svFlags & SVF_BOT ) &&
				modfn.CheckJoinAllowed( clientNum, CJA_AUTOJOIN, TEAM_FREE ) ) {
			sess->sessionTeam = PickTeam( clientNum );
		} else {
			// always spawn as spectator in team games
			sess->sessionTeam = TEAM_SPECTATOR;
		}
	}

	else {
		// FFA, tournament or sp
		value = Info_ValueForKey( userinfo, "team" );
		if ( value[0] == 's' ) {
			// a willing spectator, not a waiting-in-line
			sess->sessionTeam = TEAM_SPECTATOR;
		} else if ( modfn.CheckJoinAllowed( clientNum, CJA_AUTOJOIN, TEAM_FREE ) ) {
			sess->sessionTeam = TEAM_FREE;
		} else {
			sess->sessionTeam = TEAM_SPECTATOR;
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
G_WriteSessionData

==================
*/
void G_WriteSessionData( void ) {
	int		i;
	info_string_t info;

	trap_Cvar_Set( "session", va("%i*" SESSION_VERSION, g_gametype.integer) );

	// Write global session info
	info.s[0] = '\0';
	modfn.GenerateGlobalSessionInfo( &info );
	trap_Cvar_Set( "session_info", info.s );

	// Write client info
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected >= CON_CONNECTING ) {
			G_WriteClientSessionData( &level.clients[i] );
		}
	}
}
