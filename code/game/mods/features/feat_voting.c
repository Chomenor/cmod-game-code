/*
* Voting Support
* 
* This module implements basic support for the callvote command. It fixes significant issues
* compared to the stock implementation, but is still very limited compared to mods and only
* intended for simple uses.
* 
* More advanced voting features in cMod are planned to be handled by a separate engine-based
* system in the future, so adding features or vote options here is not considered a priority.
*/

#define MOD_NAME ModVoting

#include "mods/g_mod_local.h"

#define	MAX_VOTE_COUNT 3

typedef struct {
	char voteEligible;
	char voteCallCount;
} ModVotingClient_t;

typedef enum {
	VOTE_GENERAL,
	VOTE_MAP,
	VOTE_NEXTMAP,
} VoteCommandType_t;

// reset when a new vote is called
typedef struct {
	int voteTime;	// level.time vote was called; 0 if vote inactive
	int voteYes;
	int voteNo;
	int numVotingClients;

	// command to run when vote passes
	VoteCommandType_t commandType;
	char commandString[256];
} ModVoteState_t;

static struct {
	ModVotingClient_t clients[MAX_CLIENTS];
	trackedCvar_t g_allowVote;
	ModVoteState_t currentVote;
	char nextmapPending[256];
} *MOD_STATE;

/*
==================
ModVoting_LaunchMap

Runs a command to launch a map, but sets "nextmap" cvar back to original value afterwards to prevent
breaking rotations. Workaround for engine behavior in which "map" command resets "nextmap" cvar.
==================
*/
static void ModVoting_LaunchMap( const char *command ) {
	char oldNextmap[256];
	trap_Cvar_VariableStringBuffer( "nextmap", oldNextmap, sizeof( oldNextmap ) );
	EF_WARN_ASSERT( !strchr( oldNextmap, '"' ) );
	trap_SendConsoleCommand( EXEC_APPEND, va( "%s ; set nextmap \"%s\"\n", command, oldNextmap ) );
}

