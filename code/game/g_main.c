// Copyright (C) 1999-2000 Id Software, Inc.
//

#ifndef MODULE_GAME
#error Building game module with no MODULE_GAME preprocessor definition. Check the build configuration.
#endif

#include "g_local.h"

level_locals_t	level;
gentity_t		g_entities[MAX_GENTITIES];
gclient_t		g_clients[MAX_CLIENTS];

#define MAX_TRACKED_CVARS 1024
static trackedCvar_t *trackedCvars[MAX_TRACKED_CVARS];
static int trackedCvarCount = 0;

#define CVAR_DEF( vmcvar, name, def, flags, announce ) trackedCvar_t vmcvar;
#include "g_cvar_defs.h"
#undef CVAR_DEF

void G_InitGame( int levelTime, int randomSeed, int restart );
void G_RunFrame( int levelTime );
void G_ShutdownGame( int restart );
static void G_CheckExitRules( void );
static int G_WarmupLength( void );

//=============================
//** begin code

/*
================
vmMain

This is the only way control passes into the module.
This MUST be the very first function compiled into the .q3vm file
================
*/
#ifdef _MSC_VER
__declspec(dllexport)
#endif
intptr_t vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6 ) {
	switch ( command ) {
	case GAME_INIT:
		G_InitGame( arg0, arg1, arg2 );
		return 0;
	case GAME_SHUTDOWN:
		G_ShutdownGame( arg0 );
		return 0;
	case GAME_CLIENT_CONNECT:
		if ( level.clients[arg0].pers.connected != CON_DISCONNECTED ) {
			// Engine can call this during reconnects without calling disconnect first.
			// Call ClientDisconnect here to to make sure flags are returned, etc.
			G_DedPrintf( "GAME_CLIENT_CONNECT: Reconnecting active client %i\n", arg0 );
			ClientDisconnect( arg0 );
		}
		return (intptr_t)ClientConnect( arg0, arg1, arg2 );
	case GAME_CLIENT_THINK:
		if ( EF_WARN_ASSERT( level.clients[arg0].pers.connected == CON_CONNECTED ) ) {
			ClientThink( arg0 );
		}
		return 0;
	case GAME_CLIENT_USERINFO_CHANGED:
		if ( EF_WARN_ASSERT( level.clients[arg0].pers.connected >= CON_CONNECTING ) ) {
			ClientUserinfoChanged( arg0 );
		}
		return 0;
	case GAME_CLIENT_DISCONNECT:
		if ( EF_WARN_ASSERT( level.clients[arg0].pers.connected >= CON_CONNECTING ) ) {
			ClientDisconnect( arg0 );
		}
		return 0;
	case GAME_CLIENT_BEGIN:
		// It is possible for client to already be spawned via "team" command, which
		// happens when starting game from the menu. Don't do anything here in this case.
		EF_WARN_ASSERT( level.clients[arg0].pers.connected >= CON_CONNECTING );
		if ( level.clients[arg0].pers.connected == CON_CONNECTING ) {
			ClientBegin( arg0 );
		}
		return 0;
	case GAME_CLIENT_COMMAND:
		if ( EF_WARN_ASSERT( level.clients[arg0].pers.connected >= CON_CONNECTING ) ) {
			ClientCommand( arg0 );
		}
		return 0;
	case GAME_RUN_FRAME:
		G_RunFrame( arg0 );
		return 0;
	case GAME_CONSOLE_COMMAND:
		return ConsoleCommand();
	case BOTAI_START_FRAME:
		return BotAIStartFrame( arg0 );
	}

	return -1;
}


void QDECL G_Printf( const char *fmt, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	vsprintf (text, fmt, argptr);
	va_end (argptr);

	trap_Printf( text );
}

void QDECL G_DedPrintf( const char *fmt, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	vsprintf (text, fmt, argptr);
	va_end (argptr);

	// support calling before cvar init completed
	if ( g_dedicated.handle ? g_dedicated.integer : trap_Cvar_VariableIntegerValue( "dedicated" ) ) {
		trap_Printf( text );
	}
}

void QDECL G_Error( const char *fmt, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	vsprintf (text, fmt, argptr);
	va_end (argptr);

	trap_Error( text );
}


