/*
* Elimination Timelimit Handling
* 
* Adds support for updating timelimit when the last players are remaining, similar
* to Gladiator mod.
* 
* Controlled by g_mod_finalistsTimelimit cvar, which sets the amount of time left
* when down to the final 2 players in FFA or final 1 player on team.
*/

#define MOD_NAME ModElimTimelimit

#include "mods/modes/elimination/elim_local.h"

static struct {
	trackedCvar_t g_mod_finalistsTimelimit;
	trackedCvar_t g_mod_onlyBotsTimelimit;
	qboolean finalistTimelimitActive;
	qboolean onlyBotsTimelimitActive;
	int oldTimelimit;
} *MOD_STATE;

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
	if ( level.matchState == MS_ACTIVE && ModJoinLimit_Static_MatchLocked() ) {
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

/*
================
(ModFN) PostGameShutdown
================
*/
static void MOD_PREFIX(PostGameShutdown)( MODFN_CTV, qboolean restart ) {
	MODFN_NEXT( PostGameShutdown, ( MODFN_NC, restart ) );

	// Restore original timelimit value.
	if ( ADJUSTED_TIMELIMIT_ACTIVE ) {
		trap_Cvar_Set( "timelimit", va( "%i", MOD_STATE->oldTimelimit ) );
	}
}

/*
================
(ModFN) PostRunFrame
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );
	ModElimTimelimit_CheckAdjustTimelimit();
}

/*
================
ModElimTimelimit_Init
================
*/
void ModElimTimelimit_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( PostGameShutdown, ++modePriorityLevel );
		MODFN_REGISTER( PostRunFrame, ++modePriorityLevel );

		G_RegisterTrackedCvar( &MOD_STATE->g_mod_finalistsTimelimit, "g_mod_finalistsTimelimit", "0", CVAR_ARCHIVE, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_mod_onlyBotsTimelimit, "g_mod_onlyBotsTimelimit", "0", CVAR_ARCHIVE, qfalse );

		if ( MOD_STATE->g_mod_finalistsTimelimit.integer <= 0 ) {
			G_DedPrintf( "NOTE: Recommend setting 'g_mod_finalistsTimelimit' to 3 or 4 for Elimination mode.\n" );
		}
	}
}
