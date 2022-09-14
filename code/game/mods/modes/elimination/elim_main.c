/*
* Elimination Mode
* 
* In this mode players have only have one life and don't respawn until the match
* is over. In FFA mode the goal of the game is to eliminate everybody else, and in
* teams mode the goal is to eliminate everybody on the other team.
*/

#include "mods/modes/elimination/elim_local.h"

#define PREFIX( x ) ModElimination_##x
#define MOD_STATE PREFIX( state )

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

	// For mod function stacking
	ModFNType_SpectatorClient Prev_SpectatorClient;
	ModFNType_AdjustScoreboardAttributes Prev_AdjustScoreboardAttributes;
	ModFNType_PreClientSpawn Prev_PreClientSpawn;
	ModFNType_CheckRespawnTime Prev_CheckRespawnTime;
	ModFNType_AdjustGeneralConstant Prev_AdjustGeneralConstant;
	ModFNType_CheckJoinAllowed Prev_CheckJoinAllowed;
	ModFNType_PostPlayerDie Prev_PostPlayerDie;
	ModFNType_PrePlayerLeaveTeam Prev_PrePlayerLeaveTeam;
	ModFNType_InitClientSession Prev_InitClientSession;
	ModFNType_PostRunFrame Prev_PostRunFrame;
	ModFNType_MatchStateTransition Prev_MatchStateTransition;
} *MOD_STATE;

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
static qboolean PREFIX(SpectatorClient)( int clientNum ) {
	if( G_AssertConnectedClient( clientNum ) && MOD_STATE->clients[clientNum].eliminatedSpect ) {
		return qtrue;
	}

	return MOD_STATE->Prev_SpectatorClient( clientNum );
}

/*
================
(ModFN) AdjustScoreboardAttributes

Display red 'X' next to eliminated players name.
================
*/
int PREFIX(AdjustScoreboardAttributes)( int clientNum, scoreboardAttribute_t saType, int defaultValue ) {
	if ( saType == SA_ELIMINATED ) {
		elimination_client_t *modclient = &MOD_STATE->clients[clientNum];
		return modclient->eliminated ? 1 : 0;
	}

	return MOD_STATE->Prev_AdjustScoreboardAttributes( clientNum, saType, defaultValue );
}

