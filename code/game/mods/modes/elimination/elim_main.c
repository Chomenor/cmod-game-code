/*
* Elimination Mode
* 
* In this mode players have only have one life and don't respawn until the match
* is over. In FFA mode the goal of the game is to eliminate everybody else, and in
* teams mode the goal is to eliminate everybody on the other team.
*/

#define MOD_NAME ModElimination

#include "mods/modes/elimination/elim_local.h"

typedef struct {
	qboolean eliminated;
	qboolean eliminatedSpect;	// eliminated and respawned as spectator
	int score;		// player's true score, even if PERS_SCORE is modified
} elimination_client_t;

static struct {
	elimination_client_t clients[MAX_CLIENTS];
	trackedCvar_t g_noJoinTimeout;

	// Current round state
	int numEliminated;
	int joinLimitTime;
} *MOD_STATE;

/*
================
ModElimination_Static_CountPlayersAlive

Returns total number of non-eliminated players in the game.
Valid to call even if Elimination mod not loaded.
================
*/
int ModElimination_Static_CountPlayersAlive( void ) {
	if ( MOD_STATE ) {
		int i;
		int count = 0;

		for ( i = 0; i < level.maxclients; ++i ) {
			gclient_t *client = &level.clients[i];
			elimination_client_t *modclient = &MOD_STATE->clients[i];

			if ( client->pers.connected >= CON_CONNECTING && !modclient->eliminated && !modfn.SpectatorClient( i ) ) {
				++count;
			}
		}

		return count;

	} else {
		return level.numNonSpectatorClients;
	}
}

/*
================
ModElimination_Shared_CountPlayersAliveTeam

Returns number of non-eliminated players on the specified team.
================
*/
int ModElimination_Shared_CountPlayersAliveTeam( team_t team, int ignoreClientNum ) {
	int i;
	int count = 0;

	for ( i = 0; i < level.maxclients; ++i ) {
		gclient_t *client = &level.clients[i];
		elimination_client_t *modclient = &MOD_STATE->clients[i];

		if ( client->pers.connected >= CON_CONNECTING && client->sess.sessionTeam == team &&
				ignoreClientNum != i && !modclient->eliminated && !modfn.SpectatorClient( i ) ) {
			++count;
		}
	}

	return count;
}

/*
================
ModElimination_Static_IsPlayerEliminated

Returns whether specific player is eliminated.
================
*/
qboolean ModElimination_Static_IsPlayerEliminated( int clientNum ) {
	return MOD_STATE && G_IsConnectedClient( clientNum ) && MOD_STATE->clients[clientNum].eliminated;
}

/*
================
ModElimination_Shared_MatchLocked

Match is considered locked if g_noJoinTimeout has been reached, or if one or more players
have been eliminated.
================
*/
qboolean ModElimination_Shared_MatchLocked( void ) {
	EF_ERR_ASSERT( MOD_STATE );

	if ( level.matchState >= MS_ACTIVE ) {
		if ( MOD_STATE->numEliminated > 0 ) {
			return qtrue;
		}

		else if ( MOD_STATE->g_noJoinTimeout.integer > 0 &&
				level.time >= MOD_STATE->joinLimitTime + ( MOD_STATE->g_noJoinTimeout.integer * 1000 ) ) {
			return qtrue;
		}
	}

	return qfalse;
}

/*
================
(ModFN) SpectatorClient

Treat eliminated players as spectators.
================
*/
static qboolean MOD_PREFIX(SpectatorClient)( MODFN_CTV, int clientNum ) {
	if( G_AssertConnectedClient( clientNum ) && MOD_STATE->clients[clientNum].eliminatedSpect ) {
		return qtrue;
	}

	return MODFN_NEXT( SpectatorClient, ( MODFN_NC, clientNum ) );
}