/*
================
G_FindTeams

Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams( void ) {
	gentity_t	*e, *e2;
	int		i, j;
	int		c, c2;

	c = 0;
	c2 = 0;
	for ( i=1, e=g_entities+i ; i < level.num_entities ; i++,e++ ){
		if (!e->inuse)
			continue;
		if (!e->team)
			continue;
		if (e->flags & FL_TEAMSLAVE)
			continue;
		if ( e->classname && Q_stricmp( "func_door", e->classname ) != 0 )
		{//not a door
			if ( Q_stricmp( "1", e->team ) == 0 || Q_stricmp( "2", e->team ) == 0 )
			{//is trying to tell us it belongs to the TEAM_RED or TEAM_BLUE
				continue;
			}
		}
		e->teammaster = e;
		c++;
		c2++;
		for (j=i+1, e2=e+1 ; j < level.num_entities ; j++,e2++)
		{
			if (!e2->inuse)
				continue;
			if (!e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;
			if (!strcmp(e->team, e2->team))
			{
				c2++;
				e2->teamchain = e->teamchain;
				e->teamchain = e2;
				e2->teammaster = e;
				e2->flags |= FL_TEAMSLAVE;

				// make sure that targets only point at the master
				if ( e2->targetname ) {
					e->targetname = e2->targetname;
					e2->targetname = NULL;
				}
			}
		}
	}

	G_Printf ("%i teams with %i entities\n", c, c2);
}

// Only set mod config after initialization is complete to avoid unnecessary configstring updates.
static qboolean modConfigReady = qfalse;

/*
=================
G_UpdateModConfigInfo

Updates mod config configstring. Called on game startup and any time the value might have changed.
=================
*/
void G_UpdateModConfigInfo( void ) {
	if ( modConfigReady ) {
		char buffer[BIG_INFO_STRING + 8];
		char *info = buffer + 8;
		Q_strncpyz( buffer, "!modcfg ", sizeof( buffer ) );

		modfn.AddModConfigInfo( info );

		if ( *info ) {
			trap_SetConfigstring( CS_MOD_CONFIG, buffer );
		} else {
			trap_SetConfigstring( CS_MOD_CONFIG, "" );
		}
	}
}

/*
=================
G_UpdateNeedPass

Update the g_needpass serverinfo cvar when g_password changes.
=================
*/
static void G_UpdateNeedPass( trackedCvar_t *cv ) {
	if ( *g_password.string && Q_stricmp( g_password.string, "none" ) ) {
		trap_Cvar_Set( "g_needpass", "1" );
	} else {
		trap_Cvar_Set( "g_needpass", "0" );
	}
}

/*
=================
G_RegisterTrackedCvar

Registers a cvar which is automatically synchronized with the engine once per frame.
cvarName parameter should be a static string.
=================
*/
void G_RegisterTrackedCvar( trackedCvar_t *tc, const char *cvarName, const char *defaultValue, int flags, qboolean announceChanges ) {
	memset( tc, 0, sizeof( *tc ) );

	tc->cvarName = cvarName;
	tc->announceChanges = announceChanges;

	trap_Cvar_Register( (vmCvar_t *)tc, cvarName, defaultValue, flags );
	trap_Cvar_Update( (vmCvar_t *)tc );		// just to be safe...

	EF_ERR_ASSERT( trackedCvarCount < MAX_TRACKED_CVARS );
	trackedCvars[trackedCvarCount++] = tc;
}

/*
=================
G_RegisterCvarCallback

Registers a callback function to a tracked cvar which is invoked whenever the cvar is changed.
=================
*/
void G_RegisterCvarCallback( trackedCvar_t *tc, void ( *callback )( trackedCvar_t *tc ), qboolean callNow ) {
	cvarCallback_t *callbackObj = (cvarCallback_t *)G_Alloc( sizeof( cvarCallback_t ) );
	callbackObj->callback = callback;
	callbackObj->next = tc->callbackObj;
	tc->callbackObj = (void *)callbackObj;

	if ( callNow ) {
		callback( tc );
	}
}

/*
=================
G_UpdateTrackedCvar
=================
*/
void G_UpdateTrackedCvar( trackedCvar_t *tc ) {
	int oldModificationCount = tc->modificationCount;
	trap_Cvar_Update( (vmCvar_t *)tc );

	if ( tc->modificationCount != oldModificationCount ) {
		cvarCallback_t *callbackObj = (cvarCallback_t *)tc->callbackObj;

		// Print announcements
		if ( tc->announceChanges && !level.exiting )
			trap_SendServerCommand( -1, va( "print \"Server: %s changed to %s\n\"", tc->cvarName, tc->string ) );

		// Run callbacks
		while( callbackObj ) {
			callbackObj->callback( tc );
			callbackObj = (cvarCallback_t *)callbackObj->next;
		}
	}
}

/*
=================
G_RegisterCvars
=================
*/
static void G_RegisterCvars( void ) {
	// import from g_cvar_defs.h
	#define CVAR_DEF( vmcvar, name, def, flags, announce ) \
		G_RegisterTrackedCvar( &vmcvar, name, def, flags, announce );
	#include "g_cvar_defs.h"
	#undef CVAR_DEF

	// check g_gametype range
	if ( g_gametype.integer < 0 || g_gametype.integer >= GT_MAX_GAME_TYPE ) {
		G_Printf( "g_gametype %i is out of range, defaulting to 0\n", g_gametype.integer );
		trap_Cvar_Set( "g_gametype", "0" );
		G_UpdateTrackedCvar( &g_gametype );
	}

	// configure g_needpass auto update
	G_RegisterCvarCallback( &g_password, G_UpdateNeedPass, qtrue );
}

/*
=================
G_UpdateCvars
=================
*/
static void G_UpdateCvars( void ) {
	int i;
	for ( i = 0; i < trackedCvarCount; ++i ) {
		G_UpdateTrackedCvar( trackedCvars[i] );
	}
}

