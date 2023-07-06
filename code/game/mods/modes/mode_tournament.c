/*
* Tournament Mode
* 
* This mode works like FFA mode, but with only two players in the game at one time,
* while the rest are spectators. When the match ends, the winner remains in the game
* for next match, while the loser is removed and replaced with the spectator who
* has been waiting the longest.
*/

#define MOD_NAME ModTournament

#include "mods/g_mod_local.h"

static struct {
	int losingClient;
} *MOD_STATE;

/*
=============
(ModFN) ExitLevel

Currently tournament just restarts the rounds indefinitely rather than changing map.
=============
*/
static void MOD_PREFIX(ExitLevel)( MODFN_CTV ) {
	trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
}

/*
================
(ModFN) CheckJoinAllowed

Check if joining or changing team/class is disabled due to match in progress.

If join was blocked and messageClientNum >= 0, sends appropriate notification message to client.
================
*/
static qboolean MOD_PREFIX(CheckJoinAllowed)( MODFN_CTV, int clientNum, join_allowed_type_t type, team_t targetTeam ) {
	gclient_t *client = &level.clients[clientNum];

	// Class changes are allowed only during warmup
	if ( type == CJA_SETCLASS ) {
		if ( !level.warmupTime ) {
			trap_SendServerCommand( clientNum, "cp \"Can't change classes during match.\"" );
			return qfalse;
		}
	}

	else if ( level.numNonSpectatorClients >= 2 ) {
		if ( targetTeam != client->sess.sessionTeam ) {
			if ( type == CJA_FORCETEAM ) {
				G_Printf( "Match already in progress.\n" );
			} else if ( type != CJA_AUTOJOIN ) {
				trap_SendServerCommand( clientNum, "cp \"Match already in progress.\"" );
			}
		}
		
		return qfalse;
	}

	return MODFN_NEXT( CheckJoinAllowed, ( MODFN_NC, clientNum, type, targetTeam ) );
}

/*
================
(ModFN) PrePlayerLeaveTeam

If we are playing in a tournament game and losing, give a win to other player
================
*/
static void MOD_PREFIX(PrePlayerLeaveTeam)( MODFN_CTV, int clientNum, team_t oldTeam ) {
	gclient_t *client = &level.clients[clientNum];

	MODFN_NEXT( PrePlayerLeaveTeam, ( MODFN_NC, clientNum, oldTeam ) );

	if ( !level.warmupTime && !level.intermissionQueued && !level.intermissiontime &&
			level.numPlayingClients == 2 && level.sortedClients[1] == clientNum ) {
		int opponentNum = level.sortedClients[0];
		gclient_t *opponent = &level.clients[opponentNum];

		if ( opponent->ps.persistant[PERS_SCORE] > client->ps.persistant[PERS_SCORE] ) {
			// Count a win for opponent
			opponent->sess.wins++;
			ClientUserinfoChanged( opponentNum );

			// Count a loss for this player
			client->sess.losses++;
			ClientUserinfoChanged( clientNum );
		}
	}
}

/*
================
(ModFN) GenerateClientSessionStructure

Save wins, losses, and wait time state for client.
================
*/
static void MOD_PREFIX(GenerateClientSessionStructure)( MODFN_CTV, int clientNum, clientSession_t *sess ) {
	gclient_t *client = &level.clients[clientNum];

	MODFN_NEXT( GenerateClientSessionStructure, ( MODFN_NC, clientNum, sess ) );

	sess->spectatorTime = client->sess.spectatorTime;
	sess->wins = client->sess.wins;
	sess->losses = client->sess.losses;

	// Set the loser of this round to spectator in back of queue for next round
	if ( clientNum == MOD_STATE->losingClient ) {
		sess->sessionTeam = TEAM_SPECTATOR;
		sess->spectatorState = SPECTATOR_FREE;
		sess->spectatorTime = level.time;
	}
}

/*
==================
(ModFN) AdjustGeneralConstant
==================
*/
static int MOD_PREFIX(AdjustGeneralConstant)( MODFN_CTV, generalConstant_t gcType, int defaultValue ) {
	// Currently no enter game message in tournament.
	if ( gcType == GC_SKIP_ENTER_GAME_PRINT )
		return 1;

	return MODFN_NEXT( AdjustGeneralConstant, ( MODFN_NC, gcType, defaultValue ) );
}