/*
==================
ModVoting_CallVote_f
==================
*/
static void ModVoting_CallVote_f( int clientNum ) {
	int i;
	char arg1[MAX_STRING_TOKENS];
	char arg2[MAX_STRING_TOKENS];
	char infoString[256] = { 0 };	// message to display on screen (if empty, use commandString instead)
	ModVoteState_t *cv = &MOD_STATE->currentVote;
	gclient_t *client = &level.clients[clientNum];
	ModVotingClient_t *modclient = &MOD_STATE->clients[clientNum];

	if ( !MOD_STATE->g_allowVote.integer ) {
		trap_SendServerCommand( clientNum, "print \"Voting not allowed here.\n\"" );
		return;
	}

	if ( cv->voteTime ) {
		trap_SendServerCommand( clientNum, "print \"A vote is already in progress.\n\"" );
		return;
	}
	if ( modclient->voteCallCount >= MAX_VOTE_COUNT ) {
		trap_SendServerCommand( clientNum, "print \"You have called the maximum number of votes.\n\"" );
		return;
	}

	memset( cv, 0, sizeof( *cv ) );
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	trap_Argv( 2, arg2, sizeof( arg2 ) );

	if ( !Q_stricmp( arg1, "map_restart" ) ) {
		Q_strncpyz( cv->commandString, "map_restart", sizeof( cv->commandString ) );

	} else if ( !Q_stricmp( arg1, "map" ) || !Q_stricmp( arg1, "nextmap" ) ) {
		if ( *arg2 ) {
			fileHandle_t f = 0;
			trap_FS_FOpenFile( va( "maps/%s.bsp", arg2 ), &f, FS_READ );
			if ( f ) {
				trap_FS_FCloseFile( f );
				Com_sprintf( cv->commandString, sizeof( cv->commandString ), "map %s", arg2 );
				if ( !Q_stricmp( arg1, "nextmap" ) ) {
					cv->commandType = VOTE_NEXTMAP;
				} else {
					cv->commandType = VOTE_MAP;
				}
			} else {
				trap_SendServerCommand( clientNum, "print \"Map not found.\n\"" );
				return;
			}
		}

	} else if ( !Q_stricmp( arg1, "g_gametype" ) ) {
		int gt = atoi( arg2 );
		if ( *arg2 && ( gt == 0 || gt == 1 || gt == 3 || gt == 4 ) ) {
			char currentMap[256];
			trap_Cvar_VariableStringBuffer( "mapname", currentMap, sizeof( currentMap ) );
			if ( !EF_WARN_ASSERT( *currentMap ) ) {
				return;
			}
			Com_sprintf( cv->commandString, sizeof( cv->commandString ), "set g_gametype %i ; map %s", gt, currentMap );
			Com_sprintf( infoString, sizeof( infoString ), "g_gametype %i", gt );
			cv->commandType = VOTE_MAP;
		} else {
			trap_SendServerCommand( clientNum, "print \"Gametype must be 0 (ffa), 1 (duel), 3 (thm), or 4 (ctf).\n\"" );
			return;
		}

	} else if ( !Q_stricmp( arg1, "g_doWarmup" ) ) {
		int val;
		if ( atoi( arg2 ) > 0 || arg2[0] == 'y' || arg2[0] == 'Y' ) {
			val = 1;
		} else if ( *arg2 ) {
			val = 0;
		} else {
			trap_SendServerCommand( clientNum, "print \"g_doWarmup must be 0 or 1.\n\"" );
			return;
		}
		Com_sprintf( cv->commandString, sizeof( cv->commandString ), "set g_doWarmup %i", val );
		Com_sprintf( infoString, sizeof( infoString ), "g_doWarmup %i", val );

	} else {
		trap_SendServerCommand( clientNum, "print \"Invalid vote string.\n\"" );
		trap_SendServerCommand( clientNum, "print \"Vote commands are: map_restart, map <mapname>,"
				" nextmap <mapname>, g_gametype <n>, and g_dowarmup <b>.\n\"" );
		return;
	}

	if ( !EF_WARN_ASSERT( *cv->commandString ) ) {
		return;
	}

	trap_SendServerCommand( -1, va("print \"%s called a vote.\n\"", client->pers.netname ) );

	// start the voting, the caller automatically votes yes
	++modclient->voteCallCount;
	cv->voteTime = level.time;
	cv->voteYes = 1;
	cv->voteNo = 0;
	cv->numVotingClients = 1;

	// make all connected, non-bot clients eligible to vote (except the caller, who was already counted)
	for ( i = 0; i < level.maxclients; i++ ) {
		gclient_t *cl = &level.clients[i];
		MOD_STATE->clients[i].voteEligible = 0;
		if ( level.clients[i].pers.connected == CON_CONNECTED && !( g_entities[i].r.svFlags & SVF_BOT )
				&& i != clientNum ) {
			MOD_STATE->clients[i].voteEligible = 1;
			++cv->numVotingClients;
		}
	}

	trap_SetConfigstring( CS_VOTE_TIME, va( "%i", cv->voteTime ) );
	trap_SetConfigstring( CS_VOTE_STRING, *infoString ? infoString : cv->commandString );
	trap_SetConfigstring( CS_VOTE_YES, va( "%i", cv->voteYes ) );
	trap_SetConfigstring( CS_VOTE_NO, va( "%i", cv->voteNo ) );
}

/*
==================
ModVoting_Vote_f
==================
*/
static void ModVoting_Vote_f( int clientNum ) {
	char msg[64];
	ModVoteState_t *cv = &MOD_STATE->currentVote;
	ModVotingClient_t *modclient = &MOD_STATE->clients[clientNum];

	if ( !cv->voteTime ) {
		trap_SendServerCommand( clientNum, "print \"No vote in progress.\n\"" );
		return;
	}
	if ( !modclient->voteEligible ) {
		trap_SendServerCommand( clientNum, "print \"Vote already cast.\n\"" );
		return;
	}

	trap_SendServerCommand( clientNum, "print \"Vote cast.\n\"" );

	modclient->voteEligible = qfalse;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[0] == 'Y' || msg[0] == '1' ) {
		cv->voteYes++;
		trap_SetConfigstring( CS_VOTE_YES, va( "%i", cv->voteYes ) );
	} else {
		cv->voteNo++;
		trap_SetConfigstring( CS_VOTE_NO, va( "%i", cv->voteNo ) );
	}
}