/*
============
G_InitGame

============
*/
void G_InitGame( int levelTime, int randomSeed, int restart ) {
	int					i;

	G_Printf ("------- Game Initialization -------\n");
	G_Printf ("gamename: %s\n", GAMEVERSION);
	G_Printf ("gamedate: %s\n", __DATE__);

	init_tonextint(qtrue);
	srand( randomSeed );

	// set some level globals
	level.time = levelTime;
	level.startTime = levelTime;
	level.hasRestarted = restart;

	level.matchState = MS_INIT;
	level.restartClientsPending = ( restart && G_RetrieveGlobalSessionValue( "numConnected" ) );
	level.warmupRestarting = ( restart && G_RetrieveGlobalSessionValue( "warmupRestarting" ) );

	G_ModsInit();

	// initialize match state directly to active after warmup reset
	if ( level.warmupRestarting ) {
		G_SetMatchState( MS_ACTIVE );
	}

	G_RegisterCvars();

	G_ProcessIPBans();

	BG_LoadItemNames();

	level.snd_fry = G_SoundIndex("sound/player/fry.wav");	// FIXME standing in lava / slime

	if ( g_gametype.integer != GT_SINGLE_PLAYER && g_log.string[0] ) {
		if ( g_logSync.integer ) {
			trap_FS_FOpenFile( g_log.string, &level.logFile, FS_APPEND_SYNC );
		} else {
			trap_FS_FOpenFile( g_log.string, &level.logFile, FS_APPEND );
		}
		if ( !level.logFile ) {
			G_Printf( "WARNING: Couldn't open logfile: %s\n", g_log.string );
		} else {
			char	serverinfo[MAX_INFO_STRING];

			trap_GetServerinfo( serverinfo, sizeof( serverinfo ) );

			G_LogPrintf("------------------------------------------------------------\n" );
			G_LogPrintf("InitGame: %s\n", serverinfo );
		}
	} else {
		G_Printf( "Not logging to disk.\n" );
	}


	G_LogWeaponInit();

	// initialize all entities for this game
	memset( g_entities, 0, MAX_GENTITIES * sizeof(g_entities[0]) );
	level.gentities = g_entities;

	// initialize all clients for this game
	level.maxclients = g_maxclients.integer;
	memset( g_clients, 0, MAX_CLIENTS * sizeof(g_clients[0]) );
	level.clients = g_clients;

	// set client fields on player ents
	for ( i=0 ; i<level.maxclients ; i++ ) {
		g_entities[i].client = level.clients + i;
	}

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS;

	// let the server system know where the entites are
	trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ),
		&level.clients[0].ps, sizeof( level.clients[0] ) );

	// reserve some spots for dead player bodies
	InitBodyQue();

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString();

	// general initialization
	G_FindTeams();

	// make sure we have flags for CTF, etc
	G_CheckTeamItems();

	SaveRegisteredItems();

	if( g_gametype.integer == GT_SINGLE_PLAYER || trap_Cvar_VariableIntegerValue( "com_buildScript" ) ) {
		G_ModelIndex( SP_PODIUM_MODEL );
		G_SoundIndex( "sound/player/gurp1.wav" );
		G_SoundIndex( "sound/player/gurp2.wav" );
	}
	if (g_gametype.integer >= GT_TEAM || trap_Cvar_VariableIntegerValue( "com_buildScript" ) )
	{
		G_ModelIndex( TEAM_PODIUM_MODEL );
	}

	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAISetup( restart );
		BotAILoadMap( restart );
		G_InitBots( restart );
	}

	// initialize warmup
	if ( G_WarmupLength() && !level.warmupRestarting ) {
		// no sound effect until countdown starts
		level.warmupTime = -1;
		trap_SetConfigstring( CS_WARMUP, "-1" );
		G_LogPrintf( "Warmup:\n" );
	} else {
		// play "begin" sound in CG_MapRestart
		level.warmupTime = 0;
		trap_SetConfigstring( CS_WARMUP, "" );
	}

	// additional mod init
	modfn.GeneralInit();

	level.warmupRestarting = qfalse;

	// set mod config string
	modConfigReady = qtrue;
	G_UpdateModConfigInfo();

	G_Printf ("-----------------------------------\n");
}

/*
=================
G_ShutdownGame
=================
*/
void G_ShutdownGame( int restart ) {
	G_Printf ("==== ShutdownGame ====\n");

#if 0	// kef -- Pat sez this is causing some trouble these days
	G_LogWeaponOutput();
#endif
	if ( level.logFile ) {
		G_LogPrintf("ShutdownGame:\n" );
		G_LogPrintf("------------------------------------------------------------\n" );
		trap_FS_FCloseFile( level.logFile );
	}

	// write all the client session data so we can get it back
	G_WriteSessionData();

	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAIShutdown( restart );
	}
}



//===================================================================

#ifndef GAME_HARD_LINKED
// this is only here so the functions in q_shared.c and bg_*.c can link

void QDECL Com_Error ( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	G_Error( "%s", text);
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	vsprintf (text, msg, argptr);
	va_end (argptr);

	G_Printf ("%s", text);
}

#endif

/*
========================================================================

PLAYER COUNTING / SCORE SORTING

========================================================================
*/

#define SELECT_LOWER if ( va < vb ) return -1; if ( vb < va ) return 1;
#define SELECT_HIGHER if ( va > vb ) return -1; if ( vb > va ) return 1;

