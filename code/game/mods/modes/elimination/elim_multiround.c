/*
* Multi-Round Elimination
* 
* This module provides support for elimination matches containing multiple rounds,
* similar to Gladiator mod.
* 
* Enabled by setting g_mod_noOfGamesPerMatch to a value greater than 1.
*/

#define MOD_NAME ModElimMultiRound

#include "mods/modes/elimination/elim_local.h"

// Show a sequence of round info messages to clients during warmup.
#define FEATURE_WARMUP_MESSAGE_SEQUENCE

// Show round wins in place of current game score in FFA scoreboard.
#define FEATURE_SCOREBOARD_ROUND_WINS

// Show number of round wins instead of kills in HUD score indicator in team games.
#define FEATURE_HUD_TEAM_ROUND_WINS

// Perform Gladiator-style match score presentation at end of multi-round match.
#define FEATURE_FINAL_SCORES

// Set "currentround" serverinfo cvar so round info is visible to external server browsers.
#define FEATURE_CURRENTROUND_CVAR

#ifdef FEATURE_FINAL_SCORES
typedef enum {
	FS_INACTIVE,
	FS_PENDING_FIREBALL,			// next event: start "longfireball" sound effect
	FS_PENDING_TEAM_UPDATE,			// next event: update match winning team playerstate value
	FS_PENDING_FINAL_TRANSITION,	// next event: play detpack explosion and move match winner(s) to pedestal
	FS_COMPLETED
} finalScoresState_t;
#endif

typedef struct {
	int matchKills;		// total number of kills during match
	int roundWins;		// number of round wins (ffa only)
} eliminationMR_client_t;

static struct {
	eliminationMR_client_t clients[MAX_CLIENTS];

	trackedCvar_t g_mod_noOfGamesPerMatch;
	trackedCvar_t g_mod_endWhenDecided;

	// current round number (1 = first round) and tie state
	// may exceed g_mod_noOfGamesPerMatch in case of tiebreaker
	int currentRound;
	qboolean tiebreaker;

	// round number and tie state of next round (set when entering intermission)
	// 0 for end of match or intermission not yet reached
	int pendingRound;
	qboolean pendingTiebreaker;

	// number of round wins (team mode only)
	int redWins;
	int blueWins;

#ifdef FEATURE_FINAL_SCORES
	finalScoresState_t finalScores_state;
	int finalScores_endTime;
#endif
} *MOD_STATE;

#define TOTAL_ROUNDS ( MOD_STATE->g_mod_noOfGamesPerMatch.integer >= 1 ? MOD_STATE->g_mod_noOfGamesPerMatch.integer : 1 )
#define MULTI_ROUND_ENABLED ( MOD_STATE->g_mod_noOfGamesPerMatch.integer > 1 )

// Save match state on end of warmup, timelimit reset, and round transition.
// Don't save state on admin map/restart command or end of match.
#define WRITE_SESSION ( level.exiting && ( level.matchState < MS_INTERMISSION_ACTIVE || MOD_STATE->pendingRound ) )

/*
================
ModElimMultiRound_Static_GetTotalRounds

Returns the current target number of rounds.
================
*/
int ModElimMultiRound_Static_GetTotalRounds( void ) {
	if ( !MOD_STATE ) {
		return 1;
	}
	return TOTAL_ROUNDS;
}

/*
================
ModElimMultiRound_Static_GetCurrentRound

Returns the current round number. Does not exceed the total number of rounds in case of tiebreaker.
================
*/
int ModElimMultiRound_Static_GetCurrentRound( void ) {
	if ( !MOD_STATE ) {
		return 1;
	}
	return MOD_STATE->currentRound <= ModElimMultiRound_Static_GetTotalRounds() ?
			MOD_STATE->currentRound : ModElimMultiRound_Static_GetTotalRounds();
}

#ifdef FEATURE_WARMUP_MESSAGE_SEQUENCE
/*
================
ModElimMultiRound_WarmupTiebreakerMessage
================
*/
static void ModElimMultiRound_WarmupTiebreakerMessage( const char *msg ) {
	trap_SendServerCommand( -1, va( "cp \"\nRound %i of %i\n\n^5Tiebreaker Round\"",
			ModElimMultiRound_Static_GetCurrentRound(), ModElimMultiRound_Static_GetTotalRounds() ) );
}