/*
================
(ModFN) AdjustScoreboardAttributes

Display red 'X' next to eliminated players name.
================
*/
static int MOD_PREFIX(AdjustScoreboardAttributes)( MODFN_CTV, int clientNum, scoreboardAttribute_t saType, int defaultValue ) {
	if ( saType == SA_ELIMINATED ) {
		elimination_client_t *modclient = &MOD_STATE->clients[clientNum];
		return modclient->eliminated ? 1 : 0;
	}

	return MODFN_NEXT( AdjustScoreboardAttributes, ( MODFN_NC, clientNum, saType, defaultValue ) );
}

/*
================
(ModFN) EffectiveScore

Use elimination score instead of PERS_SCORE to avoid issues with follow spectators
and for compatibility with round indicator features.
================
*/
static int MOD_PREFIX(EffectiveScore)( MODFN_CTV, int clientNum, effectiveScoreType_t type ) {
	gclient_t *client = &level.clients[clientNum];

	if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
		return MOD_STATE->clients[clientNum].score;
	} else {
		return 0;
	}
}

/*
================
(ModFN) PreClientSpawn

Transition recently eliminated players into spectator mode.
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(PreClientSpawn), ( MODFN_CTV, int clientNum, clientSpawnType_t spawnType ),
		( MODFN_CTN, clientNum, spawnType ), "G_MODFN_PRECLIENTSPAWN" ) {
	gclient_t *client = &level.clients[clientNum];
	elimination_client_t *modclient = &MOD_STATE->clients[clientNum];

	MODFN_NEXT( PreClientSpawn, ( MODFN_NC, clientNum, spawnType ) );
	modclient->eliminatedSpect = modclient->eliminated;
}

/*
===========
(ModFN) SpawnCenterPrintMessage

Print info messages to clients during ClientSpawn.
============
*/
LOGFUNCTION_SVOID( MOD_PREFIX(SpawnCenterPrintMessage), ( MODFN_CTV, int clientNum, clientSpawnType_t spawnType ),
		( MODFN_CTN, clientNum, spawnType ), "G_MODFN_SPAWNCENTERPRINTMESSAGE" ) {
	gclient_t *client = &level.clients[clientNum];

	// Don't print this if warmup sequence is playing or going to be played
	if ( ModWarmupSequence_Static_SequenceInProgressOrPending() ) {
		return;
	}

	if ( client->sess.sessionTeam == TEAM_SPECTATOR || spawnType != CST_RESPAWN ) {
		trap_SendServerCommand( clientNum, "cp \"Elimination\"" );
	}
}

/*
==============
(ModFN) CheckRespawnTime

Force respawn after 3 seconds.
==============
*/
LOGFUNCTION_SRET( qboolean, MOD_PREFIX(CheckRespawnTime), ( MODFN_CTV, int clientNum, qboolean voluntary ),
		( MODFN_CTN, clientNum, voluntary ), "G_MODFN_CHECKRESPAWNTIME" ) {
	gclient_t *client = &level.clients[clientNum];

	if ( !voluntary && level.time > client->respawnKilledTime + 3000 ) {
		return qtrue;
	}

	return MODFN_NEXT( CheckRespawnTime, ( MODFN_NC, clientNum, voluntary ) );
}

/*
==================
(ModFN) AdjustGeneralConstant
==================
*/
static int MOD_PREFIX(AdjustGeneralConstant)( MODFN_CTV, generalConstant_t gcType, int defaultValue ) {
	// In original implementation, intermission countdown starts when last player "respawns" to spectator.
	// Total time from final death to intermission ranges from ~3700 to ~5000 ms depending on whether
	// they press the respawn button.
	// Now intermission queue starts as soon as the last player dies, and isn't affected by respawn,
	// so we need to use a single delay value. Currently using 4500 as a balance between the original
	// potential values.
	if ( gcType == GC_INTERMISSION_DELAY_TIME )
		return 4500;

	// Elimination uses special scoring in which points are only awarded due to eliminations, so disable
	// other score updates through AddScore.
	if ( gcType == GC_DISABLE_ADDSCORE )
		return 1;

	return MODFN_NEXT( AdjustGeneralConstant, ( MODFN_NC, gcType, defaultValue ) );
}