/*
=============
SortRanks
=============
*/
int QDECL SortRanks( const void *a, const void *b ) {
	int ia = *(int *)a;
	int ib = *(int *)b;
	gclient_t *ca = &level.clients[ia];
	gclient_t *cb = &level.clients[ib];
	int va;
	int vb;

	// sort special clients last
	va = ca->sess.sessionTeam == TEAM_SPECTATOR &&
		( ca->sess.spectatorState == SPECTATOR_SCOREBOARD || ca->sess.spectatorClient < 0 );
	vb = cb->sess.sessionTeam == TEAM_SPECTATOR &&
		( cb->sess.spectatorState == SPECTATOR_SCOREBOARD || cb->sess.spectatorClient < 0 );
	SELECT_LOWER;

	// then connecting clients
	va = ca->pers.connected == CON_CONNECTING;
	vb = cb->pers.connected == CON_CONNECTING;
	SELECT_LOWER;

	// then spectators
	va = ca->sess.sessionTeam == TEAM_SPECTATOR;
	vb = cb->sess.sessionTeam == TEAM_SPECTATOR;
	SELECT_LOWER;

	if ( ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		va = ca->sess.spectatorTime;
		vb = cb->sess.spectatorTime;
		SELECT_LOWER;
	}

	else {
		// then sort by score & number of times killed
		va = modfn.EffectiveScore( ia, EST_REALSCORE );
		vb = modfn.EffectiveScore( ib, EST_REALSCORE );
		SELECT_HIGHER;

		va = ca->ps.persistant[PERS_KILLED];
		vb = cb->ps.persistant[PERS_KILLED];
		SELECT_LOWER;
	}

	// if all else is equal, sort by clientnum
	va = ia;
	vb = ib;
	SELECT_LOWER;
	return 0;
}

/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks( void ) {
	int		i;
	int		rank;
	int		score;
	int		newScore;
	gclient_t	*cl;

	level.numConnectedClients = 0;
	level.numNonSpectatorClients = 0;
	level.numPlayingClients = 0;
	level.numVotingClients = 0;		// don't count bots
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected != CON_DISCONNECTED ) {
			level.sortedClients[level.numConnectedClients] = i;
			level.numConnectedClients++;

			if ( level.clients[i].sess.sessionTeam != TEAM_SPECTATOR ) {
				level.numNonSpectatorClients++;

				if ( level.clients[i].pers.connected == CON_CONNECTED ) {
					level.numPlayingClients++;
					if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
						level.numVotingClients++;
					}
				}
			}
		}
	}

	qsort( level.sortedClients, level.numConnectedClients,
		sizeof(level.sortedClients[0]), SortRanks );

	// set the rank value for all clients that are connected and not spectators
	if ( g_gametype.integer >= GT_TEAM ) {
		// in team games, rank is just the order of the teams, 0=red, 1=blue, 2=tied
		int rankValue;
		if ( level.forceWinningTeam == TEAM_RED ) {
			level.winningTeam = TEAM_RED;
			rankValue = 0;
		} else if ( level.forceWinningTeam == TEAM_BLUE ) {
			level.winningTeam = TEAM_BLUE;
			rankValue = 1;
		} else if ( level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE] ) {
			level.winningTeam = TEAM_RED;
			rankValue = 0;
		} else if ( level.teamScores[TEAM_BLUE] > level.teamScores[TEAM_RED] ) {
			level.winningTeam = TEAM_BLUE;
			rankValue = 1;
		} else {
			level.winningTeam = TEAM_FREE;
			rankValue = 2;
		}

		for ( i = 0;  i < level.numConnectedClients; i++ ) {
			cl = &level.clients[ level.sortedClients[i] ];
			cl->ps.persistant[PERS_RANK] = rankValue;
		}
	} else {
		rank = -1;
		score = 0;
		for ( i = 0;  i < level.numPlayingClients; i++ ) {
			newScore = modfn.EffectiveScore( level.sortedClients[i], EST_REALSCORE );
			if ( i == 0 || newScore != score ) {
				rank = i;
				// assume we aren't tied until the next client is checked
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank;
			} else {
				// we are tied with the previous client
				level.clients[ level.sortedClients[i-1] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
			score = newScore;
			if ( g_gametype.integer == GT_SINGLE_PLAYER && level.numPlayingClients == 1 ) {
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
		}
	}

	// set the CS_SCORES1/2 configstrings, which will be visible to everyone
	if ( g_gametype.integer >= GT_TEAM ) {
		trap_SetConfigstring( CS_SCORES1, va("%i", level.teamScores[TEAM_RED] ) );
		trap_SetConfigstring( CS_SCORES2, va("%i", level.teamScores[TEAM_BLUE] ) );
	} else {
		if ( level.numConnectedClients == 0 ) {
			trap_SetConfigstring( CS_SCORES1, va("%i", SCORE_NOT_PRESENT) );
			trap_SetConfigstring( CS_SCORES2, va("%i", SCORE_NOT_PRESENT) );
		} else if ( level.numConnectedClients == 1 ) {
			trap_SetConfigstring( CS_SCORES1, va("%i", modfn.EffectiveScore( level.sortedClients[0], EST_REALSCORE ) ) );
			trap_SetConfigstring( CS_SCORES2, va("%i", SCORE_NOT_PRESENT) );
		} else {
			trap_SetConfigstring( CS_SCORES1, va("%i", modfn.EffectiveScore( level.sortedClients[0], EST_REALSCORE ) ) );
			trap_SetConfigstring( CS_SCORES2, va("%i", modfn.EffectiveScore( level.sortedClients[1], EST_REALSCORE ) ) );
		}
	}

	// run exit check here to ensure exit is processed at the first qualified time
	// if we waited until next frame, simultaneous events like multiple players being hit by explosion
	// could be handled incorrectly
	G_CheckExitRules();
}


/*
========================================================================

MAP CHANGING

========================================================================
*/

/*
========================
SendScoreboardMessageToAllClients

Do this at BeginIntermission time and whenever ranks are recalculated
due to enters/exits/forced team changes
========================
*/
void SendScoreboardMessageToAllClients( void ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			DeathmatchScoreboardMessage( g_entities + i );
		}
	}
}