/*
================
ModElimMultiRound_WarmupRoundMessage

Message string: "a" to display always, "b" if click-to-join inactive, "c" if click-to-join active
This is a bit of a hack.
================
*/
static void ModElimMultiRound_WarmupRoundMessage( const char *msg ) {
	int i;

	for ( i = 0; i < level.maxclients; ++i ) {
		if ( level.clients[i].pers.connected == CON_CONNECTED && ( msg[0] == 'a' ||
				( msg[0] == 'b' && !ModClickToJoin_Static_ActiveForClient( i ) ) ||
				( msg[0] == 'c' && ModClickToJoin_Static_ActiveForClient( i ) ) ) ) {
			trap_SendServerCommand( i, va( "cp \"Round %i of %i\"", ModElimMultiRound_Static_GetCurrentRound(),
					ModElimMultiRound_Static_GetTotalRounds() ) );
		}
	}
}

/*
================
(ModFN) GetWarmupSequence

Returns message sequence to use during warmup.
================
*/
static qboolean MOD_PREFIX(GetWarmupSequence)( MODFN_CTV, modWarmupSequence_t *sequence ) {
	if ( !g_doWarmup.integer || !MULTI_ROUND_ENABLED ) {
		return qfalse;
	}

	if ( MOD_STATE->currentRound == 1 ) {
		sequence->duration = g_warmup.integer * 1000;
		if ( sequence->duration < 10000 ) {
			sequence->duration = 10000;
		}

		ModWarmupSequence_Static_AddEventToSequence( sequence, sequence->duration - 4000, ModElimMultiRound_WarmupRoundMessage, "a" );
		return qtrue;
	}

	if ( MOD_STATE->tiebreaker ) {
		ModWarmupSequence_Static_AddEventToSequence( sequence, 3000, ModElimMultiRound_WarmupTiebreakerMessage, "a" );
		sequence->duration = 7000;
		return qtrue;
	}

	// Delay the message a bit for spectators so the "click fire button to join" message is visible.
	ModWarmupSequence_Static_AddEventToSequence( sequence, 0, ModElimMultiRound_WarmupRoundMessage, "b" );
	ModWarmupSequence_Static_AddEventToSequence( sequence, 3000, ModElimMultiRound_WarmupRoundMessage, "c" );
	sequence->duration = 6000;
	return qtrue;
}
#endif

#ifdef FEATURE_FINAL_SCORES
// Total time from start of intermission to score transition
#define FINALSCORES_LENGTH 20000

/*
================
ModElimMultiRound_FinalScoresDetpackBlast
================
*/
static void ModElimMultiRound_FinalScoresDetpackBlast( void ) {
	gentity_t *te;
	vec3_t vec;
	vec3_t origin;

	// explosion sound
	G_GlobalSound( G_SoundIndex( "sound/weapons/explosions/explode12.wav" ) );

	// calculate detpack origin as per SetPodiumOrigin
	AngleVectors( level.intermission_angle, vec, NULL, NULL );
	VectorMA( level.intermission_origin, g_podiumDist.value, vec, origin );
	origin[2] -= g_podiumDrop.value;

	// adjust to first place player origin as per Gladiator mod
	origin[2] += 64.0f;

	te = G_TempEntity( origin, EV_DETPACK );
	te->r.svFlags |= SVF_BROADCAST;
}