/*
================
ModElimination_ExitRound
================
*/
LOGFUNCTION_SVOID( ModElimination_ExitRound, ( team_t winningTeam ), ( winningTeam ), "" ) {
	if ( g_gametype.integer >= GT_TEAM ) {
		// Make sure surviving team wins, not just team with most points.
		level.forceWinningTeam = winningTeam;
		CalculateRanks();
	}

	G_LogExit( "Last Man Standing." );
}

/*
================
(ModFN) CheckExitRules
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(CheckExitRules), ( MODFN_CTV ), ( MODFN_CTN ), "G_MODFN_CHECKEXITRULES" ) {
	// Don't go to intermission if nobody was eliminated.
	if ( MOD_STATE->numEliminated ) {
		if ( g_gametype.integer >= GT_TEAM ) {
			// Round ends when one team has no players left.
			if ( ModElimination_Shared_CountPlayersAliveTeam( TEAM_RED, -1 ) < 1 ) {
				ModElimination_ExitRound( TEAM_BLUE );
			} else if ( ModElimination_Shared_CountPlayersAliveTeam( TEAM_BLUE, -1 ) < 1 ) {
				ModElimination_ExitRound( TEAM_RED );
			}
		} else {
			// Round ends when only 1 player is remaining.
			if ( ModElimination_Shared_CountPlayersAliveTeam( TEAM_FREE, -1 ) < 2 ) {
				ModElimination_ExitRound( TEAM_FREE );
			}
		}
	}
}

/*
================
(ModFN) CheckJoinAllowed

Check if joining or changing team/class is disabled due to match in progress.
================
*/
LOGFUNCTION_SRET( qboolean, MOD_PREFIX(CheckJoinAllowed), ( MODFN_CTV, int clientNum, join_allowed_type_t type, team_t targetTeam ),
		( MODFN_CTN, clientNum, type, targetTeam ), "G_MODFN_CHECKJOINLOCKED" ) {
	gclient_t *client = &level.clients[clientNum];
	elimination_client_t *modclient = &MOD_STATE->clients[clientNum];

	if ( ModElimination_Shared_MatchLocked() ) {
		if ( type == CJA_SETTEAM ) {
			if ( modclient->eliminated ) {
				trap_SendServerCommand( clientNum, "cp \"You have been eliminated until next round\"" );
			} else if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
				trap_SendServerCommand( clientNum, "cp \"Wait until next round to join\"" );
			} else {
				trap_SendServerCommand( clientNum, "cp \"Wait until next round to change teams\"" );
			}
		}

		if ( type == CJA_SETCLASS ) {
			if ( modclient->eliminated ) {
				trap_SendServerCommand( clientNum, "cp \"You have been eliminated until next round\"" );
			} else if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
				trap_SendServerCommand( clientNum, "cp \"Wait until next round to join\"" );
			} else {
				trap_SendServerCommand( clientNum, "cp \"Wait until next round to change classes\"" );
			}
		}

		return qfalse;
	}

	return MODFN_NEXT( CheckJoinAllowed, ( MODFN_NC, clientNum, type, targetTeam ) );
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
	elimination_client_t *modclient = &MOD_STATE->clients[clientNum];

	MODFN_NEXT( PostPlayerDie, ( MODFN_NC, self, inflictor, attacker, meansOfDeath, awardPoints ) );

	// In elimination, you get scored by when you died, but knockout just respawns you, not kill you.
	if ( meansOfDeath != MOD_KNOCKOUT ) {
		modclient->eliminated = qtrue;
		MOD_STATE->numEliminated++;

		// Normally unused, but set in case some server engine modification wants to access it.
		self->r.svFlags |= SVF_ELIMINATED;

		if ( !level.warmupTime ) {
			// If we were eliminated by a player not on the same team, increment their scores.
			// Note that the normal AddScore increment is disabled to avoid any non-elimination scoring effects.
			if ( attacker && attacker->client && attacker != self &&
					( client->sess.sessionTeam == TEAM_FREE || attacker->client->sess.sessionTeam != client->sess.sessionTeam ) ) {
				elimination_client_t *attacker_modclient = &MOD_STATE->clients[attacker - g_entities];

				if ( attacker->client->sess.sessionTeam == TEAM_RED ) {
					level.teamScores[TEAM_RED]++;
				} else if ( attacker->client->sess.sessionTeam == TEAM_BLUE ) {
					level.teamScores[TEAM_BLUE]++;
				}

				if ( g_gametype.integer >= GT_TEAM ) {
					attacker_modclient->score++;
				}
			}

			// In FFA, increment score for all non-eliminated players.
			if ( g_gametype.integer < GT_TEAM ) {
				int i;
				for ( i = 0; i < level.maxclients; i++ ) {
					gclient_t *client = &level.clients[i];
					elimination_client_t *modclient = &MOD_STATE->clients[i];
					if ( client->pers.connected >= CON_CONNECTING && client->sess.sessionTeam != TEAM_SPECTATOR &&
							!MOD_STATE->clients[i].eliminated ) {
						modclient->score = MOD_STATE->numEliminated;
					}
				}
			}
		}
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
	elimination_client_t *modclient = &MOD_STATE->clients[clientNum];

	MODFN_NEXT( PrePlayerLeaveTeam, ( MODFN_NC, clientNum, oldTeam ) );

	// Reset eliminated state
	modclient->eliminated = qfalse;
	modclient->eliminatedSpect = qfalse;
	modclient->score = 0;
}