/*
========================
MoveClientToIntermission

When the intermission starts, this will be called for all players.
If a new client connects, this will be called from the spawn function.
========================
*/
void MoveClientToIntermission( gentity_t *ent ) {
	gclient_t *client = ent->client;

	// take out of follow mode if needed
	if ( modfn.SpectatorClient( ent - g_entities ) && client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		StopFollowing( ent );
	}

	// reset client
	trap_UnlinkEntity( ent );
	G_ResetClient( ent - g_entities );

	// move to the spot
	VectorCopy( level.intermission_origin, ent->s.origin );
	VectorCopy( level.intermission_origin, ent->client->ps.origin );
	VectorCopy (level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pm_type = PM_INTERMISSION;

	ent->s.eFlags = 0;
	ent->s.eType = ET_GENERAL;
	ent->s.modelindex = 0;
	ent->s.loopSound = 0;
	ent->s.event = 0;
	ent->r.contents = 0;

	// Don't trip the out of ammo sound in CG_CheckAmmo
	client->ps.stats[STAT_WEAPONS] = 1 << WP_PHASER;
	client->ps.ammo[WP_PHASER] = 100;
}

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
void FindIntermissionPoint( void ) {
	gentity_t	*ent, *target;
	vec3_t		dir;

	// find the intermission spot
	ent = G_Find (NULL, FOFS(classname), "info_player_intermission");
	if ( !ent ) {	// the map creator forgot to put in an intermission point...
		SelectSpawnPoint ( vec3_origin, level.intermission_origin, level.intermission_angle, qtrue, qtrue );
	} else {
		VectorCopy (ent->s.origin, level.intermission_origin);
		VectorCopy (ent->s.angles, level.intermission_angle);
		// if it has a target, look towards it
		if ( ent->target ) {
			target = G_PickTarget( ent->target );
			if ( target ) {
				VectorSubtract( target->s.origin, level.intermission_origin, dir );
				vectoangles( dir, level.intermission_angle );
			}
		}
	}

}

/*
==================
BeginIntermission
==================
*/
void BeginIntermission( void ) {
	int			i;
	gentity_t	*client;
	qboolean	doingLevelshot;

	if (level.intermissiontime == -1)
		doingLevelshot = qtrue;
	else
		doingLevelshot = qfalse;

	if ( level.intermissiontime && level.intermissiontime != -1 ) {
		return;		// already active
	}

	level.intermissiontime = level.time;
	FindIntermissionPoint();

	// cdr - Want to generate victory pads for all game types  - except level shots (gametype 10)
	UpdateTournamentInfo();
	if (!doingLevelshot)
		SpawnModelsOnVictoryPads();


	// move all clients to the intermission point
	for (i=0 ; i< level.maxclients ; i++) {
		client = g_entities + i;
		if (!client->inuse)
			continue;
		MoveClientToIntermission( client );
	}

	// send the current scoring to all clients
	SendScoreboardMessageToAllClients();

	G_SetMatchState( MS_INTERMISSION_ACTIVE );
}


void G_ClearObjectives( void )
{
	gentity_t *tent;

	tent = G_TempEntity( vec3_origin, EV_OBJECTIVE_COMPLETE );
	//Be sure to send the event to everyone
	tent->r.svFlags |= SVF_BROADCAST;
	tent->s.eventParm = 0;//tells it to clear all
}

/*
=============
(ModFN) ExitLevel

Execute change to next round or map when intermission completes.
=============
*/
LOGFUNCTION_VOID( ModFNDefault_ExitLevel, ( void ), (), "G_MATCHSTATE" ) {
	trap_SendConsoleCommand( EXEC_APPEND, "vstr nextmap\n" );
}

/*
=============
G_ExitLevel

Wrapper for ExitLevel modfn.
=============
*/
LOGFUNCTION_VOID( G_ExitLevel, ( void ), (), "G_MATCHSTATE" ) {
	level.exiting = qtrue;
	BotInterbreedEndMatch();
	G_ClearObjectives();
	modfn.ExitLevel();
}

/*
=================
CheckIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 20 seconds before going on.
=================
*/
void CheckIntermissionExit( void ) {
	int			ready, notReady;
	int			i;
	gclient_t	*cl;
	int			readyMask;


	if ( level.exiting )
	{//already on our way out, skip the check
		return;
	}

	if ( level.time < level.intermissiontime + 5000 )
	{
			// bring up the scoreboard after 5 seconds
	}


	// Single player exit does not happen until menu event
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		return;
	}

	// see which players are ready
	ready = 0;
	notReady = 0;
	readyMask = 0;
	for (i=0 ; i< g_maxclients.integer ; i++) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( g_entities[i].r.svFlags & SVF_BOT ) {
			continue;
		}

		if ( cl->readyToExit ) {
			ready++;
			if ( i < 16 ) {
				readyMask |= 1 << i;
			}
		} else {
			notReady++;
		}
	}

	// copy the readyMask to each player's stats so
	// it can be displayed on the scoreboard
	for (i=0 ; i< g_maxclients.integer ; i++) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		cl->ps.stats[STAT_CLIENTS_READY] = readyMask;
	}

	// never exit in less than five seconds
	if ( level.time < level.intermissiontime + 5000 ) {
		return;
	}

	// if nobody wants to go, clear timer
	if ( !ready ) {
		level.readyToExit = qfalse;
		return;
	}

	// if everyone wants to go, go now
	if ( !notReady ) {
		G_ExitLevel();
		return;
	}

	// the first person to ready starts the ten second timeout
	if ( !level.readyToExit ) {
		level.readyToExit = qtrue;
		level.exitTime = level.time;
	}

	// if we have waited g_intermissionTime seconds since at least one player
	// wanted to exit, go ahead
	if ( level.time < level.exitTime + (1000 * g_intermissionTime.integer) ) {
		return;
	}

	G_ExitLevel();
}