/*
================
ModElimMultiRound_CheckFinalScores

Perform match scores sequence.
================
*/
static void ModElimMultiRound_CheckFinalScores( void ) {
	if ( MOD_STATE->finalScores_state == FS_PENDING_FIREBALL && level.time >= MOD_STATE->finalScores_endTime - 1500 ) {
		G_GlobalSound( G_SoundIndex( "sound/weapons/explosions/longfireball.wav" ) );
		MOD_STATE->finalScores_state = FS_PENDING_TEAM_UPDATE;
	}

	if ( MOD_STATE->finalScores_state == FS_PENDING_TEAM_UPDATE && level.time >= MOD_STATE->finalScores_endTime - 200 ) {
		MOD_STATE->finalScores_state = FS_PENDING_FINAL_TRANSITION;

		if ( g_gametype.integer >= GT_TEAM ) {
			// As soon as the UpdateTournamentInfo "awards" command is received, the client will grab the winning team
			// from the playerstate PERS_RANK value. However the client is slow to process playerstate updates compared
			// to commands, so we need to call CalculateRanks to update the PERS_RANK values ahead of the awards command.
			level.forceWinningTeam = TEAM_FREE;
			level.teamScores[TEAM_RED] = MOD_STATE->redWins;
			level.teamScores[TEAM_BLUE] = MOD_STATE->blueWins;
			CalculateRanks();
		}
	}

	if ( MOD_STATE->finalScores_state == FS_PENDING_FINAL_TRANSITION && level.time >= MOD_STATE->finalScores_endTime ) {
		MOD_STATE->finalScores_state = FS_COMPLETED;
		CalculateRanks();
		SendScoreboardMessageToAllClients();
		UpdateTournamentInfo();
		SpawnModelsOnVictoryPads();
		ModElimMultiRound_FinalScoresDetpackBlast();
		ModIntermissionReady_Shared_Resume();
	}
}

/*
================
ModElimMultiRound_FinalScoresStartSequence
================
*/
static void ModElimMultiRound_FinalScoresStartSequence( void ) {
	MOD_STATE->finalScores_state = FS_PENDING_FIREBALL;
	MOD_STATE->finalScores_endTime = level.time + FINALSCORES_LENGTH;
	ModIntermissionReady_Shared_Suspend();
}
#endif

/*
================
(ModFN) AdjustScoreboardAttributes
================
*/
static int MOD_PREFIX(AdjustScoreboardAttributes)( MODFN_CTV, int clientNum, scoreboardAttribute_t saType, int defaultValue ) {
#ifdef FEATURE_FINAL_SCORES
	// Set scoreboard times and eliminated count to 0 on the final match scoreboard.
	if ( MOD_STATE->finalScores_state >= FS_COMPLETED && ( saType == SA_PLAYERTIME || saType == SA_NUM_DEATHS ) ) {
		return 0;
	}
#endif

	return MODFN_NEXT( AdjustScoreboardAttributes, ( MODFN_NC, clientNum, saType, defaultValue ) );
}

/*
================
(ModFN) EffectiveScore
================
*/
static int MOD_PREFIX(EffectiveScore)( MODFN_CTV, int clientNum, effectiveScoreType_t type ) {
	gclient_t *client = &level.clients[clientNum];
	eliminationMR_client_t *modclient = &MOD_STATE->clients[clientNum];

#ifdef FEATURE_FINAL_SCORES
	// Use match stats on the final scoreboard.
	if ( MOD_STATE->finalScores_state == FS_COMPLETED && client->sess.sessionTeam != TEAM_SPECTATOR ) {
		if ( g_gametype.integer >= GT_TEAM ) {
			return modclient->matchKills;
		} else {
			return modclient->roundWins;
		}
	}
#endif

#ifdef FEATURE_SCOREBOARD_ROUND_WINS
	// Show round wins in place of current game score in scoreboard.
	if ( type == EST_SCOREBOARD && g_gametype.integer < GT_TEAM && MULTI_ROUND_ENABLED ) {
		return modclient->roundWins;
	}
#endif

	return MODFN_NEXT( EffectiveScore, ( MODFN_NC, clientNum, type ) );
}

