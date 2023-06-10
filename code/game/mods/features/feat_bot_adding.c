/*
* Bot Adding
* 
* This module replaces the regular bot_minplayers handling with an updated version.
* It improves avoidance of server player count limits to keep the server from
* overfilling with bots in certain situations, and adds several cvar options:
* 
* bot_minplayers "x:y" format: Support specifying separate number of bots on red and blue teams.
* g_bot_doubleRemove: Causes 2 bots to be removed for each human player that joins.
* g_bot_fastAdding: Increases the speed at which bots are added or removed.
*/

#define MOD_NAME ModBotAdding

#include "mods/g_mod_local.h"

static struct {
	// -1 = mod default
	// 0 = bot_minplayers specifies total number of bots on all teams (Gladiator behavior)
	// 1 = bot_minplayers specifies number of bots on each team (original EF behavior)
	trackedCvar_t g_bot_perTeamCount;

	// -1 = mod default
	// 0 = spectators cause bots to be removed in FFA
	// 1 = spectators do not cause bots to be removed in FFA
	trackedCvar_t g_bot_ignoreSpectators;

	trackedCvar_t g_bot_doubleRemove;
	trackedCvar_t g_bot_fastAdding;

	int checkTime;
} *MOD_STATE;

#define PER_TEAM_BOT_COUNT ( MOD_STATE->g_bot_perTeamCount.integer >= 0 ? \
		MOD_STATE->g_bot_perTeamCount.integer : modfn_lcl.AdjustModConstant( MC_BOTADD_PER_TEAM_COUNT, 1 ) )
#define PER_TEAM_BOT_ADJUST modfn_lcl.AdjustModConstant( MC_BOTADD_PER_TEAM_ADJUST, 0 )
#define IGNORE_SPECTATORS ( MOD_STATE->g_bot_ignoreSpectators.integer >= 0 ? \
		MOD_STATE->g_bot_ignoreSpectators.integer : modfn_lcl.AdjustModConstant( MC_BOTADD_IGNORE_SPECTATORS, 1 ) )

typedef struct {
	qboolean enabled;
	int red;
	int blue;
} TeamMinPlayers_t;

/*
===============
ModBotAdding_MaxActivePlayers

Returns the number of slots on the server that are available for active players (bot or human).
For example, if sv_maxclients is 8, and 2 spectators are present, this should return 6.
===============
*/
static int ModBotAdding_MaxActivePlayers( void ) {
	int available = level.maxclients - G_CountHumanPlayers( TEAM_SPECTATOR );

	if ( g_gametype.integer != GT_TOURNAMENT &&
			g_maxGameClients.integer > 0 && g_maxGameClients.integer < available ) {
		available = g_maxGameClients.integer;
	}

	return available;
}

/*
===============
ModBotAdding_PruneTeamTargets

Reduce the target number of bots on each team to try to keep the server under limits so players can join.
===============
*/
static void ModBotAdding_PruneTeamTargets( int *team1target, int *team2target ) {
	int humanPlayers = G_CountHumanPlayers( -1 ) - G_CountHumanPlayers( TEAM_SPECTATOR );
	int maxPlayers = ModBotAdding_MaxActivePlayers() - 1;	// leave 1 free slot
	int maxBots = maxPlayers - humanPlayers;
	int excess = *team1target + *team2target - maxBots;

	if ( excess > 0 ) {
		// try to remove half the excess per team
		*team1target -= ( excess + 1 ) / 2;
		*team2target -= ( excess + 1 ) / 2;

		// if one team went negative, try to remove the remainder from other team instead
		if ( *team1target < 0 ) {
			*team2target += *team1target;
		} else if ( *team2target < 0 ) {
			*team1target += *team2target;
		}

		if ( *team1target < 0 ) {
			*team1target = 0;
		}
		if ( *team2target < 0 ) {
			*team2target = 0;
		}
	}
}