/*
=============
ScoreIsTied
=============
*/
qboolean ScoreIsTied( void ) {
	int		a, b;

	if ( level.numPlayingClients < 2 ) {
		return qfalse;
	}

	if ( g_gametype.integer >= GT_TEAM ) {
		return level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE];
	}

	a = modfn.EffectiveScore( level.sortedClients[0], EST_REALSCORE );
	b = modfn.EffectiveScore( level.sortedClients[1], EST_REALSCORE );

	return a == b;
}

/*
==================
(ModFN) CheckExitRules

Checks for initiating match exit to intermission.
Called every frame and after CalculateRanks, which is called on events like
   player death, connect, disconnect, team change, etc.
Not called if warmup or intermission is already in progress.
==================
*/
LOGFUNCTION_VOID( ModFNDefault_CheckExitRules, ( void ), (), "G_MODFN_CHECKEXITRULES" ) {
	int			i;
	gclient_t	*cl;

	if ( level.numPlayingClients < 2 ) {
		//not enough players
		return;
	}

	if ( g_timelimit.integer && !level.warmupTime )
	{//check timelimit
		if ( level.time - level.startTime >= g_timelimit.integer*60000 ) {
			// check for sudden death
			if ( g_gametype.integer != GT_CTF && ScoreIsTied() ) {
				// score is tied, so don't end the game
				return;
			}
			//if timelimit causes a team to win, we need to fudge the score
			if ( g_timelimitWinningTeam.string[0] == 'r' || g_timelimitWinningTeam.string[0] == 'R' )
			{
				if ( g_capturelimit.integer > level.teamScores[TEAM_BLUE])
				{
					level.teamScores[TEAM_RED] = g_capturelimit.integer;
				}
				else
				{
					level.teamScores[TEAM_RED] = level.teamScores[TEAM_BLUE] + 1;
				}
				CalculateRanks();
			}
			else if ( g_timelimitWinningTeam.string[0] == 'b' || g_timelimitWinningTeam.string[0] == 'B' )
			{
				if ( g_capturelimit.integer > level.teamScores[TEAM_RED])
				{
					level.teamScores[TEAM_BLUE] = g_capturelimit.integer;
				}
				else
				{
					level.teamScores[TEAM_BLUE] = level.teamScores[TEAM_RED] + 1;
				}
				CalculateRanks();
			}
			trap_SendServerCommand( -1, "print \"Timelimit hit.\n\"");
			G_LogExit( "Timelimit hit." );
			return;
		}
	}

	if ( g_gametype.integer != GT_CTF && g_fraglimit.integer )
	{//check fraglimit
		if ( level.teamScores[TEAM_RED] >= g_fraglimit.integer ) {
			trap_SendServerCommand( -1, "print \"Red hit the point limit.\n\"" );
			G_LogExit( "Fraglimit hit." );
			return;
		}

		if ( level.teamScores[TEAM_BLUE] >= g_fraglimit.integer ) {
			trap_SendServerCommand( -1, "print \"Blue hit the point limit.\n\"" );
			G_LogExit( "Fraglimit hit." );
			return;
		}

		for ( i=0 ; i< g_maxclients.integer ; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED ) {
				continue;
			}
			if ( cl->sess.sessionTeam != TEAM_FREE ) {
				continue;
			}

			if ( modfn.EffectiveScore( i, EST_REALSCORE ) >= g_fraglimit.integer ) {
				G_LogExit( "Fraglimit hit." );
				trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " hit the point limit.\n\"",
					cl->pers.netname ) );
				return;
			}
		}
	}

	if ( g_gametype.integer == GT_CTF && g_capturelimit.integer )
	{//check CTF
		if ( level.teamScores[TEAM_RED] >= g_capturelimit.integer ) {
			trap_SendServerCommand( -1, "print \"Red hit the capturelimit.\n\"" );
			G_LogExit( "Capturelimit hit." );
			return;
		}

		if ( level.teamScores[TEAM_BLUE] >= g_capturelimit.integer ) {
			trap_SendServerCommand( -1, "print \"Blue hit the capturelimit.\n\"" );
			G_LogExit( "Capturelimit hit." );
			return;
		}
	}
}