/*
================
(ModFN) CalculateAwards

Generate final match awards based on Gladiator mod.
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(CalculateAwards), ( MODFN_CTV, int clientNum, char *msg ), ( MODFN_CTN, clientNum, msg ), "G_MODFN_CALCULATEAWARDS" ) {
	if ( MOD_STATE->finalScores_state == FS_COMPLETED ) {
		gentity_t *ent = &g_entities[clientNum];
		eliminationMR_client_t *modclient = &MOD_STATE->clients[clientNum];
		char buf1[AWARDS_MSG_LENGTH], buf2[AWARDS_MSG_LENGTH];
		int awardFlags = 0;

		memset( buf1, 0, AWARDS_MSG_LENGTH );
		memset( buf2, 0, AWARDS_MSG_LENGTH );

		if ( g_gametype.integer < GT_TEAM ) {
			if ( ent->client == &level.clients[level.sortedClients[0]] ) {
				// Champion
				awardFlags |= ( 1 << AWARD_STREAK );
				strcpy( buf2, buf1 );
				Com_sprintf( buf1, AWARDS_MSG_LENGTH, "%s %d", buf2, STREAK_CHAMPION );
			}

			else if ( modclient->matchKills ) {
				// Warrior
				awardFlags |= ( 1 << AWARD_TEAM );
				strcpy( buf2, buf1 );
				Com_sprintf( buf1, AWARDS_MSG_LENGTH, "%s %d", buf2, 1 << TEAM_WARRIOR );
			}

			else {
				// Bravery
				awardFlags |= ( 1 << AWARD_TEAM );
				strcpy( buf2, buf1 );
				Com_sprintf( buf1, AWARDS_MSG_LENGTH, "%s %d", buf2, 1 << TEAM_BRAVERY );
			}
		}

		else {
			if ( CalculateTeamMVP( ent ) ) {
				// Master
				awardFlags |= ( 1 << AWARD_STREAK );
				strcpy( buf2, buf1 );
				Com_sprintf( buf1, AWARDS_MSG_LENGTH, "%s %d", buf2, STREAK_MASTER );
			}

			else if ( modclient->matchKills ) {
				// Warrior
				awardFlags |= ( 1 << AWARD_TEAM );
				strcpy( buf2, buf1 );
				Com_sprintf( buf1, AWARDS_MSG_LENGTH, "%s %d", buf2, 1 << TEAM_WARRIOR );
			}

			else {
				// Bravery
				awardFlags |= ( 1 << AWARD_TEAM );
				strcpy( buf2, buf1 );
				Com_sprintf( buf1, AWARDS_MSG_LENGTH, "%s %d", buf2, 1 << TEAM_BRAVERY );
			}
		}

		strcpy( buf2, msg );
		Com_sprintf( msg, AWARDS_MSG_LENGTH, "%s %d%s", buf2, awardFlags, buf1 );
	}

	else {
		MODFN_NEXT( CalculateAwards, ( MODFN_NC, clientNum, msg ) );
	}
}

/*
============
(ModFN) SetScoresConfigStrings
============
*/
LOGFUNCTION_SVOID( MOD_PREFIX(SetScoresConfigStrings), ( MODFN_CTV ), ( MODFN_CTN ), "G_MODFN_SETSCORESCONFIGSTRINGS" ) {
#ifdef FEATURE_HUD_TEAM_ROUND_WINS
	if ( g_gametype.integer >= GT_TEAM ) {
		trap_SetConfigstring( CS_SCORES1, va( "%i", MOD_STATE->redWins ) );
		trap_SetConfigstring( CS_SCORES2, va( "%i", MOD_STATE->blueWins ) );
		return;
	}
#endif

	MODFN_NEXT( SetScoresConfigStrings, ( MODFN_NC ) );
}

/*
=============
(ModFN) ExitLevel
=============
*/
LOGFUNCTION_SVOID( MOD_PREFIX(ExitLevel), ( MODFN_CTV ), ( MODFN_CTN ), "G_MODFN_EXITLEVEL" ) {
	if ( MOD_STATE->pendingRound ) {
		// Restart when transitioning to next round.
		trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
	} else {
		MODFN_NEXT( ExitLevel, ( MODFN_NC ) );
	}
}