/*
===============
ModBotAdding_PruneFFATarget

Reduce the target number of bots to try to keep the server under limits so players can join.
===============
*/
static void ModBotAdding_PruneFFATarget( int *target ) {
	int humanPlayers;
	int maxPlayers;
	int maxBots;

	if ( g_gametype.integer == GT_TOURNAMENT ) {
		humanPlayers = G_CountHumanPlayers( -1 );
		maxPlayers = level.maxclients - 1;
	} else {
		humanPlayers = G_CountHumanPlayers( TEAM_FREE );
		maxPlayers = ModBotAdding_MaxActivePlayers() - 1;
	}

	maxBots = maxPlayers - humanPlayers;
	if ( *target > maxBots ) {
		*target = maxBots;
	}
}

/*
===============
ModBotAdding_ReadTeamMinplayers

Returns the effective bot_minplayers values to use for each team.
===============
*/
static TeamMinPlayers_t ModBotAdding_ReadTeamMinplayers( void ) {
	TeamMinPlayers_t result;
	const char *split = strchr( bot_minplayers.string, ':' );
	memset( &result, 0, sizeof( result ) );
	if ( split ) {
		// A specific value was set for each team in "red:blue" format.
		result.enabled = qtrue;
		result.red = atoi( bot_minplayers.string );
		result.blue = atoi( &split[1] );
	} else {
		// A single value was set, determine whether it applies to total bots or each team.
		if ( bot_minplayers.integer > 0 ) {
			result.enabled = qtrue;
			if ( PER_TEAM_BOT_COUNT ) {
				result.red = result.blue = bot_minplayers.integer;
			} else {
				result.red = result.blue = bot_minplayers.integer / 2;
			}
		}
	}
	return result;
}

/*
===============
ModBotAdding_IsEnabled

Check if bot_minplayers is enabled.
===============
*/
static qboolean ModBotAdding_IsEnabled( void ) {
	if ( g_gametype.integer >= GT_TEAM ) {
		return ModBotAdding_ReadTeamMinplayers().enabled;
	} else {
		return bot_minplayers.integer > 0 ? qtrue : qfalse;
	}
}

/*
===============
ModBotAdding_TargetBotsForTeam

Determine number of bots to aim for on specified team. For example, in a simple
case, if bot_minplayers is 4 and there are 3 human players this would return 1.
===============
*/
static int ModBotAdding_TargetBotsForTeam( team_t team ) {
	int humans;
	int minPlayers;

	if ( g_gametype.integer == GT_TOURNAMENT || ( g_gametype.integer == GT_FFA && !IGNORE_SPECTATORS ) ) {
		humans = G_CountHumanPlayers( -1 );
	} else {
		humans = G_CountHumanPlayers( team );
	}

	if ( team == TEAM_RED ) {
		minPlayers = ModBotAdding_ReadTeamMinplayers().red;
	} else if ( team == TEAM_BLUE ) {
		minPlayers = ModBotAdding_ReadTeamMinplayers().blue;
	} else {
		minPlayers = bot_minplayers.integer;
	}

	// If g_bot_doubleRemove active, remove an extra bot for each human (except the first).
	if ( MOD_STATE->g_bot_doubleRemove.integer && humans > 1 ) {
		minPlayers -= humans - 1;
	}

	if ( minPlayers > humans ) {
		return minPlayers - humans;
	}
	return 0;
}