/*
================
(ModFN) InitClientSession
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(InitClientSession), ( MODFN_CTV, int clientNum, qboolean initialConnect, const info_string_t *info ),
		( MODFN_CTN, clientNum, initialConnect, info ), "G_MODFN_INITCLIENTSESSION" ) {
	elimination_client_t *modclient = &MOD_STATE->clients[clientNum];

	MODFN_NEXT( InitClientSession, ( MODFN_NC, clientNum, initialConnect, info ) );
	memset( modclient, 0, sizeof( *modclient ) );
}

/*
================
(ModFN) PostRunFrame
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(PostRunFrame), ( MODFN_CTV ), ( MODFN_CTN ), "G_MODFN_POSTRUNFRAME" ) {
	int i;
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );

	for ( i = 0; i < level.maxclients; ++i ) {
		gclient_t *client = &level.clients[i];
		if ( client->pers.connected >= CON_CONNECTING ) {
			elimination_client_t *modclient = &MOD_STATE->clients[i];

			// Set eliminated flag for clients who were eliminated, but are not follow spectating.
			if ( modclient->eliminatedSpect && client->sess.spectatorState != SPECTATOR_FOLLOW &&
					level.matchState != MS_INTERMISSION_ACTIVE ) {
				client->ps.eFlags |= EF_ELIMINATED;
			} else {
				client->ps.eFlags &= ~EF_ELIMINATED;
			}

			// Copy real score into PERS_SCORE except for follow spectators.
			// It shouldn't affect much normally, but it does allow the original HUD behavior of
			// displaying the order of elimination to work correctly if round indicator is disabled.
			if ( !( modclient->eliminatedSpect && client->sess.spectatorState == SPECTATOR_FOLLOW ) ) {
				client->ps.persistant[PERS_SCORE] = modclient->score;
			}

			// Patch clientNum ahead of awards command so AW_SPPostgameMenu_f can determine team correctly.
			// Fixes issue with incorrect "your team won/lost" messages.
			if ( modclient->eliminatedSpect && client->sess.spectatorState == SPECTATOR_FOLLOW && g_gametype.integer >= GT_TEAM &&
					level.matchState == MS_INTERMISSION_QUEUED && G_IsConnectedClient( client->sess.spectatorClient ) &&
					level.clients[client->sess.spectatorClient].sess.sessionTeam != client->sess.sessionTeam &&
					level.time - level.intermissionQueued + 100 >= modfn.AdjustGeneralConstant( GC_INTERMISSION_DELAY_TIME, 2000 ) ) {
				client->ps.clientNum = i;
				// Hack to keep model from flickering in...
				g_entities[client->sess.spectatorClient].s.eFlags |= EF_NODRAW;
			}
		}
	}
}

/*
================
(ModFN) MatchStateTransition
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(MatchStateTransition), ( MODFN_CTV, matchState_t oldState, matchState_t newState ),
		( MODFN_CTN, oldState, newState ), "G_MODFN_MATCHSTATETRANSITION" ) {
	MODFN_NEXT( MatchStateTransition, ( MODFN_NC, oldState, newState ) );

	if ( newState < MS_ACTIVE && !EF_WARN_ASSERT( MOD_STATE->numEliminated == 0 ) ) {
		// Shouldn't happen - if players were eliminated we should hit CheckExitRules instead.
		MOD_STATE->numEliminated = 0;
	}

	// Update join limit timer.
	if ( newState == MS_ACTIVE ) {
		G_DedPrintf( "elimination: Starting join limit countdown due to sufficient players.\n" );
		MOD_STATE->joinLimitTime = level.time;
	} else {
		if ( newState < MS_ACTIVE && MOD_STATE->joinLimitTime ) {
			G_DedPrintf( "elimination: Resetting join limit time due to insufficient players.\n" );
		}
		MOD_STATE->joinLimitTime = 0;
	}
}

/*
================
ModElimination_Init
================
*/
LOGFUNCTION_VOID( ModElimination_Init, ( void ), (), "G_MOD_INIT G_ELIMINATION" ) {
	if ( EF_WARN_ASSERT( !MOD_STATE ) ) {
		modcfg.mods_enabled.elimination = qtrue;
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		G_RegisterTrackedCvar( &MOD_STATE->g_noJoinTimeout, "g_noJoinTimeout", "120", CVAR_ARCHIVE, qfalse );

		// Support combining with other mods
		if ( G_ModUtils_GetLatchedValue( "g_pModDisintegration", "0", 0 ) ) {
			ModDisintegration_Init();
		} else if ( G_ModUtils_GetLatchedValue( "g_pModSpecialties", "0", 0 ) ) {
			ModSpecialties_Init();
		}

		MODFN_REGISTER( SpectatorClient, ++modePriorityLevel );
		MODFN_REGISTER( AdjustScoreboardAttributes, ++modePriorityLevel );
		MODFN_REGISTER( EffectiveScore, ++modePriorityLevel );
		MODFN_REGISTER( PreClientSpawn, ++modePriorityLevel );
		MODFN_REGISTER( SpawnCenterPrintMessage, ++modePriorityLevel );
		MODFN_REGISTER( CheckRespawnTime, ++modePriorityLevel );
		MODFN_REGISTER( AdjustGeneralConstant, ++modePriorityLevel );
		MODFN_REGISTER( CheckExitRules, ++modePriorityLevel );
		MODFN_REGISTER( CheckJoinAllowed, ++modePriorityLevel );
		MODFN_REGISTER( PostPlayerDie, ++modePriorityLevel );
		MODFN_REGISTER( PrePlayerLeaveTeam, ++modePriorityLevel );
		MODFN_REGISTER( InitClientSession, ++modePriorityLevel );
		MODFN_REGISTER( PostRunFrame, ++modePriorityLevel );
		MODFN_REGISTER( MatchStateTransition, ++modePriorityLevel );

		// Initialize additional features
		ModElimMisc_Init();
	}
}