/*
================
(ModFN) PostPlayerDie
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(PostPlayerDie), ( MODFN_CTV, gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int meansOfDeath, int *awardPoints ),
		( MODFN_CTN, self, inflictor, attacker, meansOfDeath, awardPoints ), "G_MODFN_POSTPLAYERDIE" ) {
	int clientNum = self - g_entities;
	gclient_t *client = &level.clients[clientNum];

	MODFN_NEXT( PostPlayerDie, ( MODFN_NC, self, inflictor, attacker, meansOfDeath, awardPoints ) );

	// If we were eliminated by a player not on the same team, increment their match scores.
	if ( level.matchState == MS_ACTIVE && ModElimination_Static_IsPlayerEliminated( clientNum ) &&
			attacker && attacker->client && attacker != self &&
			( client->sess.sessionTeam == TEAM_FREE || attacker->client->sess.sessionTeam != client->sess.sessionTeam ) ) {
		eliminationMR_client_t *attacker_modclient = &MOD_STATE->clients[attacker - g_entities];
		attacker_modclient->matchKills++;
	}
}

/*
================
(ModFN) GenerateGlobalSessionInfo
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(GenerateGlobalSessionInfo), ( MODFN_CTV, info_string_t *info ), ( MODFN_CTN, info ), "G_MODFN_GENERATEGLOBALSESSIONINFO" ) {
	MODFN_NEXT( GenerateGlobalSessionInfo, ( MODFN_NC, info ) );

	if ( WRITE_SESSION ) {
		int pendingRound = MOD_STATE->pendingRound ? MOD_STATE->pendingRound : MOD_STATE->currentRound;
		qboolean pendingTiebreaker = MOD_STATE->pendingRound ? MOD_STATE->pendingTiebreaker : MOD_STATE->tiebreaker;
		Info_SetValueForKey_Big( info->s, "elim_session", "1" );
		Info_SetValueForKey_Big( info->s, "elim_currentRound", va( "%i", pendingRound ) );
		Info_SetValueForKey_Big( info->s, "elim_tiebreaker", va( "%i", pendingTiebreaker ) );
		Info_SetValueForKey_Big( info->s, "elim_redWins", va( "%i", MOD_STATE->redWins ) );
		Info_SetValueForKey_Big( info->s, "elim_blueWins", va( "%i", MOD_STATE->blueWins ) );
	}
}

/*
================
ModElimMultiRound_LoadGlobalSession
================
*/
static void ModElimMultiRound_LoadGlobalSession( void ) {
	info_string_t info;
	G_RetrieveGlobalSessionInfo( &info );

	if ( atoi( Info_ValueForKey( info.s, "elim_session" ) ) ) {
		MOD_STATE->currentRound = atoi( Info_ValueForKey( info.s, "elim_currentRound" ) );
		MOD_STATE->tiebreaker = atoi( Info_ValueForKey( info.s, "elim_tiebreaker" ) );
		MOD_STATE->redWins = atoi( Info_ValueForKey( info.s, "elim_redWins" ) );
		MOD_STATE->blueWins = atoi( Info_ValueForKey( info.s, "elim_blueWins" ) );
	}
}