/*
===========
(ModFN) SpawnCenterPrintMessage

Prints info messages to clients during ClientSpawn.
============
*/
static void MOD_PREFIX(SpawnCenterPrintMessage)( MODFN_CTV, int clientNum, clientSpawnType_t spawnType ) {
	gclient_t *client = &level.clients[clientNum];

	if ( client->sess.sessionTeam == TEAM_SPECTATOR || spawnType != CST_RESPAWN ) {
		trap_SendServerCommand( clientNum, "cp \"Tournament\"" );
	}
}

/*
================
ModTournament_CheckAddPlayers

If there are less than two tournament players, put a
spectator in the game and restart
================
*/
static void ModTournament_CheckAddPlayers( void ) {
	int			i;
	gclient_t	*client;
	gclient_t	*nextInLine;

	if ( level.numPlayingClients >= 2 ) {
		return;
	}

	// never change during intermission
	if ( level.matchState >= MS_INTERMISSION_QUEUED ) {
		return;
	}

	nextInLine = NULL;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		// never select the dedicated follow or scoreboard clients
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD || client->sess.spectatorClient < 0  ) {
			continue;
		}

		if ( !nextInLine || client->sess.spectatorTime < nextInLine->sess.spectatorTime ) {
			nextInLine = client;
		}
	}

	if ( !nextInLine ) {
		return;
	}

	// set them to free-for-all team
	SetTeam( &g_entities[ nextInLine - level.clients ], "f", qtrue );

	// if player is being put in mid-match and warmup is disabled, make game is restarted
	if ( level.time - level.startTime > 1000 && modfn.WarmupLength() <= 0 ) {
		trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
		level.exiting = qtrue;
	}
}

/*
================
(ModFN) PostRunFrame

Check for adding more players to match.
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );

	ModTournament_CheckAddPlayers();
}

/*
================
(ModFN) MatchStateTransition
================
*/
static void MOD_PREFIX(MatchStateTransition)( MODFN_CTV, matchState_t oldState, matchState_t newState ) {
	MODFN_NEXT( MatchStateTransition, ( MODFN_NC, oldState, newState ) );

	// If exit was triggered, it means we should have a valid winner and loser, so update scores
	if ( newState == MS_INTERMISSION_QUEUED && EF_WARN_ASSERT( oldState < MS_INTERMISSION_QUEUED )
			&& EF_WARN_ASSERT( level.numPlayingClients == 2 ) ) {
		level.clients[level.sortedClients[0]].sess.wins++;
		ClientUserinfoChanged( level.sortedClients[0] );

		level.clients[level.sortedClients[1]].sess.losses++;
		ClientUserinfoChanged( level.sortedClients[1] );

		MOD_STATE->losingClient = level.sortedClients[1];
	}
}

/*
================
ModTournament_Init
================
*/
void ModTournament_Init( void ) {
	if ( EF_WARN_ASSERT( !MOD_STATE ) ) {
		modcfg.mods_enabled.tournament = qtrue;
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );
		MOD_STATE->losingClient = -1;

		MODFN_REGISTER( ExitLevel, ++modePriorityLevel );
		MODFN_REGISTER( SpawnCenterPrintMessage, ++modePriorityLevel );

		if ( trap_Cvar_VariableIntegerValue( "g_modsEnabled" ) >= 2 &&
				G_ModUtils_GetLatchedValue( "g_mod_uam", "0", 0 ) ) {
			ModUAM_Init();
		} else if ( trap_Cvar_VariableIntegerValue( "g_modsEnabled" ) >= 2 &&
				G_ModUtils_GetLatchedValue( "g_mod_razor", "0", 0 ) ) {
			ModRazor_Init();
		} else if ( G_ModUtils_GetLatchedValue( "g_pModElimination", "0", 0 ) ) {
			ModElimination_Init();
		} else if ( G_ModUtils_GetLatchedValue( "g_pModDisintegration", "0", 0 ) ) {
			ModDisintegration_Init();
		} else if ( G_ModUtils_GetLatchedValue( "g_pModSpecialties", "0", 0 ) ) {
			ModSpecialties_Init();
		} else if ( G_ModUtils_GetLatchedValue( "g_pModActionHero", "0", 0 ) ) {
			ModActionHero_Init();
		}

		MODFN_REGISTER( CheckJoinAllowed, ++modePriorityLevel );
		MODFN_REGISTER( PrePlayerLeaveTeam, ++modePriorityLevel );
		MODFN_REGISTER( GenerateClientSessionStructure, ++modePriorityLevel );
		MODFN_REGISTER( AdjustGeneralConstant, ++modePriorityLevel );
		MODFN_REGISTER( PostRunFrame, ++modePriorityLevel );
		MODFN_REGISTER( MatchStateTransition, ++modePriorityLevel );
	}
}