/*
==================
ModVoting_CheckVote
==================
*/
static void ModVoting_CheckVote( void ) {
	ModVoteState_t *cv = &MOD_STATE->currentVote;
	if ( !cv->voteTime ) {
		return;
	}
	if ( level.time - cv->voteTime >= VOTE_TIME ) {
		trap_SendServerCommand( -1, "print \"Vote failed.\n\"" );
	} else {
		if ( cv->voteYes > cv->numVotingClients / 2 ) {
			// execute the command, then remove the vote
			trap_SendServerCommand( -1, "print \"Vote passed.\n\"" );
			if ( cv->commandType == VOTE_MAP ) {
				ModVoting_LaunchMap( cv->commandString );
			} else if ( cv->commandType == VOTE_NEXTMAP ) {
				Q_strncpyz( MOD_STATE->nextmapPending, cv->commandString, sizeof( MOD_STATE->nextmapPending ) );
			} else {
				trap_SendConsoleCommand( EXEC_APPEND, va( "%s\n", cv->commandString ) );
			}
		} else if ( cv->voteNo >= cv->numVotingClients / 2 ) {
			// same behavior as a timeout
			trap_SendServerCommand( -1, "print \"Vote failed.\n\"" );
		} else {
			// still waiting for a majority
			return;
		}
	}
	cv->voteTime = 0;
	trap_SetConfigstring( CS_VOTE_TIME, "" );
}

/*
================
(ModFN) ModClientCommand
================
*/
static qboolean MOD_PREFIX(ModClientCommand)( MODFN_CTV, int clientNum, const char *cmd ) {
	if ( level.clients[clientNum].pers.connected == CON_CONNECTED && level.matchState < MS_INTERMISSION_QUEUED ) {
		if ( !Q_stricmp( cmd, "callvote" ) ) {
			ModVoting_CallVote_f( clientNum );
			return qtrue;
		}
		if ( !Q_stricmp( cmd, "vote" ) ) {
			ModVoting_Vote_f( clientNum );
			return qtrue;
		}
	}

	return MODFN_NEXT( ModClientCommand, ( MODFN_NC, clientNum, cmd ) );
}

/*
================
(ModFN) ExitLevel

Run pending nextmap if it was voted.
================
*/
static void MOD_PREFIX(ExitLevel)( MODFN_CTV ) {
	if ( *MOD_STATE->nextmapPending ) {
		ModVoting_LaunchMap( MOD_STATE->nextmapPending );
	} else {
		MODFN_NEXT( ExitLevel, ( MODFN_NC ) );
	}
}

/*
================
(ModFN) InitClientSession
================
*/
static void MOD_PREFIX(InitClientSession)( MODFN_CTV, int clientNum, qboolean initialConnect, const info_string_t *info ) {
	ModVotingClient_t *modclient = &MOD_STATE->clients[clientNum];
	memset( modclient, 0, sizeof( *modclient ) );
	MODFN_NEXT( InitClientSession, ( MODFN_NC, clientNum, initialConnect, info ) );
}

/*
================
(ModFN) PostRunFrame
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );
	ModVoting_CheckVote();
}

/*
================
ModVoting_Init
================
*/
void ModVoting_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		G_RegisterTrackedCvar( &MOD_STATE->g_allowVote, "g_allowVote", "1", CVAR_SERVERINFO, qfalse );

		MODFN_REGISTER( ModClientCommand, MODPRIORITY_GENERAL );
		MODFN_REGISTER( ExitLevel, MODPRIORITY_GENERAL );
		MODFN_REGISTER( InitClientSession, MODPRIORITY_GENERAL );
		MODFN_REGISTER( PostRunFrame, MODPRIORITY_GENERAL );
	}
}