/*
================
(ModFN) GenerateClientSessionInfo
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(GenerateClientSessionInfo), ( MODFN_CTV, int clientNum, info_string_t *info ),
		( MODFN_CTN, clientNum, info ), "G_MODFN_GENERATECLIENTSESSIONINFO" ) {
	MODFN_NEXT( GenerateClientSessionInfo, ( MODFN_NC, clientNum, info ) );

	if ( WRITE_SESSION ) {
		eliminationMR_client_t *modclient = &MOD_STATE->clients[clientNum];
		Info_SetValueForKey_Big( info->s, "elim_matchKills", va( "%i", modclient->matchKills ) );
		Info_SetValueForKey_Big( info->s, "elim_roundWins", va( "%i", modclient->roundWins ) );
	}
}

/*
================
(ModFN) InitClientSession
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(InitClientSession), ( MODFN_CTV, int clientNum, qboolean initialConnect, const info_string_t *info ),
		( MODFN_CTN, clientNum, initialConnect, info ), "G_MODFN_INITCLIENTSESSION" ) {
	eliminationMR_client_t *modclient = &MOD_STATE->clients[clientNum];

	MODFN_NEXT( InitClientSession, ( MODFN_NC, clientNum, initialConnect, info ) );
	memset( modclient, 0, sizeof( *modclient ) );

	if ( !initialConnect ) {
		// Read client session info.
		modclient->matchKills = atoi ( Info_ValueForKey( info->s, "elim_matchKills" ) );
		modclient->roundWins = atoi ( Info_ValueForKey( info->s, "elim_roundWins" ) );
	}
}

/*
================
(ModFN) PrePlayerLeaveTeam

Reset stats and eliminated state when player switches teams or becomes spectator.
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(PrePlayerLeaveTeam), ( MODFN_CTV, int clientNum, team_t oldTeam ),
		( MODFN_CTN, clientNum, oldTeam ), "G_MODFN_PREPLAYERLEAVETEAM" ) {
	eliminationMR_client_t *modclient = &MOD_STATE->clients[clientNum];

	MODFN_NEXT( PrePlayerLeaveTeam, ( MODFN_NC, clientNum, oldTeam ) );

	// Reset match stats.
	modclient->matchKills = 0;
	modclient->roundWins = 0;
}

/*
================
(ModFN) PostGameShutdown
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(PostGameShutdown), ( MODFN_CTV, qboolean restart ), ( MODFN_CTN, restart ), "G_MODFN_POSTGAMESHUTDOWN" ) {
	MODFN_NEXT( PostGameShutdown, ( MODFN_NC, restart ) );

#ifdef FEATURE_CURRENTROUND_CVAR
	// Clear currentround cvar so it doesn't stick around if changing modes.
	trap_Cvar_Set( "currentround", "" );
#endif
}

/*
================
(ModFN) PostRunFrame
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(PostRunFrame), ( MODFN_CTV ), ( MODFN_CTN ), "G_MODFN_POSTRUNFRAME" ) {
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );

#ifdef FEATURE_FINAL_SCORES
	ModElimMultiRound_CheckFinalScores();
#endif
}

typedef enum {
	RSS_INCOMPLETE,
	RSS_TIEBREAKER,
	RSS_COMPLETE
} roundScoresState_t;

/*
================
ModElimMultiRound_GetRoundScoresState

Used to determine when the match is over.
================
*/
static roundScoresState_t ModElimMultiRound_GetRoundScoresState( void ) {
	int firstPlacePoints = 0;
	int secondPlacePoints = 0;

	if ( g_gametype.integer >= GT_TEAM ) {
		if ( MOD_STATE->redWins > MOD_STATE->blueWins ) {
			firstPlacePoints = MOD_STATE->redWins;
			secondPlacePoints = MOD_STATE->blueWins;
		} else {
			firstPlacePoints = MOD_STATE->blueWins;
			secondPlacePoints = MOD_STATE->redWins;
		}
	}

	else {
		int i;
		for ( i = 0; i < level.maxclients; ++i ) {
			gclient_t *client = &level.clients[i];
			eliminationMR_client_t *modclient = &MOD_STATE->clients[i];

			if ( client->pers.connected == CON_DISCONNECTED ) {
				continue;
			}

			if ( modclient->roundWins > secondPlacePoints ) {
				secondPlacePoints = modclient->roundWins;
			}

			if ( secondPlacePoints > firstPlacePoints ) {
				int swap = firstPlacePoints;
				firstPlacePoints = secondPlacePoints;
				secondPlacePoints = swap;
			}
		}
	}

	if ( MOD_STATE->currentRound >= TOTAL_ROUNDS ) {
		if ( firstPlacePoints == secondPlacePoints ) {
			return RSS_TIEBREAKER;
		} else {
			return RSS_COMPLETE;
		}
	}

	if ( MOD_STATE->g_mod_endWhenDecided.integer && firstPlacePoints - secondPlacePoints >
			ModElimMultiRound_Static_GetTotalRounds() - ModElimMultiRound_Static_GetCurrentRound() ) {
		return RSS_COMPLETE;
	}

	return RSS_INCOMPLETE;
}