/*
==================
G_CheckExitRules

Wrapper for CheckExitRules modfn.
==================
*/
static void G_CheckExitRules( void ) {
	// make sure no recursive calls via CalculateRanks
	static qboolean recursive = qfalse;
	if ( recursive ) {
		return;
	}

	// only check for exits during active match
	if ( level.matchState == MS_ACTIVE ) {
		recursive = qtrue;
		modfn.CheckExitRules();
		recursive = qfalse;
	}
}



/*
========================================================================

FUNCTIONS CALLED EVERY FRAME

========================================================================
*/


/*
==================
G_CheckEnoughPlayers

Returns qtrue if there are enough players for a match.
Determines whether to reset warmup to "waiting for players" state.
==================
*/
static qboolean G_CheckEnoughPlayers( void ) {
	if ( g_gametype.integer >= GT_TEAM ) {
		if ( TeamCount( -1, TEAM_BLUE, qfalse ) < 1 || TeamCount( -1, TEAM_RED, qfalse ) < 1 ) {
			return qfalse;
		}
	} else {
		if ( level.numNonSpectatorClients < 2 ) {
			return qfalse;
		}
	}

	return qtrue;
}

/*
==================
G_WarmupLength

Returns length in milliseconds to use for warmup if enabled, or 0 if warmup disabled.
==================
*/
static int G_WarmupLength( void ) {
	if ( g_doWarmup.integer && g_gametype.integer != GT_SINGLE_PLAYER ) {
		int length = ( g_warmup.integer - 1 ) * 1000;
		if ( length < 1000 ) {
			length = 1000;
		}
		return length;
	}

	return 0;
}

/*
==================
G_SetWarmupState

Sets current warmup state

-1: waiting for players
0: no warmup
>0: length of warmup
==================
*/
static void G_SetWarmupState( int length ) {
	int oldTime = level.warmupTime;
	level.warmupTime = length > 0 ? level.time + length : length;
	level.warmupLength = length;

	if( oldTime == level.warmupTime ) {
		// don't touch configstring unless necessary, as it can interfere with the warmup mechanism
		// the engine uses for map_restart with g_doWarmup 0
		return;
	}

	if( level.warmupTime && !oldTime ) {
		G_LogPrintf( "Warmup:\n" );
	}

	trap_SetConfigstring( CS_WARMUP, level.warmupTime ? va( "%i", level.warmupTime ) : "" );
}

/*
==================
G_SetMatchState
==================
*/
void G_SetMatchState( matchState_t matchState ) {
	if ( level.matchState != matchState ) {
		matchState_t oldState = level.matchState;

		if ( g_dedicated.integer ) {
			G_Printf( "matchstate: Transitioning from %s to %s\n",
					G_MatchStateString( level.matchState ), G_MatchStateString( matchState ) );
		}

		level.matchState = matchState;
		modfn.MatchStateTransition( oldState, matchState );
	}
}

/*
==================
G_CheckMatchState
==================
*/
static void G_CheckMatchState( void ) {
	// wait for clients to be reconnected during restart process
	if ( level.restartClientsPending ) {
		if ( !level.numConnectedClients ) {
			return;
		}
		level.restartClientsPending = qfalse;
	}

	// check for starting queued intermission
	G_CheckExitRules();

	if ( level.matchState == MS_INTERMISSION_ACTIVE ) {
		// current intermission - check for exit to next match
		CheckIntermissionExit();

	} else if ( level.matchState == MS_INTERMISSION_QUEUED ) {
		// queued intermission - check for transition to actual intermission
		if ( level.time - level.intermissionQueued >= modfn.AdjustGeneralConstant( GC_INTERMISSION_DELAY_TIME, 2000 ) ) {
			level.intermissionQueued = 0;
			BeginIntermission();
		}

	} else if ( G_CheckEnoughPlayers() ) {
		// enough players
		int warmupLength = G_WarmupLength();

		if ( warmupLength > 0 && level.matchState == MS_WARMUP ) {
			// warmup countdown in progress
			if ( warmupLength != level.warmupLength ) {
				// restart warmup if length was changed
				G_SetWarmupState( warmupLength );
			}

			if ( level.time > level.warmupTime ) {
				// countdown complete; begin the game
				level.warmupRestarting = qtrue;
				level.exiting = qtrue;
				trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			}
		}

		else if ( warmupLength > 0 && level.matchState < MS_ACTIVE ) {
			// begin warmup countdown
			G_SetWarmupState( warmupLength );
			G_SetMatchState( MS_WARMUP );
		}

		else {
			// warmup completed or disabled
			G_SetWarmupState( 0 );
			G_SetMatchState( MS_ACTIVE );
		}

	} else {
		// not enough players
		if ( G_WarmupLength() > 0 ) {
			G_SetWarmupState( -1 );
		} else {
			G_SetWarmupState( 0 );
		}

		G_SetMatchState( MS_WAITING_FOR_PLAYERS );
	}
}