/*
================
ModBotAdding_CheckBots

Returns qtrue if bots were added or removed.
================
*/
static qboolean ModBotAdding_CheckBots( void ) {
	qboolean changed = qfalse;

	if ( !ModBotAdding_IsEnabled() ) {
		return qfalse;
	}

	if ( g_gametype.integer >= GT_TEAM ) {
		int redBots = G_CountBotPlayers( TEAM_RED );
		int blueBots = G_CountBotPlayers( TEAM_BLUE );
		int pendingBots = G_CountBotPlayers( -1 ) - redBots - blueBots;		// spectators, queued, or just added
		int redTarget = ModBotAdding_TargetBotsForTeam( TEAM_RED );			// number of red bots to aim for
		int blueTarget = ModBotAdding_TargetBotsForTeam( TEAM_BLUE );		// number of blue bots to aim for

		int redNeeded;
		int blueNeeded;
		int totalNeeded;

		ModBotAdding_PruneTeamTargets( &redTarget, &blueTarget );

		redNeeded = redTarget - redBots;
		blueNeeded = blueTarget - blueBots;
		totalNeeded = ( redNeeded > 0 ? redNeeded : 0 ) + ( blueNeeded > 0 ? blueNeeded : 0 );

		// check for removing excess pending bots
		if ( pendingBots > 0 && pendingBots > totalNeeded ) {
			if ( G_RemoveRandomBot( TEAM_SPECTATOR ) ) {
				if ( PER_TEAM_BOT_ADJUST ) {
					return qtrue;
				}
				--pendingBots;
			}
		}

		// only change bots for one team per cycle if specified by mod
		if ( PER_TEAM_BOT_ADJUST ) {
			// adjust team with most needed change first
			if ( abs( blueNeeded ) > abs( redNeeded ) || ( abs( blueNeeded ) == abs( redNeeded ) && rand() % 2 ) ) {
				redNeeded = 0;
			} else {
				blueNeeded = 0;
			}
		}

		// check for adding/removing red team bots
		if ( redNeeded < 0 && redBots ) {
			if ( G_RemoveRandomBot( TEAM_RED ) ) {
				--pendingBots;
				changed = qtrue;
			}
		} else if ( pendingBots < totalNeeded && redNeeded > 0 ) {
			G_AddRandomBot( TEAM_RED );
			++pendingBots;
			changed = qtrue;
		}

		// check for adding/removing blue team bots
		if ( blueNeeded < 0 && blueBots ) {
			if ( G_RemoveRandomBot( TEAM_BLUE ) ) {
				--pendingBots;
				changed = qtrue;
			}
		} else if ( pendingBots < totalNeeded && blueNeeded > 0 ) {
			G_AddRandomBot( TEAM_BLUE );
			++pendingBots;
			changed = qtrue;
		}
	}
	else if ( g_gametype.integer == GT_FFA || g_gametype.integer == GT_TOURNAMENT ) {
		int botPlayers = G_CountBotPlayers( -1 );
		int target = ModBotAdding_TargetBotsForTeam( TEAM_FREE );		// number of bots to aim for
		int needed;

		ModBotAdding_PruneFFATarget( &target );

		needed = target - botPlayers;

		if ( needed > 0 ) {
			G_AddRandomBot( TEAM_FREE );
			changed = qtrue;
		} else if ( needed < 0 && botPlayers ) {
			// try to remove spectators first
			if ( !G_RemoveRandomBot( TEAM_SPECTATOR ) ) {
				G_RemoveRandomBot( -1 );
			}
			changed = qtrue;
		}
	}

	return changed;
}

/*
==================
(ModFN) AdjustGeneralConstant

Disable standard bot_minplayers handling.
==================
*/
int MOD_PREFIX(AdjustGeneralConstant)( MODFN_CTV, generalConstant_t gcType, int defaultValue ) {
	if ( gcType == GC_DISABLE_BOT_ADDING ) {
		return 1;
	}

	return MODFN_NEXT( AdjustGeneralConstant, ( MODFN_NC, gcType, defaultValue ) );
}

/*
================
(ModFN) PostRunFrame

Check periodically for adding/removing bots.
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );

	if ( level.matchState < MS_INTERMISSION_QUEUED && !level.restartClientsPending &&
			level.time >= MOD_STATE->checkTime ) {
		qboolean changed = ModBotAdding_CheckBots();

		if ( MOD_STATE->g_bot_fastAdding.integer && changed ) {
			MOD_STATE->checkTime = level.time + 2000;
		} else {
			MOD_STATE->checkTime = level.time + 10000;
		}
	}
}

/*
================
ModBotAdding_Init
================
*/
void ModBotAdding_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MOD_STATE->checkTime = 10000;

		G_RegisterTrackedCvar( &MOD_STATE->g_bot_perTeamCount, "g_bot_perTeamCount", "-1", 0, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_bot_ignoreSpectators, "g_bot_ignoreSpectators", "-1", 0, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_bot_doubleRemove, "g_bot_doubleRemove", "0", 0, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_bot_fastAdding, "g_bot_fastAdding", "0", 0, qfalse );

		MODFN_REGISTER( AdjustGeneralConstant, MODPRIORITY_GENERAL );
		MODFN_REGISTER( PostRunFrame, MODPRIORITY_GENERAL );
	}
}