/*
================
(ModFN) MatchStateTransition
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(MatchStateTransition), ( MODFN_CTV, matchState_t oldState, matchState_t newState ),
		( MODFN_CTN, oldState, newState ), "G_MODFN_MATCHSTATETRANSITION" ) {
	MODFN_NEXT( MatchStateTransition, ( MODFN_NC, oldState, newState ) );

	if ( newState == MS_WAITING_FOR_PLAYERS ) {
		// Reset rounds (and stats) if we run out of players.
		int i;
		G_DedPrintf( "elimination: Resetting round state due to no players.\n" );

		for ( i = 0; i < level.maxclients; ++i ) {
			eliminationMR_client_t *modclient = &MOD_STATE->clients[i];
			modclient->matchKills = 0;
			modclient->roundWins = 0;
		}

		MOD_STATE->currentRound = 1;
		MOD_STATE->tiebreaker = qfalse;
		MOD_STATE->redWins = 0;
		MOD_STATE->blueWins = 0;
		CalculateRanks();
	}

	if ( newState == MS_INTERMISSION_QUEUED ) {
		// We should have a winning player or team for the round at this point, so update stats.
		if ( g_gametype.integer >= GT_TEAM ) {
			if ( level.winningTeam == TEAM_RED ) {
				MOD_STATE->redWins++;
			} else if ( level.winningTeam == TEAM_BLUE ) {
				MOD_STATE->blueWins++;
			}

#ifdef FEATURE_HUD_TEAM_ROUND_WINS
			// Make sure score indicator is updated.
			CalculateRanks();
#endif
		
		} else {
			if ( G_AssertConnectedClient( level.sortedClients[0] ) ) {
				// Survivor should be first sorted client.
				eliminationMR_client_t *modclient = &MOD_STATE->clients[level.sortedClients[0]];
				modclient->roundWins++;
			}
		}
	}

	if ( newState == MS_INTERMISSION_ACTIVE ) {
		// Decide at this point if we want to start the final score sequence, or go into another round.
		if ( MULTI_ROUND_ENABLED ) {
			roundScoresState_t state = ModElimMultiRound_GetRoundScoresState();

			if ( state == RSS_INCOMPLETE ) {
				MOD_STATE->pendingRound = MOD_STATE->currentRound + 1;
				G_DedPrintf( "elimination: Setting pending round to %i.\n", MOD_STATE->pendingRound );

			} else if ( state == RSS_TIEBREAKER ) {
				MOD_STATE->pendingRound = MOD_STATE->currentRound + 1;
				MOD_STATE->pendingTiebreaker = qtrue;
				G_DedPrintf( "elimination: Setting pending round to %i (tiebreaker).\n", MOD_STATE->pendingRound );
		
			} else {
#ifdef FEATURE_FINAL_SCORES
				ModElimMultiRound_FinalScoresStartSequence();
#endif
			}
		}
	}
}

/*
================
ModElimMultiRound_Init
================
*/
LOGFUNCTION_VOID( ModElimMultiRound_Init, ( void ), (), "G_MOD_INIT G_ELIMINATION_MR" ) {
	if ( !EF_WARN_ASSERT( !MOD_STATE ) ) {
		return;
	}

	MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

	MODFN_REGISTER( AdjustScoreboardAttributes );
	MODFN_REGISTER( EffectiveScore );
	MODFN_REGISTER( CalculateAwards );
	MODFN_REGISTER( SetScoresConfigStrings );
	MODFN_REGISTER( ExitLevel );
	MODFN_REGISTER( PostPlayerDie );
	MODFN_REGISTER( GenerateGlobalSessionInfo );
	MODFN_REGISTER( GenerateClientSessionInfo );
	MODFN_REGISTER( InitClientSession );
	MODFN_REGISTER( PrePlayerLeaveTeam );
	MODFN_REGISTER( PostGameShutdown );
	MODFN_REGISTER( PostRunFrame );
	MODFN_REGISTER( MatchStateTransition );

	MOD_STATE->currentRound = 1;
	G_RegisterTrackedCvar( &MOD_STATE->g_mod_noOfGamesPerMatch, "g_mod_noOfGamesPerMatch", "1", CVAR_ARCHIVE, qfalse );
	G_RegisterTrackedCvar( &MOD_STATE->g_mod_endWhenDecided, "g_mod_endWhenDecided", "0", CVAR_ARCHIVE, qfalse );

	ModElimMultiRound_LoadGlobalSession();

#ifdef FEATURE_WARMUP_MESSAGE_SEQUENCE
	ModWarmupSequence_Init();
	MODFN_REGISTER( GetWarmupSequence );
#endif

#ifdef FEATURE_FINAL_SCORES
	// needed for suspend support
	ModIntermissionReady_Init();
#endif

#ifdef FEATURE_CURRENTROUND_CVAR
	{
		vmCvar_t temp;
		trap_Cvar_Register( &temp, "currentround", "", CVAR_SERVERINFO );
		trap_Cvar_Set( "currentround", va( "%i/%i",
				ModElimMultiRound_Static_GetCurrentRound(), ModElimMultiRound_Static_GetTotalRounds() ) );
	}
#endif
}