/*
==================
CheckVote
==================
*/
void CheckVote( void ) {
	if ( !level.voteTime ) {
		return;
	}
	if ( level.time - level.voteTime >= VOTE_TIME ) {
		trap_SendServerCommand( -1, "print \"Vote failed.\n\"" );
	} else {
		if ( level.voteYes > level.numVotingClients/2 ) {
			// execute the command, then remove the vote
			trap_SendServerCommand( -1, "print \"Vote passed.\n\"" );
			trap_SendConsoleCommand( EXEC_APPEND, va("%s\n", level.voteString ) );
		} else if ( level.voteNo >= level.numVotingClients/2 ) {
			// same behavior as a timeout
			trap_SendServerCommand( -1, "print \"Vote failed.\n\"" );
		} else {
			// still waiting for a majority
			return;
		}
	}
	level.voteTime = 0;
	trap_SetConfigstring( CS_VOTE_TIME, "" );

}


/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink (gentity_t *ent) {
	float	thinktime;

	thinktime = ent->nextthink;
	if (thinktime <= 0) {
		return;
	}
	if (thinktime > level.time) {
		return;
	}

	ent->nextthink = 0;
	if (!ent->think) {
		G_Error ( "NULL ent->think");
	}
	ent->think (ent);
}

/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/
void CheckHealthInfoMessage( void );
void G_RunFrame( int levelTime ) {
	int			i;
	gentity_t	*ent;
	int			msec;
	qboolean	skipRunMissile = modfn.AdjustGeneralConstant( GC_SKIP_RUN_MISSILE, 0 ) ? qtrue : qfalse;
int start, end;

	// if we are waiting for the level to restart, do nothing
	if ( level.exiting ) {
		return;
	}

	level.framenum++;
	level.previousTime = level.time;
	level.time = levelTime;
	msec = level.time - level.previousTime;

	// get any cvar changes
	G_UpdateCvars();

	modfn.PreRunFrame();

	//
	// go through all allocated objects
	//
start = trap_Milliseconds();
	ent = &g_entities[0];
	for (i=0 ; i<level.num_entities ; i++, ent++) {
		if ( !ent->inuse ) {
			continue;
		}

		// clear events that are too old
		if ( level.time - ent->eventTime > EVENT_VALID_MSEC ) {
			if ( ent->s.event ) {
				ent->s.event = 0;	// &= EV_EVENT_BITS;
				if ( ent->client ) {
					ent->client->ps.externalEvent = 0;
					ent->client->ps.events[0] = 0;
					ent->client->ps.events[1] = 0;
					ent->client->ps.events[2] = 0;
					ent->client->ps.events[3] = 0;
				}
			}
			if ( ent->freeAfterEvent ) {
				// tempEntities or dropped items completely go away after their event
				G_FreeEntity( ent );
				continue;
			} else if ( ent->unlinkAfterEvent ) {
				// items that will respawn will hide themselves after their pickup event
				ent->unlinkAfterEvent = qfalse;
				trap_UnlinkEntity( ent );
			}
		}

		// temporary entities don't think
		if ( ent->freeAfterEvent ) {
			continue;
		}

		if ( !ent->r.linked && ent->neverFree ) {
			continue;
		}

		if ( !ent->client )
		{
			if ( ent->s.eFlags & EF_ANIM_ONCE )
			{//this must be capped render-side
				ent->s.frame++;
			}
		}

		if ( (ent->s.eType == ET_MISSILE) || (ent->s.eType == ET_ALT_MISSILE) ) {
			if ( !skipRunMissile ) {
				G_RunMissile( ent );
			}
			continue;
		}

		if ( ent->s.eType == ET_ITEM || ent->physicsObject ) {
			G_RunItem( ent );
			continue;
		}

		if ( ent->s.eType == ET_MOVER ) {
			G_RunMover( ent );
			continue;
		}

		if ( i < MAX_CLIENTS ) {
			G_RunClient( ent );
			continue;
		}

		G_RunThink( ent );
	}
end = trap_Milliseconds();

start = trap_Milliseconds();
	// perform final fixups on the players
	ent = &g_entities[0];
	for (i=0 ; i < level.maxclients ; i++, ent++ ) {
		if ( ent->inuse ) {
			ClientEndFrame( ent );
		}
	}
end = trap_Milliseconds();

	// check for transitions between warmup, intermission, and match states
	G_CheckMatchState();
	if ( level.exiting ) {
		return;
	}

	// update to team status?
	CheckTeamStatus();

	// update to health status
	//CheckHealthInfoMessage();//done from inside CheckTeamStatus now

	// cancel vote if timed out
	CheckVote();

	// run general mod activities
	modfn.PostRunFrame();
}