/*
================
(ModFN) EffectiveScore

Use elimination score instead of PERS_SCORE to avoid issues with follow spectators.
================
*/
static int PREFIX(EffectiveScore)( int clientNum, effectiveScoreType_t type ) {
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
LOGFUNCTION_SVOID( PREFIX(PreClientSpawn), ( int clientNum, clientSpawnType_t spawnType ),
		( clientNum, spawnType ), "G_MODFN_PRECLIENTSPAWN" ) {
	gclient_t *client = &level.clients[clientNum];
	elimination_client_t *modclient = &MOD_STATE->clients[clientNum];

	MOD_STATE->Prev_PreClientSpawn( clientNum, spawnType );
	modclient->eliminatedSpect = modclient->eliminated;
}

/*
===========
(ModFN) SpawnCenterPrintMessage

Print info messages to clients during ClientSpawn.
============
*/
LOGFUNCTION_SVOID( PREFIX(SpawnCenterPrintMessage), ( int clientNum, clientSpawnType_t spawnType ),
		( clientNum, spawnType ), "G_MODFN_SPAWNCENTERPRINTMESSAGE" ) {
	gclient_t *client = &level.clients[clientNum];

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
LOGFUNCTION_SRET( qboolean, PREFIX(CheckRespawnTime), ( int clientNum, qboolean voluntary ),
		( clientNum, voluntary ), "G_MODFN_CHECKRESPAWNTIME" ) {
	gclient_t *client = &level.clients[clientNum];

	if ( !voluntary && level.time > client->respawnKilledTime + 3000 ) {
		return qtrue;
	}

	return MOD_STATE->Prev_CheckRespawnTime( clientNum, voluntary );
}

/*
==================
(ModFN) AdjustGeneralConstant
==================
*/
static int PREFIX(AdjustGeneralConstant)( generalConstant_t gcType, int defaultValue ) {
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

	return MOD_STATE->Prev_AdjustGeneralConstant( gcType, defaultValue );
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
LOGFUNCTION_SVOID( PREFIX(CheckExitRules), ( void ), (), "G_MODFN_CHECKEXITRULES" ) {
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
LOGFUNCTION_SRET( qboolean, PREFIX(CheckJoinAllowed), ( int clientNum, join_allowed_type_t type, team_t targetTeam ),
		( clientNum, type, targetTeam ), "G_MODFN_CHECKJOINLOCKED" ) {
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

	return MOD_STATE->Prev_CheckJoinAllowed( clientNum, type, targetTeam );
}

/*
================
(ModFN) PostPlayerDie
================
*/
LOGFUNCTION_SVOID( PREFIX(PostPlayerDie), ( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int meansOfDeath, int *awardPoints ),
		( self, inflictor, attacker, meansOfDeath, awardPoints ), "G_MODFN_POSTPLAYERDIE" ) {
	int clientNum = self - g_entities;
	gclient_t *client = &level.clients[clientNum];
	elimination_client_t *modclient = &MOD_STATE->clients[clientNum];

	MOD_STATE->Prev_PostPlayerDie( self, inflictor, attacker, meansOfDeath, awardPoints );

	// in elimination, you get scored by when you died, but knockout just respawns you, not kill you
	if ( meansOfDeath != MOD_KNOCKOUT ) {
		modclient->eliminated = qtrue;
		MOD_STATE->numEliminated++;

		// Normally unused, but set in case some server engine modification wants to access it
		self->r.svFlags |= SVF_ELIMINATED;

		if ( !level.warmupTime ) {
			// If we were eliminated by a player not on the same team, increment their scores
			// Note that the normal AddScore increment is disabled to avoid any non-elimination scoring effects
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

			// In FFA, increment score for all non-eliminated players
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
LOGFUNCTION_SVOID( PREFIX(PrePlayerLeaveTeam), ( int clientNum, team_t oldTeam ),
		( clientNum, oldTeam ), "G_MODFN_PREPLAYERLEAVETEAM" ) {
	elimination_client_t *modclient = &MOD_STATE->clients[clientNum];

	MOD_STATE->Prev_PrePlayerLeaveTeam( clientNum, oldTeam );

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
LOGFUNCTION_SVOID( PREFIX(InitClientSession), ( int clientNum, qboolean initialConnect, const info_string_t *info ),
		( clientNum, initialConnect, info ), "G_MODFN_INITCLIENTSESSION" ) {
	elimination_client_t *modclient = &MOD_STATE->clients[clientNum];

	MOD_STATE->Prev_InitClientSession( clientNum, initialConnect, info );
	memset( modclient, 0, sizeof( *modclient ) );
}

/*
================
(ModFN) PostRunFrame
================
*/
LOGFUNCTION_SVOID( PREFIX(PostRunFrame), (void), (), "G_MODFN_POSTRUNFRAME" ) {
	int i;
	MOD_STATE->Prev_PostRunFrame();

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
			// displaying the order of elimination to work correctly.
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
LOGFUNCTION_SVOID( PREFIX(MatchStateTransition), ( matchState_t oldState, matchState_t newState ),
		( oldState, newState ), "G_MODFN_MATCHSTATETRANSITION" ) {
	MOD_STATE->Prev_MatchStateTransition( oldState, newState );

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

#define INIT_FN_STACKABLE( name ) \
	MOD_STATE->Prev_##name = modfn.name; \
	modfn.name = PREFIX(name);

#define INIT_FN_OVERRIDE( name ) \
	modfn.name = PREFIX(name);

#define INIT_FN_BASE( name ) \
	EF_WARN_ASSERT( modfn.name == ModFNDefault_##name ); \
	modfn.name = PREFIX(name);

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

		INIT_FN_STACKABLE( SpectatorClient );
		INIT_FN_STACKABLE( AdjustScoreboardAttributes );
		INIT_FN_OVERRIDE( EffectiveScore );
		INIT_FN_STACKABLE( PreClientSpawn );
		INIT_FN_OVERRIDE( SpawnCenterPrintMessage );
		INIT_FN_STACKABLE( CheckRespawnTime );
		INIT_FN_STACKABLE( AdjustGeneralConstant );
		INIT_FN_BASE( CheckExitRules );
		INIT_FN_STACKABLE( CheckJoinAllowed );
		INIT_FN_STACKABLE( PostPlayerDie );
		INIT_FN_STACKABLE( PrePlayerLeaveTeam );
		INIT_FN_STACKABLE( InitClientSession );
		INIT_FN_STACKABLE( PostRunFrame );
		INIT_FN_STACKABLE( MatchStateTransition );

		if ( trap_Cvar_VariableIntegerValue( "fraglimit" ) != 0 && trap_Cvar_VariableIntegerValue( "dedicated" ) ) {
			G_Printf( "WARNING: Recommend setting 'fraglimit' to 0 for Elimination mode "
					"to remove unnecessary HUD indicator.\n" );
		}
	}
}
