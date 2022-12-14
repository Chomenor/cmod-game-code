/*
* Elimination Timelimit Handling
* 
* Adds support for updating timelimit when the last players are remaining, similar
* to Gladiator mod.
* 
* Controlled by g_mod_finalistsTimelimit cvar, which sets the amount of time left
* when down to the final 2 players in FFA or final 1 player on team.
*/

#define MOD_PREFIX( x ) ModElimTimelimit##x

#include "mods/modes/elimination/elim_local.h"

// Restart game when timelimit expires.
#define FEATURE_TIMELIMIT_RESTART

// Modify timelimit when game gets down to final players / only bots.
#define FEATURE_TIMELIMIT_ADJUST

static struct {
#ifdef FEATURE_TIMELIMIT_ADJUST
	trackedCvar_t g_mod_finalistsTimelimit;
	trackedCvar_t g_mod_onlyBotsTimelimit;
	qboolean finalistTimelimitActive;
	qboolean onlyBotsTimelimitActive;
	int oldTimelimit;
#endif

	// For mod function stacking
	ModFNType_PostGameShutdown Prev_PostGameShutdown;
	ModFNType_PostRunFrame Prev_PostRunFrame;
} *MOD_STATE;

#ifdef FEATURE_TIMELIMIT_ADJUST

// Once timelimit is adjusted, original timelimit cvar needs to be restored at end of round.
#define ADJUSTED_TIMELIMIT_ACTIVE ( MOD_STATE->finalistTimelimitActive || MOD_STATE->onlyBotsTimelimitActive )

/*
================
ModElimTimelimit_HumanPlayersAlive

Returns qtrue if any human players are alive on any team.
================
*/
static qboolean ModElimTimelimit_HumanPlayersAlive( void ) {
	int i;

	for ( i = 0; i < level.maxclients; ++i ) {
		gclient_t *client = &level.clients[i];
		gentity_t *ent = &g_entities[i];

		if ( client->pers.connected >= CON_CONNECTING && !( ent->r.svFlags & SVF_BOT ) &&
				!modfn.SpectatorClient( i ) && !ModElimination_Static_IsPlayerEliminated( i ) ) {
			return qtrue;
		}
	}

	return qfalse;
}

/*
================
ModElimTimelimit_FinalistState

Returns qtrue if down to final players.
================
*/
static qboolean ModElimTimelimit_FinalistState( void ) {
	if ( g_gametype.integer >= GT_TEAM ) {
		if ( ModElimination_Shared_CountPlayersAliveTeam( TEAM_RED, -1 ) == 1 ||
				ModElimination_Shared_CountPlayersAliveTeam( TEAM_BLUE, -1 ) == 1 ) {
			return qtrue;
		}
	} else {
		if ( ModElimination_Shared_CountPlayersAliveTeam( TEAM_FREE, -1 ) == 2 ) {
			return qtrue;
		}
	}
	return qfalse;
}

/*
================
ModElimTimelimit_ModifyTimelimit

Adjusts current timelimit so there are approximately remainingTime minutes remaining.

It's possible this could be done more precisely using CS_LEVEL_START_TIME, but for now just use the
Gladiator method of rounding up to the nearest minute because a bit of variation is probably fine anyway.
================
*/
static void ModElimTimelimit_ModifyTimelimit( int remainingTime, qboolean allowIncrease ) {
	// Round up to the next minute.
	int newTime = (level.time - level.startTime + remainingTime * 60000 - 1) / 60000 + 1;

	if ( newTime < g_timelimit.integer || allowIncrease ) {
		if ( !ADJUSTED_TIMELIMIT_ACTIVE ) {
			MOD_STATE->oldTimelimit = g_timelimit.integer;
		}

		trap_Cvar_Set( "timelimit", va( "%i", newTime ) );

		// Kludgy
		g_timelimit.integer = newTime;
		g_timelimit.announceChanges = qfalse;
	}
}

/*
================
ModElimTimelimit_CheckAdjustTimelimit
================
*/
static void ModElimTimelimit_CheckAdjustTimelimit( void ) {
	if ( level.matchState == MS_ACTIVE && ModElimination_Shared_MatchLocked() ) {
		if ( MOD_STATE->g_mod_onlyBotsTimelimit.integer > 0 && !MOD_STATE->onlyBotsTimelimitActive &&
				!ModElimTimelimit_HumanPlayersAlive() ) {
			ModElimTimelimit_ModifyTimelimit( MOD_STATE->g_mod_onlyBotsTimelimit.integer, qfalse );
			MOD_STATE->onlyBotsTimelimitActive = qtrue;
		}

		if ( MOD_STATE->g_mod_finalistsTimelimit.integer > 0 && !MOD_STATE->finalistTimelimitActive &&
				ModElimTimelimit_FinalistState() ) {
			qboolean allowIncrease = !MOD_STATE->onlyBotsTimelimitActive;
			ModElimTimelimit_ModifyTimelimit( MOD_STATE->g_mod_finalistsTimelimit.integer, allowIncrease );
			MOD_STATE->finalistTimelimitActive = qtrue;
		}
	}
}
#endif

/*
================
(ModFN) PostGameShutdown
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(PostGameShutdown), ( qboolean restart ), ( restart ), "G_MODFN_POSTGAMESHUTDOWN" ) {
	MOD_STATE->Prev_PostGameShutdown( restart );

#ifdef FEATURE_TIMELIMIT_ADJUST
	// Restore original timelimit value.
	if ( ADJUSTED_TIMELIMIT_ACTIVE ) {
		trap_Cvar_Set( "timelimit", va( "%i", MOD_STATE->oldTimelimit ) );
	}
#endif
}

/*
================
(ModFN) PostRunFrame
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(PostRunFrame), (void), (), "G_MODFN_POSTRUNFRAME" ) {
	MOD_STATE->Prev_PostRunFrame();

#ifdef FEATURE_TIMELIMIT_RESTART
	// Check for restart.
	if ( g_timelimit.integer && level.matchState == MS_ACTIVE && !level.exiting &&
			level.time - level.startTime >= g_timelimit.integer * 60000 ) {
		trap_SendServerCommand( -1, "print \"Timelimit hit, game restart...\n\"");
		level.exiting = qtrue;
		trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
	}
#endif

#ifdef FEATURE_TIMELIMIT_ADJUST
	ModElimTimelimit_CheckAdjustTimelimit();
#endif
}

/*
================
ModElimTimelimit_Init
================
*/
LOGFUNCTION_VOID( ModElimTimelimit_Init, ( void ), (), "G_MOD_INIT G_ELIMINATION" ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		INIT_FN_STACKABLE( PostGameShutdown );
		INIT_FN_STACKABLE( PostRunFrame );

#ifdef FEATURE_TIMELIMIT_ADJUST
		G_RegisterTrackedCvar( &MOD_STATE->g_mod_finalistsTimelimit, "g_mod_finalistsTimelimit", "0", CVAR_ARCHIVE, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_mod_onlyBotsTimelimit, "g_mod_onlyBotsTimelimit", "0", CVAR_ARCHIVE, qfalse );

		if ( MOD_STATE->g_mod_finalistsTimelimit.integer <= 0 ) {
			G_DedPrintf( "NOTE: Recommend setting 'g_mod_finalistsTimelimit' to 3 or 4 for Elimination mode.\n" );
		}
#endif
	}
}
